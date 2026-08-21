//
// The one and only translation unit in modcurses that touches curses.
//
// Everything here is written against BUILD_NOTES section 2 (the API
// differences table) and TUI_DESIGN section 10 (the contractual constructor
// sequence). Every line of that sequence is a past bug; do not reorder it.
//
// Standard headers come first, deliberately: curses.h defines macros that
// collide with libstdc++/MSVC STL identifiers, so nothing std:: may be
// included after the shim.
//
#include <algorithm>
#include <charconv>
#include <chrono>
#include <clocale>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <variant>
#include <vector>

#include "modcurses/core.hpp"
#include "modcurses/render.hpp"
#include "modcurses/terminal.hpp"

#define MODCURSES_CURSES_SHIM_OWNER 1
#include "curses_shim.hpp"

namespace modcurses {
namespace {

// --------------------------------------------------------------- env fixup

void unset_env(const char* name) {
#if defined(_WIN32)
    // Windows has no unsetenv; assigning an empty value removes the variable.
    _putenv_s(name, "");
#else
    ::unsetenv(name);
#endif
}

// getenv is deprecated on MSVC and there is no portable spelling, so the
// branch lives here with the other env handling rather than leaking outward.
std::optional<std::string> get_env(const char* name) {
#if defined(_WIN32)
    char* buf = nullptr;
    std::size_t len = 0;
    if (_dupenv_s(&buf, &len, name) != 0 || buf == nullptr) return std::nullopt;
    std::string value{buf};
    std::free(buf);
    return value;
#else
    const char* v = std::getenv(name);
    if (v == nullptr) return std::nullopt;
    return std::string{v};
#endif
}

// IDE consoles (CLion's emulated terminal) export LINES=1, and BOTH backends
// trust these env vars over the real console size - so initscr() aborts with
// "LINES value must be >= 2". Clear anything nonsensical before init.
void sanitize_size_env(const char* name) {
    const auto v = get_env(name);
    if (!v) return;
    const std::string_view sv{*v};
    int n = 0;
    const auto* end = sv.data() + sv.size();
    const auto res = std::from_chars(sv.data(), end, n);
    if (res.ec != std::errc{} || res.ptr != end || n < 2) unset_env(name);
}

// ------------------------------------------------------------ colour tables

constexpr bool is_bright(Color c) {
    return static_cast<int>(c) >= static_cast<int>(Color::BrightBlack);
}

// Returns the curses colour number, or -1 for Color::Default (which
// use_default_colors() maps to the terminal's own default).
//
// NEVER derive this arithmetically from the enum's own order. The two
// backends do not agree on the numbering: ncurses has COLOR_RED == 1 and
// COLOR_BLUE == 4, while PDCurses has COLOR_RED == 4 and COLOR_BLUE == 1
// (its constants follow the Windows console's blue-green-red bit order).
// Computing `int(c) - 1` hardcoded ncurses' answer, so every blue came out
// red and every yellow came out cyan on Windows - while looking perfect on
// Linux. The COLOR_* macros are the only portable source of truth.
int curses_color_index(Color c, int colors_available) {
    int index = 0;
    switch (c) {
        case Color::Default: return -1;
        case Color::Black:
        case Color::BrightBlack: index = COLOR_BLACK; break;
        case Color::Red:
        case Color::BrightRed: index = COLOR_RED; break;
        case Color::Green:
        case Color::BrightGreen: index = COLOR_GREEN; break;
        case Color::Yellow:
        case Color::BrightYellow: index = COLOR_YELLOW; break;
        case Color::Blue:
        case Color::BrightBlue: index = COLOR_BLUE; break;
        case Color::Magenta:
        case Color::BrightMagenta: index = COLOR_MAGENTA; break;
        case Color::Cyan:
        case Color::BrightCyan: index = COLOR_CYAN; break;
        case Color::White:
        case Color::BrightWhite: index = COLOR_WHITE; break;
    }
    // The bright half sits 8 above the base hue on both backends, but only
    // when the terminal actually has 16 colours; otherwise it folds onto the
    // base hue and attrs_for() adds A_BOLD to get most of the way back.
    if (is_bright(c) && colors_available >= 16) index += 8;
    return index;
}

// PDCurses' character field is 16 bits wide (A_CHARTEXT == 0x0000ffff), so
// Windows is BMP-only in v1. Narrowing failures become U+FFFD rather than
// silently truncating into a different glyph.
wchar_t narrow_codepoint(char32_t cp) {
    if (cp >= 0xD800u && cp <= 0xDFFFu) return static_cast<wchar_t>(0xFFFD);
#if defined(_WIN32)
    if (cp > 0xFFFFu) return static_cast<wchar_t>(0xFFFD);
#else
    if (cp > 0x10FFFFu) return static_cast<wchar_t>(0xFFFD);
#endif
    return static_cast<wchar_t>(cp);
}

// -------------------------------------------------------------- the backend

class CursesTerminal final : public TerminalIO {
public:
    CursesTerminal() {
        // A second live terminal would fight over curses' global state. Check
        // curses' own globals rather than keeping static state of our own.
        if (stdscr != nullptr && !isendwin())
            throw TerminalError{"a curses terminal is already initialised"};

        // (1) Sanitise the size env vars BEFORE initscr - they outrank the
        //     real console size on both backends.
        sanitize_size_env("LINES");
        sanitize_size_env("COLUMNS");
        sanitize_size_env("COLS");

        // (2) Locale: required for wide output under ncursesw, harmless here.
        std::setlocale(LC_ALL, "");

        // (3) Init, and actually check the result. CPPurses never did.
        if (initscr() == nullptr || stdscr == nullptr)
            throw TerminalError{
                "initscr() failed: no usable terminal. On Windows, PDCurses "
                "rejects piped stdin ('Redirection is not supported') - run "
                "this from a real console, not an IDE's emulated one."};

        // (4) Input mode. raw() means Ctrl-C arrives as a key, not a signal,
        //     so the application MUST provide a way out (App installs a
        //     default quit shortcut).
        raw();
        noecho();
        keypad(stdscr, TRUE);
        curs_set(0);
        nodelay(stdscr, FALSE);

        // (5) Mouse. mouseinterval(0) disables curses' own click synthesis;
        //     the library does its own so both backends behave alike.
        mousemask(ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION, nullptr);
        mouseinterval(0);
#if !defined(_WIN32) && defined(NCURSES_VERSION)
        // ncurses can do the ESC/Alt disambiguation itself; PDCurses has no
        // ESCDELAY at all, so we also do it by hand in read_after_escape().
        set_escdelay(kEscDelayMs);
#endif

        // (6) Colour. Pair 0 stays as-is; every other pair is allocated
        //     lazily, on first use, by pair_for().
        if (has_colors()) {
            start_color();
            has_default_colors_ = (use_default_colors() == OK);
            colors_available_ = COLORS;
            // COLOR_PAIRS can exceed a short on ncurses; pair ids cannot.
            max_pairs_ = std::min<int>(COLOR_PAIRS, 32767);
        }

        // (7) Only now is it safe to ask how big the screen is.
        refresh_size();
    }

