// Copyright Ben Paul Wise. All Rights Reserved.

#include "palette/harmony.h"
#include "test_support.h"

using namespace palette;

namespace {

void testNormalizeAndDistance() {
    CHECK_NEAR(normalizeHue(370.0), 10.0, 1e-9);
    CHECK_NEAR(normalizeHue(-30.0), 330.0, 1e-9);
    CHECK_NEAR(normalizeHue(360.0), 0.0, 1e-9);

    CHECK_NEAR(hueDistance(10.0, 40.0), 30.0, 1e-9);
    CHECK_NEAR(hueDistance(350.0, 10.0), 20.0, 1e-9); // wraps across 0
    CHECK_NEAR(hueDistance(0.0, 180.0), 180.0, 1e-9);
    CHECK_NEAR(hueDistance(200.0, 20.0), 180.0, 1e-9);
}

void testMidpointAndComplement() {
    CHECK_NEAR(circularMidpoint(0.0, 120.0), 60.0, 1e-9);
    CHECK_NEAR(circularMidpoint(350.0, 10.0), 0.0, 1e-9); // shorter arc, wraps
    CHECK_NEAR(complementHue(0.0), 180.0, 1e-9);
    CHECK_NEAR(complementHue(200.0), 20.0, 1e-9);
}

void testPieceHuesForBackground() {
    const HuePair tri = pieceHuesForBackground(0.0, Harmony::Triad, 30.0);
    CHECK_NEAR(tri.first, 120.0, 1e-9);
    CHECK_NEAR(tri.second, 240.0, 1e-9);

    const HuePair split = pieceHuesForBackground(0.0, Harmony::SplitComplement, 30.0);
    CHECK_NEAR(split.first, 150.0, 1e-9);
    CHECK_NEAR(split.second, 210.0, 1e-9);

    const HuePair comp = pieceHuesForBackground(0.0, Harmony::Complement, 30.0);
    CHECK_NEAR(comp.first, 180.0, 1e-9);
    CHECK_NEAR(comp.second, 180.0, 1e-9); // coincide -> distinguished by luminance

    const HuePair tet = pieceHuesForBackground(0.0, Harmony::Tetrad, 30.0);
    CHECK_NEAR(tet.first, 90.0, 1e-9);
    CHECK_NEAR(tet.second, 270.0, 1e-9);
}

void testPartnerHueForPiece() {
    CHECK_NEAR(partnerHueForPiece(0.0, Harmony::Complement, 30.0), 180.0, 1e-9);
    CHECK_NEAR(partnerHueForPiece(0.0, Harmony::Triad, 30.0), 120.0, 1e-9);
    CHECK_NEAR(partnerHueForPiece(0.0, Harmony::SplitComplement, 30.0), 210.0, 1e-9);
    CHECK_NEAR(partnerHueForPiece(0.0, Harmony::Analogous, 30.0), 30.0, 1e-9);
    CHECK_NEAR(partnerHueForPiece(0.0, Harmony::Tetrad, 30.0), 90.0, 1e-9);
}

} // namespace

int main() {
    testNormalizeAndDistance();
    testMidpointAndComplement();
    testPieceHuesForBackground();
    testPartnerHueForPiece();
    return palette_test::summarize("harmony");
}
// Copyright Ben Paul Wise. All Rights Reserved.
