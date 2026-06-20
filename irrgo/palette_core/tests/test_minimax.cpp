// Copyright Ben Paul Wise. All Rights Reserved.
#include "palette/minimax.hpp"
#include "test_support.hpp"

#include <algorithm>
#include <cmath>

using palette::minimaxBackgroundLuminance;

namespace {

// Contrast between a background and a piece expressed directly in luminance
// space (avoids round-tripping through sRGB): the same ratio formula, where the
// inputs are already relative luminances.
double luminanceContrast(double bgL, double pieceL) {
    const double hi = std::max(bgL, pieceL) + 0.05;
    const double lo = std::min(bgL, pieceL) + 0.05;
    return hi / lo;
}

void testSpreadPiecesGiveInteriorGray() {
    // Black + white pieces: beta* = sqrt(0.0525) ~= 0.22913, target L = beta*-0.05.
    const double L = minimaxBackgroundLuminance(0.0, 1.0);
    CHECK_NEAR(L, std::sqrt(0.0525) - 0.05, 1e-9); // ~= 0.17913 (the chess/beige gray)

    // The defining property of the interior optimum: the two figure-ground
    // contrasts equalize.
    CHECK_NEAR(luminanceContrast(L, 0.0), luminanceContrast(L, 1.0), 1e-9);

    // Order of arguments must not matter.
    CHECK_NEAR(minimaxBackgroundLuminance(1.0, 0.0), L, 1e-12);
}

void testClusteredPiecesGoToAnExtreme() {
    // Two clustered bright pieces -> black background wins.
    CHECK_NEAR(minimaxBackgroundLuminance(0.80, 0.85), 0.0, 1e-12);

    // Two clustered dark pieces -> white background wins.
    CHECK_NEAR(minimaxBackgroundLuminance(0.0, 0.0), 1.0, 1e-12);
}

void testBlackWhiteBoundary() {
    // Clustered case picks black iff l1*l2 > 0.0525. With both pieces at the
    // same luminance L, l1*l2 = (L+0.05)^2, so the crossover is at
    // L = sqrt(0.0525) - 0.05 ~= 0.17913.
    // L = 0.20 -> (0.25)^2 = 0.0625 > 0.0525 -> black.
    CHECK_NEAR(minimaxBackgroundLuminance(0.20, 0.20), 0.0, 1e-12);
    // L = 0.15 -> (0.20)^2 = 0.0400 < 0.0525 -> white.
    CHECK_NEAR(minimaxBackgroundLuminance(0.15, 0.15), 1.0, 1e-12);
}

void testReturnedBackgroundIsOptimal() {
    // Spot-check that the returned background really maximizes the min contrast
    // against a small sweep of alternative backgrounds.
    const double l1 = 0.1;
    const double l2 = 0.6;
    const double L = minimaxBackgroundLuminance(l1, l2);
    const double best = std::min(luminanceContrast(L, l1), luminanceContrast(L, l2));
    for (int i = 0; i <= 100; ++i) {
        const double cand = static_cast<double>(i) / 100.0;
        const double g = std::min(luminanceContrast(cand, l1),
                                  luminanceContrast(cand, l2));
        CHECK(g <= best + 1e-9);
    }
}

void testRejectsBadInput() {
    CHECK_THROWS(minimaxBackgroundLuminance(-0.01, 0.5));
    CHECK_THROWS(minimaxBackgroundLuminance(0.5, 1.01));
}

} // namespace

int main() {
    testSpreadPiecesGiveInteriorGray();
    testClusteredPiecesGoToAnExtreme();
    testBlackWhiteBoundary();
    testReturnedBackgroundIsOptimal();
    testRejectsBadInput();
    return palette_test::summarize("minimax");
}
// Copyright Ben Paul Wise. All Rights Reserved.
