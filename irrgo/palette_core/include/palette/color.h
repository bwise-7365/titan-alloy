// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PALETTE_COLOR_HPP
#define PALETTE_COLOR_HPP

namespace palette {

// Small immutable value type for a rendered sRGB color.
// Each channel is a normalized intensity in [0, 1] (NOT 0..255).
// This is the only color representation the legibility/minimax layer needs;
// RYB types live in the (not-yet-scaffolded) harmony layer.
struct Srgb {
    double r;
    double g;
    double b;
};

} // namespace palette

#endif // PALETTE_COLOR_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