    ~CursesTerminal() override { endwin(); }

    CursesTerminal(const CursesTerminal&) = delete;
    CursesTerminal& operator=(const CursesTerminal&) = delete;

    // ------------------------------------------------------------ TerminalIO

    Size size() override { return refresh_size(); }

    std::optional<Event> poll_event(std::optional<std::chrono::milliseconds> timeout) override {
        int tmo = -1;  // block forever
        if (timeout) {
            const auto ms = timeout->count();
            tmo = ms <= 0 ? 0 : static_cast<int>(std::min<long long>(ms, 1000LL * 60 * 60));
        }
        wtimeout(stdscr, tmo);

        wint_t wch = 0;
        const int rc = wget_wch(stdscr, &wch);
        if (rc == ERR) return std::nullopt;  // timed out
        if (rc == KEY_CODE_YES) return translate_special(static_cast<int>(wch));
        return translate_char(static_cast<char32_t>(wch), Mods{});
    }

    void draw_run(Point origin, std::u32string_view run, Style style) override {
        if (run.empty()) return;
        if (origin.y < 0 || origin.y >= rows_ || origin.x < 0 || origin.x >= cols_) return;

        const short pair = pair_for(style);
        const attr_t attrs = attrs_for(style);
        const int room = cols_ - origin.x;

        cells_.clear();
        cells_.reserve(run.size());
        for (char32_t cp : run) {
            if (static_cast<int>(cells_.size()) >= room) break;  // never wrap
            wchar_t wbuf[2] = {narrow_codepoint(cp), L'\0'};
            cchar_t cc{};
            // NOTE: the colour goes in setcchar's dedicated pair argument.
            // A raw pair number OR'd into the attributes lands in the chtype's
            // CHARACTER bits and renders the whole UI black-on-black - the
            // single worst CPPurses bug. Never construct one by hand; if you
            // ever must, it is COLOR_PAIR(n), always.
            if (setcchar(&cc, wbuf, attrs, pair, nullptr) != OK) {
                wbuf[0] = L'?';
                if (setcchar(&cc, wbuf, attrs, pair, nullptr) != OK) continue;
            }
            cells_.push_back(cc);
        }
        if (cells_.empty()) return;

        if (wmove(stdscr, origin.y, origin.x) == ERR) return;
        // wadd_wchnstr writes without advancing the cursor and without
        // scrolling, so even the bottom-right cell is safe to paint.
        wadd_wchnstr(stdscr, cells_.data(), static_cast<int>(cells_.size()));
    }

