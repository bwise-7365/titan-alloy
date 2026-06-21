// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PALETTE_TYPES_HPP
#define PALETTE_TYPES_HPP

#include <vector>

#include "palette/color.h"

// Value types for the harmony layer and the mode results (DESIGN.md §6).
// All are small immutable aggregates. `Srgb` lives in color.hpp.
namespace palette {

// RYB cube coordinates, each in [0, 1] (ArtColors convention: 0,0,0 = black,
// 1,1,1 = white). This is the cube the Gossett-Chen/ArtColors trilinear maps.
struct Ryb {
    double r;
    double y;
    double b;
};

// Hue measured on the RYB wheel (primaries Red=0, Yellow=120, Blue=240 degrees),
// with HSV-style saturation/value over the RYB channels.
struct HsvRyb {
    double hueDeg; // [0, 360)
    double sat;    // [0, 1]
    double val;    // [0, 1]
};

// Harmony templates, reasoned angularly on the RYB wheel (DESIGN.md §1).
enum class Harmony {
    Complement,
    SplitComplement,
    Triad,
    Analogous,
    Tetrad
};

// Tunable inputs to the modes. Defaults follow DESIGN.md; the threshold values
// marked "provisional" cover open TODO(decide) items (§8) and are surfaced here
// rather than hidden, so the caller can override them.
struct Constraints {
    double minPieceBgContrast      = 3.0;  // figure-ground floor (§2)
    double splitComplementAlphaDeg = 30.0; // split-complement arm offset (§1)
    bool   allowPieceLuminanceSpread = true; // §8 #2 (provisional default)

    // Least-bad-accommodation detection thresholds (§5, §8 #4 — provisional).
    double hueTooCloseDeg      = 30.0; // pieces "nearly same color" hue gap
    double smallLuminanceDelta = 0.10; // |L1-L2| considered "similar luminance"
    double midbandLuminanceTol = 0.05; // half-width of the mid-luminance band
};

// Non-fatal diagnostics emitted alongside a valid result (DESIGN.md §5). These
// are NOT errors: the result honors legibility and relaxes harmony instead.
enum class Warning {
    PiecesHueTooClose,
    PiecesLuminanceMidband,
    NoHarmonicBackground
};

struct Diagnostics {
    double  contrastBgP1            = 0.0;
    double  contrastBgP2            = 0.0;
    double  contrastP1P2            = 0.0;
    Harmony templateUsed           = Harmony::SplitComplement;
    double  backgroundTargetLuminance = 0.0;
};

struct Palette {
    Srgb background{};
    Srgb piece1{};
    Srgb piece2{};
    std::vector<Warning> warnings{};
    Diagnostics diagnostics{};
};

} // namespace palette

#endif // PALETTE_TYPES_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
