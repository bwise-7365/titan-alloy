// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PALETTE_CONVERSION_HPP
#define PALETTE_CONVERSION_HPP

#include "palette/color.h"
#include "palette/types.h"

// The RYB<->sRGB bridge (DESIGN.md §1). The cube corner constants and the
// numerically-fit inverse are PORTED from ProfJski/ArtColors (RYB.cpp), not
// invented (CLAUDE.md / DESIGN.md §7). The inverse is approximate; round-tripping
// preserves hue closely but not exactly (see test_conversion.cpp tolerance).
namespace palette {

// Forward map: RYB cube coords [0,1] -> sRGB [0,1] (Gossett-Chen trilinear).
// Throws std::out_of_range if any component of `v` is outside [0,1].
Srgb rybToSrgb(const Ryb& v);

// Approximate inverse: sRGB [0,1] -> RYB cube coords (ArtColors fitted cube).
// Throws std::out_of_range if any channel of `c` is outside [0,1].
Ryb srgbToRyb(const Srgb& c);

// HSV-on-the-RYB-wheel <-> RYB cube. Primaries Red=0, Yellow=120, Blue=240 deg.
// `hsvRybToRyb` normalizes hue into [0,360); throws if sat/val outside [0,1].
Ryb    hsvRybToRyb(const HsvRyb& h);
HsvRyb rybToHsvRyb(const Ryb& v);

// Convenience: the hue angle of an sRGB color on the RYB wheel, in [0,360).
double hueOfSrgb(const Srgb& c);

// Convenience: realize an HSV-on-RYB color directly as sRGB.
Srgb srgbAtHsvRyb(const HsvRyb& h);

} // namespace palette

#endif // PALETTE_CONVERSION_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