    void set_cursor(std::optional<Point> pos) override {
        cursor_ = pos;
        apply_cursor();
        // Applied immediately AND re-applied in flush(), so this works whether
        // the loop positions the cursor before or after the frame's flush.
        wrefresh(stdscr);
    }

    void flush() override {
        apply_cursor();
        wrefresh(stdscr);
    }

    bool define_color(Color slot, Rgb value) override {
        if (!can_define_colors()) return false;
        const int idx = curses_color_index(slot, colors_available_);
        if (idx < 0 || idx >= colors_available_) return false;
        const auto scale = [](std::uint8_t c) {
            return static_cast<short>((static_cast<int>(c) * 1000) / 255);
        };
        return init_color(static_cast<short>(idx), scale(value.r), scale(value.g),
                          scale(value.b)) == OK;
    }

    bool can_define_colors() override { return has_colors() && can_change_color(); }

    void beep() override { ::beep(); }

private:
    static constexpr int kEscDelayMs = 25;

    // ----------------------------------------------------------- size

    Size refresh_size() {
        const int w = getmaxx(stdscr);
        const int h = getmaxy(stdscr);
        // ERR is -1. Assigning that into unsigned geometry is what produced
        // CPPurses' 18446744073709551615-cell layout; here we keep the last
        // known-good value instead.
        if (w != ERR && h != ERR && w > 0 && h > 0) {
            cols_ = w;
            rows_ = h;
        }
        return Size{cols_, rows_};
    }

    // -------------------------------------------------------- attributes

    [[nodiscard]] attr_t attrs_for(Style s) const {
        attr_t a = A_NORMAL;
        if (s.has(Trait::Bold)) a |= A_BOLD;
        if (s.has(Trait::Underline)) a |= A_UNDERLINE;
        if (s.has(Trait::Reverse)) a |= A_REVERSE;
        if (s.has(Trait::Dim)) a |= A_DIM;  // a no-op alias on PDCurses
        if (s.has(Trait::Blink)) a |= A_BLINK;
#ifdef A_ITALIC
        if (s.has(Trait::Italic)) a |= A_ITALIC;
#endif
        // On an 8-colour terminal the bright foregrounds fold onto their base
        // hue (see curses_color_index); bold gets most of the way back.
        if (colors_available_ < 16 && is_bright(s.fg)) a |= A_BOLD;
        return a;
    }

