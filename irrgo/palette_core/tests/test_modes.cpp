// Copyright Ben Paul Wise. All Rights Reserved.

#include <cmath>

#include "palette/modes.h"
#include "palette/color.h"
#include "palette/conversion.h"
#include "palette/types.h"
#include "test_support.h"

using namespace palette;

namespace {

bool hasWarning(const Palette& p, Warning w) {
    for (const Warning x : p.warnings) {
        if (x == w) {
            return true;
        }
    }
    return false;
}

const Srgb kBlack{0.0, 0.0, 0.0};
const Srgb kWhite{1.0, 1.0, 1.0};
const Srgb kBeige{1.0, 1.0, 0.8667}; // RGB(255,255,221), the default board

void testMode3BlackWhite() {
    const Constraints c;
    const Palette p = fromTwoPieces(kBlack, kWhite, c);

    // Spread pieces -> interior geometric-mean gray (the chess/beige case).
    CHECK_NEAR(p.diagnostics.backgroundTargetLuminance, std::sqrt(0.0525) - 0.05, 1e-6);
    // Both figure-ground contrasts clear the floor and are roughly equalized.
    CHECK(p.diagnostics.contrastBgP1 >= c.minPieceBgContrast - 1e-6);
    CHECK(p.diagnostics.contrastBgP2 >= c.minPieceBgContrast - 1e-6);
    // Black + white are far apart in luminance: not a "too close" failure.
    CHECK(!hasWarning(p, Warning::PiecesHueTooClose));
}

void testMode3PiecesHueTooClose() {
    const Constraints c;
    // Two near-identical reddish colors: tiny hue gap AND tiny luminance gap.
    const Palette p = fromTwoPieces(Srgb{0.50, 0.20, 0.20}, Srgb{0.54, 0.22, 0.22}, c);
    CHECK(hasWarning(p, Warning::PiecesHueTooClose));
    // Still legible: never returns an illegible board.
    CHECK(p.diagnostics.contrastBgP1 >= c.minPieceBgContrast - 1e-6);
    CHECK(p.diagnostics.contrastBgP2 >= c.minPieceBgContrast - 1e-6);
}

void testMode3LuminanceMidband() {
    const Constraints c;
    // A gray at L ~= sqrt(0.0525)-0.05 ~= 0.179 (channel ~0.46).
    const Srgb midGray{0.46, 0.46, 0.46};
    const Palette p = fromTwoPieces(midGray, midGray, c);
    CHECK(hasWarning(p, Warning::PiecesLuminanceMidband));
}

void testMode3NoHarmonicBackground() {
    const Constraints c;
    // Two in-gamut hues 95 deg apart: matches no recognized scheme separation
    // ({0,30,60,120,180}) -> neutral background + warning.
    const Srgb piece1 = srgbAtHsvRyb(HsvRyb{0.0, 1.0, 0.6});
    const Srgb piece2 = srgbAtHsvRyb(HsvRyb{95.0, 1.0, 0.6});
    const Palette p = fromTwoPieces(piece1, piece2, c);
    CHECK(hasWarning(p, Warning::NoHarmonicBackground));
    // Background is neutral (near-zero chroma): R, G, B nearly equal.
    const Srgb bg = p.background;
    CHECK(std::fabs(bg.r - bg.g) < 0.02);
    CHECK(std::fabs(bg.g - bg.b) < 0.02);
    CHECK(p.diagnostics.contrastBgP1 >= c.minPieceBgContrast - 1e-6);
    CHECK(p.diagnostics.contrastBgP2 >= c.minPieceBgContrast - 1e-6);
}

void testMode1Background() {
    const Constraints c;
    const Palette p = fromBackground(kBeige, Harmony::SplitComplement, c);
    // Pieces are generated to clear the floor against the fixed board.
    CHECK(p.diagnostics.contrastBgP1 >= c.minPieceBgContrast - 1e-6);
    CHECK(p.diagnostics.contrastBgP2 >= c.minPieceBgContrast - 1e-6);
    // The returned background is exactly the fixed input.
    CHECK_NEAR(p.background.r, kBeige.r, 1e-12);
    CHECK_NEAR(p.background.b, kBeige.b, 1e-12);
}

void testMode2OnePiece() {
    const Constraints c;
    const Palette p = fromOnePiece(Srgb{1.0, 0.0, 0.0}, Harmony::Complement, c);
    CHECK(p.diagnostics.contrastBgP1 >= c.minPieceBgContrast - 1e-6);
    CHECK(p.diagnostics.contrastBgP2 >= c.minPieceBgContrast - 1e-6);
    // The fixed piece is preserved as piece1.
    CHECK_NEAR(p.piece1.r, 1.0, 1e-12);
}

void testInfeasibleNeverThrows() {
    const Constraints c;
    // Clashing but well-formed inputs must produce a valid result, not throw.
    CHECK_THROWS_NOT(fromTwoPieces(Srgb{1.0, 0.0, 0.0}, Srgb{0.0, 1.0, 0.0}, c));
    // Out-of-range channels ARE malformed input and should throw.
    CHECK_THROWS(fromTwoPieces(Srgb{1.2, 0.0, 0.0}, kWhite, c));
}

} // namespace

int main() {
    testMode3BlackWhite();
    testMode3PiecesHueTooClose();
    testMode3LuminanceMidband();
    testMode3NoHarmonicBackground();
    testMode1Background();
    testMode2OnePiece();
    testInfeasibleNeverThrows();
    return palette_test::summarize("modes");
}
// Copyright Ben Paul Wise. All Rights Reserved.
