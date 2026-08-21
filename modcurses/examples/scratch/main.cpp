//
// M1 acceptance program: paints styled text, survives resize, and reports
// every key it receives (including Ctrl- and Alt- modifiers) and every mouse
// event. There is no widget tree and no event loop yet - those are M2 - so
// this drives TerminalIO, ScreenBuffer and Canvas directly, which is exactly
// the surface M1 is supposed to deliver.
//
// Run it from Windows Terminal, not from an IDE's emulated console: PDCurses
// rejects piped stdin outright.
//
#include <chrono>
#include <cstdio>
#include <deque>
#include <string>
#include <variant>
#include <vector>

#include "modcurses/render.hpp"
#include "modcurses/terminal.hpp"
#include "modcurses/utf8.hpp"

using namespace modcurses;

namespace {

std::string describe(const KeyEvent& k) {
    std::string s;
    if (k.mods.ctrl) s += "Ctrl+";
    if (k.mods.alt) s += "Alt+";
    if (k.mods.shift) s += "Shift+";
    if (k.key == Key::Char) {
        s += "'" + utf8_encode(k.text) + "'";
        s += " (U+" + [](char32_t c) {
            static const char* hex = "0123456789ABCDEF";
            std::string h;
            for (int shift = 16; shift >= 0; shift -= 4) {
                const int nib = static_cast<int>((c >> shift) & 0xFu);
                if (!h.empty() || nib != 0 || shift == 0) h += hex[nib];
            }
            return h.size() < 4 ? std::string(4 - h.size(), '0') + h : h;
        }(k.text) + ")";
    } else {
        s += to_string(k.key);
    }
    return s;
}

std::string describe(const MouseEvent& m) {
    static const char* buttons[] = {"None", "Left", "Middle", "Right", "WheelUp", "WheelDown"};
    static const char* actions[] = {"Press", "Release", "DoubleClick", "Move"};
    std::string s;
    if (m.mods.ctrl) s += "Ctrl+";
    if (m.mods.alt) s += "Alt+";
    if (m.mods.shift) s += "Shift+";
    s += buttons[static_cast<int>(m.button)];
    s += " ";
    s += actions[static_cast<int>(m.action)];
    s += " at " + std::to_string(m.pos.x) + "," + std::to_string(m.pos.y);
    return s;
}

const Color kAllColors[] = {
    Color::Black, Color::Red, Color::Green, Color::Yellow,
    Color::Blue, Color::Magenta, Color::Cyan, Color::White,
    Color::BrightBlack, Color::BrightRed, Color::BrightGreen, Color::BrightYellow,
    Color::BrightBlue, Color::BrightMagenta, Color::BrightCyan, Color::BrightWhite,
};

void paint(Canvas& c, const std::deque<std::string>& log, Size sz, int frames, int resizes,
           int ticks) {
    const Style title = Style{}.with_fg(Color::BrightWhite).with_bg(Color::Blue).with(Trait::Bold);
    const Style dim = fg(Color::BrightBlack);
    const Style key = fg(Color::BrightCyan);

    c.fill(Glyph{U' ', {}});

    // --- title bar -------------------------------------------------------
    c.fill(Rect{{0, 0}, {sz.width, 1}}, Glyph{U' ', title});
    c.print({1, 0}, "modcurses M1 scratch", title);
    const std::string dims = std::to_string(sz.width) + "x" + std::to_string(sz.height) +
                             "  frames " + std::to_string(frames) + "  resizes " +
                             std::to_string(resizes) + "  ticks " + std::to_string(ticks) + " ";
    if (sz.width > static_cast<int>(dims.size()) + 24)
        c.print({sz.width - static_cast<int>(dims.size()), 0}, dims, title);

    int y = 2;
    const auto line = [&](int indent, std::string_view text, Style s = {}) {
        if (y < sz.height) c.print({indent, y}, text, s);
        ++y;
    };

    // --- colour pairs ----------------------------------------------------
    line(1, "colour pairs (lazily allocated in the backend):", dim);
    if (y < sz.height) {
        int x = 1;
        for (Color col : kAllColors) {
            c.print({x, y}, "  ", Style{}.with_bg(col));
            x += 2;
        }
        x += 2;
        for (Color col : kAllColors) {
            c.put({x, y}, U'#', fg(col));
            ++x;
        }
    }
    y += 2;

    // --- traits ----------------------------------------------------------
    line(1, "traits:", dim);
    if (y < sz.height) {
        int x = 1;
        const std::pair<const char*, Trait> traits[] = {
            {"bold", Trait::Bold},   {"underline", Trait::Underline}, {"reverse", Trait::Reverse},
            {"dim", Trait::Dim},     {"blink", Trait::Blink},         {"italic", Trait::Italic},
        };
        for (const auto& [name, t] : traits) {
            c.print({x, y}, name, Style{}.with_fg(Color::White).with(t));
            x += static_cast<int>(std::string_view{name}.size()) + 2;
        }
    }
    y += 2;

    // --- box drawing (the /utf-8 canary) ---------------------------------
    line(1, "box drawing (mangled here means /utf-8 is missing on MSVC):", dim);
    const BoxStyle styles[] = {BoxStyle::Light, BoxStyle::Heavy, BoxStyle::Double,
                               BoxStyle::Rounded, BoxStyle::Ascii};
    const char* names[] = {"Light", "Heavy", "Double", "Rounded", "Ascii"};
    int bx = 1;
    for (int i = 0; i < 5; ++i) {
        if (bx + 9 >= sz.width || y + 2 >= sz.height) break;
        c.draw_box(Rect{{bx, y}, {9, 3}}, fg(Color::BrightBlue), styles[i]);
        c.print({bx + 1, y + 1}, names[i], fg(Color::White));
        bx += 10;
    }
    y += 4;

    // --- key log ---------------------------------------------------------
    line(1, "events (newest first):", dim);
    for (const auto& entry : log) {
        if (y >= sz.height - 1) break;
        line(3, entry, key);
    }

    // --- footer ----------------------------------------------------------
    if (sz.height >= 2) {
        const Style footer = Style{}.with_fg(Color::Black).with_bg(Color::BrightBlack);
        c.fill(Rect{{0, sz.height - 1}, {sz.width, 1}}, Glyph{U' ', footer});
        c.print({1, sz.height - 1},
                "q or Ctrl-C quits  |  resize the window  |  try Ctrl-, Alt-, Esc, F-keys, mouse",
                footer);
    }
}

}  // namespace