    // Lazy (fg, bg) -> pair id. 17 colours means 289 combinations but
    // COLOR_PAIRS is only 256 on PDCurses; real applications use a dozen, so
    // allocating on demand never reaches the ceiling in practice. CPPurses
    // eagerly ground out all 256 and had no headroom left.
    short pair_for(Style s) {
        if (!has_colors()) return 0;
        if (s.fg == Color::Default && s.bg == Color::Default) return 0;

        const auto key = std::pair<Color, Color>{s.fg, s.bg};
        if (const auto it = pairs_.find(key); it != pairs_.end()) return it->second;

        if (next_pair_ >= max_pairs_) {
            pair_exhaustion_ = true;  // reuse the terminal default; do not wrap
            return 0;
        }

        int fg = curses_color_index(s.fg, colors_available_);
        int bg = curses_color_index(s.bg, colors_available_);
        if (!has_default_colors_) {  // no -1 support: pick something visible
            if (fg < 0) fg = COLOR_WHITE;
            if (bg < 0) bg = COLOR_BLACK;
        }

        const short id = next_pair_;
        if (init_pair(id, static_cast<short>(fg), static_cast<short>(bg)) != OK) return 0;
        ++next_pair_;
        pairs_.emplace(key, id);
        return id;
    }

    void apply_cursor() {
        if (cursor_ && cursor_->x >= 0 && cursor_->y >= 0 && cursor_->x < cols_ &&
            cursor_->y < rows_) {
            curs_set(1);
            wmove(stdscr, cursor_->y, cursor_->x);
        } else {
            curs_set(0);
        }
    }

    // ------------------------------------------------------- translation

    std::optional<Event> translate_special(int code) {
        switch (code) {
            case KEY_RESIZE:
#if defined(_WIN32)
                // PDCurses does NOT resync its internal buffers on its own;
                // without this the screen garbles or the app wedges. ncurses
                // has already done the equivalent by the time we get here.
                resize_term(0, 0);
#endif
                return Event{ResizeEvent{refresh_size()}};
            case KEY_MOUSE:
                return translate_mouse();
            default:
                break;
        }
        if (const auto k = extended_to_key(code)) {
            if (k->key == Key::None) return std::nullopt;  // a bare modifier press
            return Event{*k};
        }
        if (const auto k = special_to_key(code)) return Event{*k};
        return std::nullopt;  // a key we have no name for: swallow it
    }

