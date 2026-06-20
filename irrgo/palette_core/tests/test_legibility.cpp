// Copyright Ben Paul Wise. All Rights Reserved.
#include "palette/legibility.hpp"
#include "test_support.hpp"

using palette::Srgb;
using palette::contrastRatio;
using palette::relativeLuminance;

namespace {

const Srgb kBlack{0.0, 0.0, 0.0};
const Srgb kWhite{1.0, 1.0, 1.0};
const Srgb kMidGray{0.5, 0.5, 0.5};

void testRelativeLuminanceAnchors() {
    CHECK_NEAR(relativeLuminance(kBlack), 0.0, 1e-12);
    CHECK_NEAR(relativeLuminance(kWhite), 1.0, 1e-12);

    // Mid sRGB 0.5 linearizes to ((0.5+0.055)/1.055)^2.4 ~= 0.21404 per channel.
    CHECK_NEAR(relativeLuminance(kMidGray), 0.21404, 1e-4);

    // Per-channel weights: a pure-green primary is far brighter than pure blue.
    const double lr = relativeLuminance(Srgb{1.0, 0.0, 0.0});
    const double lg = relativeLuminance(Srgb{0.0, 1.0, 0.0});
    const double lb = relativeLuminance(Srgb{0.0, 0.0, 1.0});
    CHECK_NEAR(lr, 0.2126, 1e-9);
    CHECK_NEAR(lg, 0.7152, 1e-9);
    CHECK_NEAR(lb, 0.0722, 1e-9);
}

void testContrastRatio() {
    // The canonical extreme: black vs white is exactly 21:1.
    CHECK_NEAR(contrastRatio(kBlack, kWhite), 21.0, 1e-9);

    // Self-contrast is 1:1.
    CHECK_NEAR(contrastRatio(kMidGray, kMidGray), 1.0, 1e-12);

    // Symmetric in its arguments.
    CHECK_NEAR(contrastRatio(kBlack, kMidGray),
               contrastRatio(kMidGray, kBlack), 1e-12);

    // Hand-checked mid pair: white vs 0.5 gray = 1.05 / (0.21404 + 0.05).
    CHECK_NEAR(contrastRatio(kWhite, kMidGray), 1.05 / 0.26404, 1e-3);
}

void testRejectsBadInput() {
    CHECK_THROWS(relativeLuminance(Srgb{1.5, 0.0, 0.0}));
    CHECK_THROWS(relativeLuminance(Srgb{0.0, -0.1, 0.0}));
    CHECK_THROWS(contrastRatio(kBlack, Srgb{0.0, 0.0, 2.0}));
}

} // namespace

int main() {
    testRelativeLuminanceAnchors();
    testContrastRatio();
    testRejectsBadInput();
    return palette_test::summarize("legibility");
}
// Copyright Ben Paul Wise. All Rights Reserved.
