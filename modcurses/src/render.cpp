#include "modcurses/render.hpp"

#include "modcurses/terminal.hpp"
#include "modcurses/utf8.hpp"

namespace modcurses {

const char* to_string(Color c) {
    switch (c) {
        case Color::Default: return "Default";
        case Color::Black: return "Black";
        case Color::Red: return "Red";
        case Color::Green: return "Green";
        case Color::Yellow: return "Yellow";
        case Color::Blue: return "Blue";
        case Color::Magenta: return "Magenta";
        case Color::Cyan: return "Cyan";
        case Color::White: return "White";
        case Color::BrightBlack: return "BrightBlack";
        case Color::BrightRed: return "BrightRed";
        case Color::BrightGreen: return "BrightGreen";
        case Color::BrightYellow: return "BrightYellow";
        case Color::BrightBlue: return "BrightBlue";
        case Color::BrightMagenta: return "BrightMagenta";
        case Color::BrightCyan: return "BrightCyan";
        case Color::BrightWhite: return "BrightWhite";
    }
    return "?";
}

// ----------------------------------------------------------------- palette

bool Palette::can_redefine() const { return term_ != nullptr && term_->can_define_colors(); }

bool Palette::set(Color slot, Rgb value) {
    if (term_ == nullptr) return false;
    if (slot == Color::Default) return false;  // not a real slot
    if (!term_->define_color(slot, value)) return false;
    applied_[static_cast<std::size_t>(slot)] = value;
    return true;
}

std::optional<Rgb> Palette::get(Color slot) const {
    const auto i = static_cast<std::size_t>(slot);
    return i < applied_.size() ? applied_[i] : std::nullopt;
}

std::span<const Palette::Entry> Palette::dawnbringer16() {
    // DawnBringer 16, hue-matched onto the ANSI slots. DB16 has no dark cyan
    // and no bright magenta, so those two are derived (darkened / lightened)
    // from its cyan and purple rather than invented.
    static constexpr Entry kDb16[] = {
        {Color::Black,         {0x14, 0x0c, 0x1c}},
        {Color::Red,           {0xd0, 0x46, 0x48}},
        {Color::Green,         {0x34, 0x65, 0x24}},
        {Color::Yellow,        {0x85, 0x4c, 0x30}},  // DB16 brown
        {Color::Blue,          {0x30, 0x34, 0x6d}},
        {Color::Magenta,       {0x44, 0x24, 0x34}},  // DB16 dark purple
        {Color::Cyan,          {0x3a, 0x6b, 0x70}},  // derived: darkened DB16 cyan
        {Color::White,         {0x75, 0x71, 0x61}},
        {Color::BrightBlack,   {0x4e, 0x4a, 0x4e}},
        {Color::BrightRed,     {0xd2, 0xaa, 0x99}},
        {Color::BrightGreen,   {0x6d, 0xaa, 0x2c}},
        {Color::BrightYellow,  {0xda, 0xd4, 0x5e}},
        {Color::BrightBlue,    {0x59, 0x7d, 0xce}},
        {Color::BrightMagenta, {0x8c, 0x5c, 0x84}},  // derived: lightened DB16 purple
        {Color::BrightCyan,    {0x6d, 0xc2, 0xca}},
        {Color::BrightWhite,   {0xde, 0xee, 0xd6}},
    };
    return std::span<const Entry>{kDb16};
}

bool Palette::apply_dawnbringer16() {
    if (!can_redefine()) return false;
    bool all_ok = true;
    // Every slot is attempted even after one fails, so a terminal that
    // refuses a single colour still gets the other fifteen.
    for (const Entry& e : dawnbringer16()) all_ok = set(e.slot, e.rgb) && all_ok;
    return all_ok;
}

// ------------------------------------------------------------ screenbuffer

namespace {
// A codepoint no real cell can hold, used to poison the front buffer so the
// next diff considers every cell changed.
constexpr char32_t kNeverDrawn = static_cast<char32_t>(0xFFFFFFFFu);
}  // namespace

void ScreenBuffer::resize(Size s) {
    if (s.width < 0) s.width = 0;
    if (s.height < 0) s.height = 0;
    size_ = s;
    const auto n = static_cast<std::size_t>(s.width) * static_cast<std::size_t>(s.height);
    back_.assign(n, Cell{});
    front_.assign(n, Cell{});
    force_full_redraw();
}

void ScreenBuffer::clear_back(Glyph g) { back_.assign(back_.size(), g); }

void ScreenBuffer::force_full_redraw() {
    for (auto& c : front_) c.ch = kNeverDrawn;
}

int ScreenBuffer::flush_to(TerminalIO& term) {
    int runs = 0;
    for (int y = 0; y < size_.height; ++y) {
        int x = 0;
        while (x < size_.width) {
            const std::size_t i = index({x, y});
            if (back_[i] == front_[i]) {
                ++x;
                continue;
            }
            // Start a run at the first changed cell; it ends at the first cell
            // that is either unchanged or styled differently.
            const Style style = back_[i].style;
            const int start = x;
            run_.clear();
            while (x < size_.width) {
                const std::size_t j = index({x, y});
                if (back_[j] == front_[j] || back_[j].style != style) break;
                run_.push_back(back_[j].ch);
                front_[j] = back_[j];
                ++x;
            }
            term.draw_run({start, y}, run_, style);
            ++runs;
        }
    }
    term.flush();
    return runs;
}

// ------------------------------------------------------------------ canvas

BoxChars box_chars(BoxStyle bs) {
    switch (bs) {
        case BoxStyle::Light:   return {U'─', U'│', U'┌', U'┐', U'└', U'┘'};
        case BoxStyle::Heavy:   return {U'━', U'┃', U'┏', U'┓', U'┗', U'┛'};
        case BoxStyle::Double:  return {U'═', U'║', U'╔', U'╗', U'╚', U'╝'};
        case BoxStyle::Rounded: return {U'─', U'│', U'╭', U'╮', U'╰', U'╯'};
        case BoxStyle::Ascii:   return {U'-', U'|', U'+', U'+', U'+', U'+'};
    }
    return {U'-', U'|', U'+', U'+', U'+', U'+'};
}

void Canvas::put(Point p, Glyph g) {
    const Point s = to_screen(p);
    if (!clip_.contains(s)) return;      // outside the widget or an ancestor
    if (!buf_->in_bounds(s)) return;     // outside the screen entirely
    buf_->back_at(s) = g;
}

void Canvas::put(Point p, char32_t c, Style s) { put(p, Glyph{c, s}); }

void Canvas::print(Point p, std::u32string_view text, Style s) {
    int x = p.x;
    for (char32_t c : text) {
        put({x, p.y}, Glyph{c, s});
        x += col_width(c);
    }
}

void Canvas::print(Point p, std::string_view utf8, Style s) {
    print(p, std::u32string_view{utf8_decode(utf8)}, s);
}

void Canvas::fill(Rect local, Glyph g) {
    if (local.empty()) return;
    for (int y = local.top(); y < local.bottom(); ++y)
        for (int x = local.left(); x < local.right(); ++x) put({x, y}, g);
}

void Canvas::fill(Glyph g) { fill(Rect{{0, 0}, area_.size}, g); }

void Canvas::draw_hline(Point local, int len, Style s, BoxStyle bs) {
    const char32_t c = box_chars(bs).h;
    for (int i = 0; i < len; ++i) put({local.x + i, local.y}, c, s);
}

void Canvas::draw_vline(Point local, int len, Style s, BoxStyle bs) {
    const char32_t c = box_chars(bs).v;
    for (int i = 0; i < len; ++i) put({local.x, local.y + i}, c, s);
}

void Canvas::draw_box(Rect r, Style s, BoxStyle bs) {
    if (r.empty()) return;
    const BoxChars b = box_chars(bs);
    const int x0 = r.left(), y0 = r.top();
    const int x1 = r.right() - 1, y1 = r.bottom() - 1;

    // Degenerate boxes render as the line they actually are.
    if (r.size.width == 1 && r.size.height == 1) {
        put({x0, y0}, b.v, s);
        return;
    }
    if (r.size.height == 1) {
        draw_hline({x0, y0}, r.size.width, s, bs);
        return;
    }
    if (r.size.width == 1) {
        draw_vline({x0, y0}, r.size.height, s, bs);
        return;
    }

    put({x0, y0}, b.tl, s);
    put({x1, y0}, b.tr, s);
    put({x0, y1}, b.bl, s);
    put({x1, y1}, b.br, s);
    for (int x = x0 + 1; x < x1; ++x) {
        put({x, y0}, b.h, s);
        put({x, y1}, b.h, s);
    }
    for (int y = y0 + 1; y < y1; ++y) {
        put({x0, y}, b.v, s);
        put({x1, y}, b.v, s);
    }
}

Canvas Canvas::sub(Rect local) const {
    const Rect absolute{to_screen(local.origin), local.size};
    return Canvas{*buf_, absolute, clip_};
}

}  // namespace modcurses
