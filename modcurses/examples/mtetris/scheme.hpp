#pragma once
//
// MTetris - the run-time colour schemes, ported from FTetris' applyColorScheme.
//
// FTetris picked colours from a 22-entry palette of exact RGB values. A
// terminal has sixteen slots, not a 24-bit colour space - but curses can
// REDEFINE those slots, so the schemes come across exactly rather than
// approximated. Where the terminal refuses (can_redefine() == false), each
// colour falls back to the nearest of the sixteen standard ANSI colours.
//
#include <array>

#include "modcurses/render.hpp"
#include "shape.hpp"

namespace mtetris {

using modcurses::Color;
using modcurses::Rgb;

// Menu order is FTetris', including the fact that Black - not the first
// entry - is the factory default.
enum class Background { Beige = 0, Black = 1, White = 2 };
enum class Pieces { GameBoy = 0, Gerasimov = 1, Sega = 2, SovietMindGame = 3, TetrisCompany = 4 };

inline constexpr int kBackgroundCount = 3;
inline constexpr int kPiecesCount = 5;

[[nodiscard]] const char* name_of(Background b);
[[nodiscard]] const char* name_of(Pieces p);

// The eleven colours FTetris required: index 0 is "no shape", 1..7 are the
// tetrominoes in TCode order, then the two column-stripe backgrounds.
struct Scheme {
    std::array<Rgb, 8> piece{};  // [0] unused, [1..7] = I J L O S T Z
    Rgb even_column{};
    Rgb odd_column{};
};

[[nodiscard]] Scheme make_scheme(Background bg, Pieces pc);

// ---------------------------------------------------------------- slots

// A scheme's nine colours resolved onto NINE DISTINCT terminal colour slots.
//
// This is what makes a scheme change visible, and it is the whole lesson of
// the bug that produced it. The first version pinned every scheme to the same
// seven slots and changed only the RGB behind them, via the palette. That
// works on a terminal that honours colour redefinition and is completely
// invisible on one that does not - and there is no reliable way to ask, since
// PDCurses' can_change_color() just answers "yes, this is Windows NT".
//
// Choosing DIFFERENT SLOTS per scheme means the cell attributes themselves
// change, so the board repaints in different colours on any terminal at all.
// Redefining the RGB on top of that is then a bonus, not the mechanism.
struct SlotMap {
    std::array<Color, 8> piece{};  // [0] unused; [1..7] = I J L O S T Z
    Color even_column = Color::Black;
    Color odd_column = Color::BrightBlack;
};

[[nodiscard]] SlotMap assign_slots(const Scheme& s);

// The nearest of the sixteen standard ANSI colours to an arbitrary RGB.
[[nodiscard]] Color nearest_ansi(Rgb value);

}  // namespace mtetris