    // PDCurses' wincon backend does NOT send Alt-chords as an ESC prefix the
    // way a POSIX terminal does. It has its own key codes (ALT_A, CTL_LEFT,
    // ...), so read_after_escape() never sees them and every Alt-chord would
    // be silently swallowed on Windows. This table is the Windows half of
    // "Alt/Ctrl detection"; read_after_escape() is the POSIX half.
    static std::optional<KeyEvent> extended_to_key(int code) {
#if defined(_WIN32)
        const Mods a{false, true, false};   // alt
        const Mods c{true, false, false};   // ctrl
        const Mods s{false, false, true};   // shift

        if (code >= ALT_A && code <= ALT_Z)
            return KeyEvent{Key::Char, static_cast<char32_t>(U'a' + (code - ALT_A)), a};
        if (code >= ALT_0 && code <= ALT_9)
            return KeyEvent{Key::Char, static_cast<char32_t>(U'0' + (code - ALT_0)), a};

        switch (code) {
            // Alt + punctuation
            case ALT_MINUS: return KeyEvent{Key::Char, U'-', a};
            case ALT_EQUAL: return KeyEvent{Key::Char, U'=', a};
            case ALT_BQUOTE: return KeyEvent{Key::Char, U'`', a};
            case ALT_LBRACKET: return KeyEvent{Key::Char, U'[', a};
            case ALT_RBRACKET: return KeyEvent{Key::Char, U']', a};
            case ALT_SEMICOLON: return KeyEvent{Key::Char, U';', a};
            case ALT_FQUOTE: return KeyEvent{Key::Char, U'\'', a};
            case ALT_COMMA: return KeyEvent{Key::Char, U',', a};
            case ALT_STOP: return KeyEvent{Key::Char, U'.', a};
            case ALT_FSLASH: return KeyEvent{Key::Char, U'/', a};
            case ALT_BSLASH: return KeyEvent{Key::Char, U'\\', a};

            // Navigation with a modifier
            case CTL_LEFT: return KeyEvent{Key::Left, 0, c};
            case CTL_RIGHT: return KeyEvent{Key::Right, 0, c};
            case CTL_UP: return KeyEvent{Key::Up, 0, c};
            case CTL_DOWN: return KeyEvent{Key::Down, 0, c};
            case CTL_HOME: return KeyEvent{Key::Home, 0, c};
            case CTL_END: return KeyEvent{Key::End, 0, c};
            case CTL_PGUP: return KeyEvent{Key::PageUp, 0, c};
            case CTL_PGDN: return KeyEvent{Key::PageDown, 0, c};
            case ALT_LEFT: return KeyEvent{Key::Left, 0, a};
            case ALT_RIGHT: return KeyEvent{Key::Right, 0, a};
            case ALT_UP: return KeyEvent{Key::Up, 0, a};
            case ALT_DOWN: return KeyEvent{Key::Down, 0, a};
            case ALT_HOME: return KeyEvent{Key::Home, 0, a};
            case ALT_END: return KeyEvent{Key::End, 0, a};
            case ALT_PGUP: return KeyEvent{Key::PageUp, 0, a};
            case ALT_PGDN: return KeyEvent{Key::PageDown, 0, a};
            case KEY_SUP:
            case SHF_UP: return KeyEvent{Key::Up, 0, s};
            case KEY_SDOWN:
            case SHF_DOWN: return KeyEvent{Key::Down, 0, s};

            // Editing keys with a modifier
            case ALT_BKSP: return KeyEvent{Key::Backspace, 0, a};
            case CTL_BKSP: return KeyEvent{Key::Backspace, 0, c};
            case ALT_DEL: return KeyEvent{Key::Delete, 0, a};
            case CTL_DEL: return KeyEvent{Key::Delete, 0, c};
            case SHF_DC: return KeyEvent{Key::Delete, 0, s};
            case ALT_INS: return KeyEvent{Key::Insert, 0, a};
            case CTL_INS: return KeyEvent{Key::Insert, 0, c};
            case SHF_IC: return KeyEvent{Key::Insert, 0, s};
            case ALT_TAB: return KeyEvent{Key::Tab, 0, a};
            case CTL_TAB: return KeyEvent{Key::Tab, 0, c};
            case ALT_ENTER:
            case ALT_PADENTER: return KeyEvent{Key::Enter, 0, a};
            case CTL_ENTER:
            case CTL_PADENTER: return KeyEvent{Key::Enter, 0, c};
            case SHF_PADENTER: return KeyEvent{Key::Enter, 0, s};
            case PADENTER: return KeyEvent{Key::Enter, 0, {}};
            case ALT_ESC: return KeyEvent{Key::Escape, 0, a};

            // Numeric keypad, unmodified
            case PAD0: return KeyEvent{Key::Char, U'0', {}};
            case PADSTOP: return KeyEvent{Key::Char, U'.', {}};
            case PADSTAR: return KeyEvent{Key::Char, U'*', {}};
            case PADMINUS: return KeyEvent{Key::Char, U'-', {}};
            case PADPLUS: return KeyEvent{Key::Char, U'+', {}};
            case PADSLASH: return KeyEvent{Key::Char, U'/', {}};

            // Bare modifier presses are not keys; drop them so applications
            // never see a spurious event from tapping Shift.
            case KEY_SHIFT_L:
            case KEY_SHIFT_R:
            case KEY_CONTROL_L:
            case KEY_CONTROL_R:
            case KEY_ALT_L:
            case KEY_ALT_R: return KeyEvent{Key::None, 0, {}};

            default: return std::nullopt;
        }
#else
        (void)code;
        return std::nullopt;  // ncurses delivers Alt as an ESC prefix instead
#endif
    }

