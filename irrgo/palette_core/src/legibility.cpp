// Copyright Ben Paul Wise. All Rights Reserved.
#include "palette/legibility.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace palette {

namespace {

// Per-channel sRGB -> linear transform (WCAG 2.x). `c` must be in [0, 1].
double linearizeChannel(double c) {
    if (c < 0.0 || c > 1.0) {
        throw std::out_of_range("sRGB channel outside [0, 1]");
    }
    if (c <= 0.04045) {
        return c / 12.92;
    }
    return std::pow((c + 0.055) / 1.055, 2.4);
}

} // namespace

double relativeLuminance(const Srgb& c) {
    const double r = linearizeChannel(c.r);
    const double g = linearizeChannel(c.g);
    const double b = linearizeChannel(c.b);
    return 0.2126 * r + 0.7152 * g + 0.0722 * b;
}

double contrastRatio(const Srgb& a, const Srgb& b) {
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    const double hi = std::max(la, lb);
    const double lo = std::min(la, lb);
    return (hi + 0.05) / (lo + 0.05);
}

} // namespace palette
// Copyright Ben Paul Wise. All Rights Reserved.