int main() {
    try {
        auto term = make_curses_terminal();
        ScreenBuffer buf{term->size()};

        std::deque<std::string> log;
        const auto push = [&log](std::string s) {
            log.push_front(std::move(s));
            if (log.size() > 8) log.pop_back();
        };
        push("(waiting for input)");

        int frames = 0, resizes = 0, ticks = 0;
        bool quit = false;

        while (!quit) {
            const Size sz = term->size();
            if (sz != buf.size()) buf.resize(sz);

            const Rect all{{0, 0}, sz};
            Canvas c{buf, all, all};
            paint(c, log, sz, ++frames, resizes, ticks);
            buf.flush_to(*term);
            term->set_cursor(std::nullopt);

            // A finite timeout exercises the path the event loop's timer heap
            // will use in M2.
            auto ev = term->poll_event(std::chrono::milliseconds{500});
            if (!ev) {
                ++ticks;
                continue;
            }

            if (const auto* k = std::get_if<KeyEvent>(&*ev)) {
                push("key   " + describe(*k));
                const bool is_q = k->key == Key::Char && (k->text == U'q' || k->text == U'Q') &&
                                  !k->mods.ctrl && !k->mods.alt;
                const bool is_ctrl_c =
                    k->key == Key::Char && k->mods.ctrl && (k->text == U'c' || k->text == U'q');
                // Escape deliberately does NOT quit: it has to stay visible in
                // the log so the ESC/Alt disambiguation can be checked.
                if (is_q || is_ctrl_c) quit = true;
            } else if (const auto* m = std::get_if<MouseEvent>(&*ev)) {
                push("mouse " + describe(*m));
            } else if (const auto* r = std::get_if<ResizeEvent>(&*ev)) {
                ++resizes;
                buf.resize(r->size);
                push("resize to " + std::to_string(r->size.width) + "x" +
                     std::to_string(r->size.height));
            }
        }
        return 0;
    } catch (const TerminalError& e) {
        std::fprintf(stderr, "modcurses: %s\n", e.what());
        return 1;
    }
}