    static std::optional<KeyEvent> special_to_key(int code) {
        Mods m;
        switch (code) {
            case KEY_UP: return KeyEvent{Key::Up, 0, m};
            case KEY_DOWN: return KeyEvent{Key::Down, 0, m};
            case KEY_LEFT: return KeyEvent{Key::Left, 0, m};
            case KEY_RIGHT: return KeyEvent{Key::Right, 0, m};
            case KEY_HOME: return KeyEvent{Key::Home, 0, m};
            case KEY_END: return KeyEvent{Key::End, 0, m};
            case KEY_NPAGE: return KeyEvent{Key::PageDown, 0, m};
            case KEY_PPAGE: return KeyEvent{Key::PageUp, 0, m};
            case KEY_IC: return KeyEvent{Key::Insert, 0, m};
            case KEY_DC: return KeyEvent{Key::Delete, 0, m};
            case KEY_BACKSPACE: return KeyEvent{Key::Backspace, 0, m};
            case KEY_ENTER: return KeyEvent{Key::Enter, 0, m};
            case KEY_BTAB: m.shift = true; return KeyEvent{Key::BackTab, 0, m};
#ifdef KEY_SLEFT
            case KEY_SLEFT: m.shift = true; return KeyEvent{Key::Left, 0, m};
#endif
#ifdef KEY_SRIGHT
            case KEY_SRIGHT: m.shift = true; return KeyEvent{Key::Right, 0, m};
#endif
#ifdef KEY_SHOME
            case KEY_SHOME: m.shift = true; return KeyEvent{Key::Home, 0, m};
#endif
#ifdef KEY_SEND
            case KEY_SEND: m.shift = true; return KeyEvent{Key::End, 0, m};
#endif
#ifdef KEY_SDC
            case KEY_SDC: m.shift = true; return KeyEvent{Key::Delete, 0, m};
#endif
            default: break;
        }
        if (code >= KEY_F(1) && code <= KEY_F(12))
            return KeyEvent{static_cast<Key>(static_cast<int>(Key::F1) + (code - KEY_F(1))), 0, m};
        return std::nullopt;
    }

    std::optional<Event> translate_char(char32_t c, Mods mods) {
        // Named keys win over the control codes that share their value:
        // Ctrl-I is Tab, Ctrl-J/Ctrl-M are Enter, Ctrl-H is Backspace. That is
        // terminal reality on both backends, not a choice we can make.
        switch (c) {
#if defined(_WIN32)
            // PDCurses' wincon backend never uses an ESC prefix for Alt - it
            // has its own ALT_* key codes (see extended_to_key). Running the
            // POSIX disambiguation here would swallow the real Escape AND
            // misreport whatever key followed it as an Alt-chord.
            case 27: return Event{KeyEvent{Key::Escape, 0, mods}};
#else
            case 27: return mods.alt ? std::optional<Event>{KeyEvent{Key::Escape, 0, mods}}
                                     : read_after_escape();
#endif
            case 9: return Event{KeyEvent{Key::Tab, 0, mods}};
            // CR is Enter. LF is Ctrl-J, and they are NOT the same key: in
            // raw() mode the terminal does no CR/LF translation, so Enter
            // arrives as 13 and Ctrl-J as 10 on both backends. Folding them
            // together made Ctrl-J insert a newline instead of running
            // whatever the application bound to it - nano binds Justify
            // there, and it silently split the line instead.
            // (Ctrl-M is genuinely indistinguishable from Enter; that one is
            // a property of every terminal, not a choice made here.)
            case 13: return Event{KeyEvent{Key::Enter, 0, mods}};
            case 10: mods.ctrl = true; return Event{KeyEvent{Key::Char, U'j', mods}};
            case 8:
            case 127: return Event{KeyEvent{Key::Backspace, 0, mods}};
            case 0: mods.ctrl = true; return Event{KeyEvent{Key::Char, U' ', mods}};
            default: break;
        }
        if (c >= 1 && c <= 26) {
            mods.ctrl = true;
            return Event{KeyEvent{Key::Char, U'a' + (c - 1), mods}};
        }
        // The four control codes above Ctrl-Z that terminals really do send.
        // They are not decoration: nano binds Ctrl-\ to Replace and Ctrl-_ to
        // Go To Line, and dropping them makes those commands unreachable.
        // (27 is Escape and was handled above.)
        switch (c) {
            case 28: mods.ctrl = true; return Event{KeyEvent{Key::Char, U'\\', mods}};
            case 29: mods.ctrl = true; return Event{KeyEvent{Key::Char, U']', mods}};
            case 30: mods.ctrl = true; return Event{KeyEvent{Key::Char, U'^', mods}};
            // Ctrl-/ arrives as this too on most terminals.
            case 31: mods.ctrl = true; return Event{KeyEvent{Key::Char, U'_', mods}};
            default: break;
        }
        if (c < 32) return std::nullopt;  // an unmapped control code
        return Event{KeyEvent{Key::Char, c, mods}};
    }

