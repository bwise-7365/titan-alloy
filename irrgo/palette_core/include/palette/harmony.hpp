// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PALETTE_HARMONY_HPP
#define PALETTE_HARMONY_HPP

#include "palette/types.hpp"

// Pure angular harmony helpers on the RYB wheel (DESIGN.md §1). All angles are
// degrees; inputs may be any real, outputs are normalized to [0,360).
namespace palette {

struct HuePair {
    double first;
    double second;
};

// Wrap an angle into [0,360).
double normalizeHue(double deg);

// Smallest unsigned angular separation between two hues, in [0,180].
double hueDistance(double a, double b);

// The hue halfway between two hues along the shorter arc, in [0,360).
double circularMidpoint(double a, double b);

double complementHue(double h); // h + 180

// The two PIECE hues for Mode 1, anchored on the background hue `bgHue`
// (DESIGN.md §4 Mode 1). For Complement the two hues coincide (pieces are then
// distinguished by luminance); the other templates give two distinct hues that
// also avoid the board hue.
HuePair pieceHuesForBackground(double bgHue, Harmony t, double alphaDeg);

// The partner PIECE hue for Mode 2, given the first piece's hue `pieceHue` and a
// template (DESIGN.md §4 Mode 2).
double partnerHueForPiece(double pieceHue, Harmony t, double alphaDeg);

} // namespace palette

#endif // PALETTE_HARMONY_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
