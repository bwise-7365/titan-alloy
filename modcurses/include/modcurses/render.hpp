#pragma once
//
// modcurses/render.hpp - colour, style, glyphs, the screen buffer, Canvas.
//
// PUBLIC HEADER: no curses. Colour-pair allocation lives entirely inside the
// backend (lesson 5); nothing here knows that curses pairs exist.
//
#include <array>
#include <cstdint>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

#include "modcurses/core.hpp"

namespace modcurses {

class TerminalIO;  // terminal.hpp; only flush_to() needs it

// ------------------------------------------------------------------ colour

enum class Color : std::uint8_t {
    Default = 0,  // the terminal's own default fg/bg (curses -1)
    Black, Red, Green, Yellow, Blue, Magenta, Cyan, White,
    BrightBlack, BrightRed, BrightGreen, BrightYellow,
    BrightBlue, BrightMagenta, BrightCyan, BrightWhite,
};

inline constexpr int kColorCount = 17;  // Default + 16

[[nodiscard]] const char* to_string(Color c);

struct Rgb {
    std::uint8_t r = 0, g = 0, b = 0;
    constexpr auto operator<=>(const Rgb&) const = default;
};

enum class Trait : std::uint8_t { Bold = 0, Underline, Reverse, Dim, Blink, Italic };

// NOTE (deviation from the design sketch): traits are a uint8_t bitmask
// rather than std::bitset<8>. std::bitset has no operator<=>, so a defaulted
// three-way comparison on Style would be deleted - and Style must be ordered
// so it can key the backend's pair cache. The mask is also constexpr-friendly.
struct Style {
    Color fg = Color::Default;
    Color bg = Color::Default;
    std::uint8_t traits = 0;

    constexpr auto operator<=>(const Style&) const = default;

    [[nodiscard]] static constexpr std::uint8_t bit(Trait t) {
        return static_cast<std::uint8_t>(1u << static_cast<std::uint8_t>(t));
    }
    [[nodiscard]] constexpr bool has(Trait t) const { return (traits & bit(t)) != 0; }

    [[nodiscard]] constexpr Style with_fg(Color c) const {
        Style s = *this;
        s.fg = c;
        return s;
    }
    [[nodiscard]] constexpr Style with_bg(Color c) const {
        Style s = *this;
        s.bg = c;
        return s;
    }
    [[nodiscard]] constexpr Style with(Trait t) const {
        Style s = *this;
        s.traits = static_cast<std::uint8_t>(s.traits | bit(t));
        return s;
    }
    [[nodiscard]] constexpr Style without(Trait t) const {
        Style s = *this;
        s.traits = static_cast<std::uint8_t>(s.traits & ~bit(t));
        return s;
    }
};

[[nodiscard]] constexpr Style fg(Color c) { return Style{}.with_fg(c); }
[[nodiscard]] constexpr Style bg(Color c) { return Style{}.with_bg(c); }

struct Glyph {
    char32_t ch = U' ';
    Style style;
    constexpr auto operator<=>(const Glyph&) const = default;
};

// A screen cell holds exactly what a glyph does.
using Cell = Glyph;

// ----------------------------------------------------------------- palette

// Redefines the terminal's colour slots. Every call happens after initscr()
// (lesson 3: CPPurses called init_color at static-init time, curses returned
// ERR, and the palette silently never applied on any platform).
class Palette {
public:
    Palette() = default;
    explicit Palette(TerminalIO* term) : term_(term) {}

    bool set(Color slot, Rgb value);        // false if the terminal refused
    [[nodiscard]] bool can_redefine() const;

    // What this Palette last successfully set for a slot, if anything.
    // Deliberately not a query of the terminal: curses offers no portable way
    // to read a colour back, and inventing one that lies on some terminals
    // would be worse than admitting the limit.
    [[nodiscard]] std::optional<Rgb> get(Color slot) const;

    // Applies a built-in 16-colour palette. Returns false if unsupported or
    // if any slot was refused.
    bool apply_dawnbringer16();

