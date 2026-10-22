// Copyright Ben Paul Wise. All Rights Reserved.
#include "palette/legibility.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace palette {

namespace {

// Per-channel sRGB -> linear transform (WCAG 2.x).
constexpr double kSrgbLinearThreshold = 0.04045;
constexpr double kSrgbLinearSlope     = 12.92;
constexpr double kSrgbGammaOffset     = 0.055;
constexpr double kSrgbGammaScale      = 1.055;
constexpr double kSrgbGammaExponent   = 2.4;

// CIE relative-luminance coefficients (Rec. 709).
constexpr double kLumR = 0.2126;
constexpr double kLumG = 0.7152;
constexpr double kLumB = 0.0722;

// WCAG contrast-ratio offset, applied to both luminances.
constexpr double kContrastOffset = 0.05;

// Per-channel sRGB -> linear transform (WCAG 2.x). `c` must be in [0, 1].
double linearizeChannel(double c) {
    if (c < 0.0 || c > 1.0) {
        throw std::out_of_range("sRGB channel outside [0, 1]");
    }
    if (c <= kSrgbLinearThreshold) {
        return c / kSrgbLinearSlope;
    }
    return std::pow((c + kSrgbGammaOffset) / kSrgbGammaScale, kSrgbGammaExponent);
}

} // namespace

double relativeLuminance(const Srgb& c) {
    const double r = linearizeChannel(c.r);
    const double g = linearizeChannel(c.g);
    const double b = linearizeChannel(c.b);
    return kLumR * r + kLumG * g + kLumB * b;
}

double contrastRatio(const Srgb& a, const Srgb& b) {
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    const double hi = std::max(la, lb);
    const double lo = std::min(la, lb);
    return (hi + kContrastOffset) / (lo + kContrastOffset);
}

} // namespace palette
// Copyright Ben Paul Wise. All Rights Reserved.
