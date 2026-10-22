// Copyright Ben Paul Wise. All Rights Reserved.

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

#include "modes_internal.h"
#include "palette/conversion.h"
#include "palette/harmony.h"
#include "palette/legibility.h"
#include "palette/minimax.h"

namespace palette {
namespace detail {

namespace {

// Binary-search the HSV-on-RYB value that lands closest to `targetL` at a fixed
// hue/saturation. Luminance rises monotonically with value here (value scales
// the RYB coordinates from black up), so a bisection converges.
Srgb valueForLuminance(double hueDeg, double sat, double targetL) {
    double lo = 0.0;
    double hi = 1.0;
    for (int k = 0; k < 48; ++k) {
        const double mid = (lo + hi) * 0.5;
        const double l = relativeLuminance(srgbAtHsvRyb(HsvRyb{hueDeg, sat, mid}));
        if (l < targetL) {
            lo = mid;
        } else {
            hi = mid;
        }
    }
    return srgbAtHsvRyb(HsvRyb{hueDeg, sat, (lo + hi) * 0.5});
}

} // namespace

Srgb realizeAtLuminance(double hueDeg, double targetL, double maxSat) {
    const double target = std::clamp(targetL, 0.0, 1.0);
    const double cap = std::clamp(maxSat, 0.0, 1.0);
    constexpr int kSatSteps = 24;

    // Scan saturation from the cap downward; the highest saturation whose full
    // value (brightest) still reaches the target is the most chromatic color
    // that can hit it. Lower saturation can always reach higher luminance, so
    // sat = 0 (gray, reaches the whole range) is the guaranteed fallback.
    for (int i = kSatSteps; i >= 0; --i) {
        const double sat = cap * static_cast<double>(i) / kSatSteps;
        const double brightest =
            relativeLuminance(srgbAtHsvRyb(HsvRyb{hueDeg, sat, 1.0}));
        if (target <= brightest + 1e-9) {
            return valueForLuminance(hueDeg, sat, target);
        }
    }
    return valueForLuminance(hueDeg, 0.0, target); // unreachable; gray fallback
}

Diagnostics makeDiagnostics(const Srgb& background, const Srgb& p1,
                            const Srgb& p2, Harmony templateUsed,
                            double backgroundTargetLuminance) {
    Diagnostics d;
    d.contrastBgP1 = contrastRatio(background, p1);
    d.contrastBgP2 = contrastRatio(background, p2);
    d.contrastP1P2 = contrastRatio(p1, p2);
    d.templateUsed = templateUsed;
    d.backgroundTargetLuminance = backgroundTargetLuminance;
    return d;
}

Palette completeFromPieces(const Srgb& p1, const Srgb& p2, Harmony t,
                           const Constraints& c) {
    const double l1 = relativeLuminance(p1);
    const double l2 = relativeLuminance(p2);
    double targetBgL = minimaxBackgroundLuminance(l1, l2);

    // The minimax may pick a pure extreme (L = 0 or 1) when it maximizes
    // figure-ground contrast. A pure black/white background, however, erases any
    // background hue. When an extreme is chosen, back off to the least-extreme
    // luminance on that same side that still meets the contrast floor against
    // both pieces, leaving headroom for chroma. (Clamped: if even the extreme is
    // the only luminance that meets the floor, it stays.)
    {
        const double floor = c.minPieceBgContrast;
        const double shiftedLo = std::min(l1, l2) + 0.05; // darker piece
        const double shiftedHi = std::max(l1, l2) + 0.05; // lighter piece
        if (targetBgL <= 0.0) {
            // Lightest dark-side background still at the floor (binds darker piece).
            targetBgL = std::clamp(shiftedLo / floor - 0.05, 0.0, 1.0);
        } else if (targetBgL >= 1.0) {
            // Darkest light-side background still at the floor (binds lighter piece).
            targetBgL = std::clamp(floor * shiftedHi - 0.05, 0.0, 1.0);
        }
    }

    const double h1 = hueOfSrgb(p1);
    const double h2 = hueOfSrgb(p2);
    const double gap = hueDistance(h1, h2);
    const double alpha = c.splitComplementAlphaDeg;

    // Recognize a harmonic separation, else neutralize the background.
    constexpr double kRecogTol = 18.0;     // provisional (DESIGN §8 #4)
    constexpr double kTriadDeg = 120.0;    // triadic separation
    constexpr double kComplementDeg = 180.0;  // complementary separation
    const auto matches = [](double value, double target) {
        return std::fabs(value - target) <= kRecogTol;
    };
    const bool recognized = matches(gap, 0.0) || matches(gap, alpha) ||
                            matches(gap, 2.0 * alpha) || matches(gap, kTriadDeg) ||
                            matches(gap, kComplementDeg);

    std::vector<Warning> warnings;
    Srgb background;
    if (!recognized) {
        background = realizeAtLuminance(0.0, targetBgL, 0.0); // neutral gray
        warnings.push_back(Warning::NoHarmonicBackground);
    } else if (matches(gap, kComplementDeg)) {
        background = realizeAtLuminance(0.0, targetBgL, 0.0); // complements: neutral
    } else {
        const double bgHue = complementHue(circularMidpoint(h1, h2));
        background = realizeAtLuminance(bgHue, targetBgL, 0.20); // low chroma
    }

    const double deltaL = std::fabs(l1 - l2);
    if (gap < c.hueTooCloseDeg && deltaL < c.smallLuminanceDelta) {
        warnings.push_back(Warning::PiecesHueTooClose);
    }
    const double midbandL = std::sqrt(0.0525) - 0.05; // L at the geometric mean
    if (std::fabs(l1 - midbandL) < c.midbandLuminanceTol &&
        std::fabs(l2 - midbandL) < c.midbandLuminanceTol) {
        warnings.push_back(Warning::PiecesLuminanceMidband);
    }

    Palette pal;
    pal.background = background;
    pal.piece1 = p1;
    pal.piece2 = p2;
    pal.warnings = std::move(warnings);
    pal.diagnostics = makeDiagnostics(background, p1, p2, t, targetBgL);
    return pal;
}

} // namespace detail
} // namespace palette
// Copyright Ben Paul Wise. All Rights Reserved.