    // The palette above as data, for callers who want to inspect or adapt it
    // rather than apply it wholesale.
    struct Entry {
        Color slot;
        Rgb rgb;
    };
    [[nodiscard]] static std::span<const Entry> dawnbringer16();

private:
    TerminalIO* term_ = nullptr;
    // Indexed by Color; only slots this Palette actually set are marked.
    std::array<std::optional<Rgb>, kColorCount> applied_{};
};

// ------------------------------------------------------------ screenbuffer

class ScreenBuffer {
public:
    ScreenBuffer() = default;
    explicit ScreenBuffer(Size s) { resize(s); }

    // Clears both buffers and forces the next flush to write every cell.
    void resize(Size s);

    [[nodiscard]] Size size() const { return size_; }
    [[nodiscard]] bool in_bounds(Point p) const {
        return p.x >= 0 && p.y >= 0 && p.x < size_.width && p.y < size_.height;
    }

    [[nodiscard]] Cell& back_at(Point p) { return back_[index(p)]; }
    [[nodiscard]] const Cell& back_at(Point p) const { return back_[index(p)]; }
    [[nodiscard]] const Cell& front_at(Point p) const { return front_[index(p)]; }

    void clear_back(Glyph g = {});

    // Makes the front buffer differ from anything, so the next flush repaints
    // the whole screen (used after a resize or an external screen corruption).
    void force_full_redraw();

    // The frame diff. Walks each row for runs of changed cells, splits runs at
    // style boundaries, and issues one draw_run per run. Returns the number of
    // runs written - the unit tests assert on it to prove the diff is minimal.
    int flush_to(TerminalIO& term);

private:
    [[nodiscard]] std::size_t index(Point p) const {
        return static_cast<std::size_t>(p.y) * static_cast<std::size_t>(size_.width) +
               static_cast<std::size_t>(p.x);
    }

    std::vector<Cell> front_, back_;
    Size size_;
    std::u32string run_;  // scratch, reused across flushes
};

// ------------------------------------------------------------------ canvas

enum class BoxStyle { Light, Heavy, Double, Rounded, Ascii };

// The only way a widget draws. Coordinates are LOCAL (0,0 is the widget's
// top-left) and every write is clipped to the widget's visible rect, so a
// widget cannot paint outside itself even by accident. Constructed by the
// renderer and handed to Widget::paint; widgets cannot make one themselves.
class Canvas {
public:
    Canvas(ScreenBuffer& buffer, Rect area, Rect clip)
        : buf_(&buffer), area_(area), clip_(area.intersect(clip)) {}

    [[nodiscard]] Size size() const { return area_.size; }

    void put(Point p, Glyph g);
    void put(Point p, char32_t c, Style s = {});
    void print(Point p, std::u32string_view text, Style s = {});
    void print(Point p, std::string_view utf8, Style s = {});  // decodes UTF-8
    void fill(Rect local, Glyph g);
    void fill(Glyph g);  // the whole widget
    void draw_box(Rect local, Style s = {}, BoxStyle bs = BoxStyle::Light);
    void draw_hline(Point local, int len, Style s = {}, BoxStyle bs = BoxStyle::Light);
    void draw_vline(Point local, int len, Style s = {}, BoxStyle bs = BoxStyle::Light);

    // A sub-canvas of this one, in local coordinates (for widgets that draw
    // an inner region, e.g. inside a border).
    [[nodiscard]] Canvas sub(Rect local) const;

private:
    [[nodiscard]] Point to_screen(Point local) const {
        return {area_.origin.x + local.x, area_.origin.y + local.y};
    }

    ScreenBuffer* buf_;
    Rect area_;  // absolute rect of the widget
    Rect clip_;  // absolute clip: area_ intersected with the ancestors' rects
};

// Box-drawing glyphs for a style, in the order:
// horizontal, vertical, top-left, top-right, bottom-left, bottom-right.
struct BoxChars {
    char32_t h, v, tl, tr, bl, br;
};
[[nodiscard]] BoxChars box_chars(BoxStyle bs);

}  // namespace modcurses