    // ESC disambiguation: peek for ~25ms. A key inside that window is Alt+key;
    // nothing is a bare Escape. ncurses can do this via ESCDELAY, PDCurses has
    // no ESCDELAY at all, so we do it by hand for both.
    std::optional<Event> read_after_escape() {
        wtimeout(stdscr, kEscDelayMs);
        wint_t wch = 0;
        const int rc = wget_wch(stdscr, &wch);
        if (rc == ERR) return Event{KeyEvent{Key::Escape, 0, {}}};

        Mods m;
        m.alt = true;
        if (rc == KEY_CODE_YES) {
            auto ev = translate_special(static_cast<int>(wch));
            if (ev && std::holds_alternative<KeyEvent>(*ev))
                std::get<KeyEvent>(*ev).mods.alt = true;
            return ev;
        }
        return translate_char(static_cast<char32_t>(wch), m);
    }

    std::optional<Event> translate_mouse() {
        MEVENT me{};
        if (getmouse(&me) != OK) return std::nullopt;

        MouseEvent ev;
        // PDCurses' wincon backend hardcodes x = y = -1 for wheel events
        // (verified in wincon/pdckbd.c), while ncurses reports the real
        // position. Carrying the last known position forward normalises the
        // two, so wheel events can be hit-tested to a widget like any other.
        if (me.x >= 0 && me.y >= 0) last_mouse_ = Point{me.x, me.y};
        ev.pos = last_mouse_;
        const mmask_t bs = me.bstate;

#ifdef BUTTON_SHIFT
        ev.mods.shift = (bs & BUTTON_SHIFT) != 0;
#endif
#ifdef BUTTON_CTRL
        ev.mods.ctrl = (bs & BUTTON_CTRL) != 0;
#endif
#ifdef BUTTON_ALT
        ev.mods.alt = (bs & BUTTON_ALT) != 0;
#endif

        using B = MouseEvent::Button;
        using A = MouseEvent::Action;
        const auto set = [&ev](B b, A a) {
            ev.button = b;
            ev.action = a;
        };

        if (bs & BUTTON1_PRESSED) set(B::Left, A::Press);
        else if (bs & BUTTON1_RELEASED) set(B::Left, A::Release);
        else if (bs & BUTTON1_DOUBLE_CLICKED) set(B::Left, A::DoubleClick);
        else if (bs & BUTTON1_CLICKED) set(B::Left, A::Press);
        else if (bs & BUTTON2_PRESSED) set(B::Middle, A::Press);
        else if (bs & BUTTON2_RELEASED) set(B::Middle, A::Release);
        else if (bs & BUTTON2_DOUBLE_CLICKED) set(B::Middle, A::DoubleClick);
        else if (bs & BUTTON2_CLICKED) set(B::Middle, A::Press);
        else if (bs & BUTTON3_PRESSED) set(B::Right, A::Press);
        else if (bs & BUTTON3_RELEASED) set(B::Right, A::Release);
        else if (bs & BUTTON3_DOUBLE_CLICKED) set(B::Right, A::DoubleClick);
        else if (bs & BUTTON3_CLICKED) set(B::Right, A::Press);
        // Wheel bits are version-dependent on both backends: guard them.
#ifdef BUTTON4_PRESSED
        else if (bs & BUTTON4_PRESSED) set(B::WheelUp, A::Press);
#endif
#ifdef BUTTON5_PRESSED
        else if (bs & BUTTON5_PRESSED) set(B::WheelDown, A::Press);
#endif
        else if (bs & REPORT_MOUSE_POSITION) set(B::None, A::Move);
        else return std::nullopt;

        return Event{ev};
    }

    // ------------------------------------------------------------- state

    int cols_ = 80;
    int rows_ = 24;
    std::optional<Point> cursor_;
    Point last_mouse_{0, 0};  // see translate_mouse(): wheel events lack one

    std::map<std::pair<Color, Color>, short> pairs_;
    short next_pair_ = 1;
    int max_pairs_ = 1;
    int colors_available_ = 0;
    bool has_default_colors_ = false;
    bool pair_exhaustion_ = false;

    std::vector<cchar_t> cells_;  // draw_run scratch, reused across frames
};

}  // namespace

std::unique_ptr<TerminalIO> make_curses_terminal() {
    return std::make_unique<CursesTerminal>();
}

}  // namespace modcurses
