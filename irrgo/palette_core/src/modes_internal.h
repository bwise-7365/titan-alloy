// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PALETTE_MODES_INTERNAL_HPP
#define PALETTE_MODES_INTERNAL_HPP

#include "palette/color.h"
#include "palette/types.h"

// Shared helpers used by two or more modes. Internal to palette_core (not
// installed). Definitions live in modes_internal.cpp.
namespace palette {
namespace detail {

// Realize an sRGB color at hue `hueDeg` whose relative luminance is as close as
// possible to `targetL`, using the MOST chroma that still reaches that
// luminance but never exceeding `maxSat`. Drops chroma toward neutral when the
// hue cannot reach the target (DESIGN.md §3). `maxSat == 0` yields a pure gray.
Srgb realizeAtLuminance(double hueDeg, double targetL, double maxSat);

// Fill the contrast fields + template + background target luminance.
Diagnostics makeDiagnostics(const Srgb& background, const Srgb& p1,
                            const Srgb& p2, Harmony templateUsed,
                            double backgroundTargetLuminance);

// The Mode-3 core, shared by Mode 2 and Mode 3: given two fixed piece colors,
// choose the background (minimax luminance + harmonic-or-neutral hue) and emit
// least-bad-accommodation warnings (DESIGN.md §3/§5).
Palette completeFromPieces(const Srgb& p1, const Srgb& p2, Harmony t,
                           const Constraints& c);

} // namespace detail
} // namespace palette

#endif // PALETTE_MODES_INTERNAL_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
