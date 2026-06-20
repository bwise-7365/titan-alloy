// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PALETTE_LEGIBILITY_HPP
#define PALETTE_LEGIBILITY_HPP

#include "palette/color.hpp"

namespace palette {

// WCAG 2.x relative luminance of an sRGB color, in [0, 1].
// See DESIGN.md §1 (Legibility layer). Throws std::out_of_range if any channel
// of `c` falls outside [0, 1] (no silent clamping — see CLAUDE.md).
double relativeLuminance(const Srgb& c);

// WCAG 2.x contrast ratio between two sRGB colors, in [1, 21].
// (max(L_a,L_b)+0.05) / (min(L_a,L_b)+0.05). Symmetric in its arguments.
// Throws std::out_of_range if either color has an out-of-range channel.
double contrastRatio(const Srgb& a, const Srgb& b);

} // namespace palette

#endif // PALETTE_LEGIBILITY_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
