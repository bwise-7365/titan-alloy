// Copyright Ben Paul Wise. All Rights Reserved.
#include "palette/conversion.hpp"
#include "palette/color.hpp"
#include "palette/types.hpp"
#include "test_support.hpp"

#include <cmath>

using namespace palette;

namespace {

void checkSrgb(const Srgb& got, const Srgb& want, double tol) {
    CHECK_NEAR(got.r, want.r, tol);
    CHECK_NEAR(got.g, want.g, tol);
    CHECK_NEAR(got.b, want.b, tol);
}

// The forward RYB->sRGB map must reproduce the ported ArtColors cube corners
// exactly (DESIGN.md §7 / CLAUDE.md: not invented).
void testForwardCorners() {
    checkSrgb(rybToSrgb(Ryb{0, 0, 0}), Srgb{0.0, 0.0, 0.0}, 1e-9);  // black
    checkSrgb(rybToSrgb(Ryb{1, 0, 0}), Srgb{1.0, 0.0, 0.0}, 1e-9);  // red
    checkSrgb(rybToSrgb(Ryb{0, 1, 0}), Srgb{0.9, 0.9, 0.0}, 1e-9);  // yellow
    checkSrgb(rybToSrgb(Ryb{0, 0, 1}), Srgb{0.0, 0.36, 1.0}, 1e-9); // blue
    checkSrgb(rybToSrgb(Ryb{0, 1, 1}), Srgb{0.0, 0.9, 0.2}, 1e-9);  // green
    checkSrgb(rybToSrgb(Ryb{1, 1, 0}), Srgb{1.0, 0.6, 0.0}, 1e-9);  // orange
    checkSrgb(rybToSrgb(Ryb{1, 0, 1}), Srgb{0.6, 0.0, 1.0}, 1e-9);  // purple
    checkSrgb(rybToSrgb(Ryb{1, 1, 1}), Srgb{1.0, 1.0, 1.0}, 1e-9);  // white
}

// HSV-on-RYB <-> RYB is exact standard HSV math (primaries at 0/120/240).
void testHsvRoundTrip() {
    for (double hue = 0.0; hue < 360.0; hue += 30.0) {
        const HsvRyb in{hue, 1.0, 1.0};
        const HsvRyb out = rybToHsvRyb(hsvRybToRyb(in));
        CHECK_NEAR(out.hueDeg, hue, 1e-6);
        CHECK_NEAR(out.sat, 1.0, 1e-6);
        CHECK_NEAR(out.val, 1.0, 1e-6);
    }
    // Primary RYB coords land at the expected wheel angles.
    CHECK_NEAR(rybToHsvRyb(Ryb{1, 0, 0}).hueDeg, 0.0, 1e-6);
    CHECK_NEAR(rybToHsvRyb(Ryb{0, 1, 0}).hueDeg, 120.0, 1e-6);
    CHECK_NEAR(rybToHsvRyb(Ryb{0, 0, 1}).hueDeg, 240.0, 1e-6);
}

// The fitted inverse recovers the primary hues from sRGB closely (the inverse is
// approximate, so a few degrees of tolerance; secondaries drift more).
void testInverseHueOfPrimaries() {
    CHECK_NEAR(hueOfSrgb(Srgb{1.0, 0.0, 0.0}), 0.0, 8.0);   // red
    CHECK_NEAR(hueOfSrgb(Srgb{0.9, 0.9, 0.0}), 120.0, 8.0); // yellow corner
    const double blueHue = hueOfSrgb(Srgb{0.0, 0.36, 1.0}); // blue corner
    CHECK(std::fabs(blueHue - 240.0) <= 8.0);
}

void testRejectsBadInput() {
    CHECK_THROWS(rybToSrgb(Ryb{1.5, 0.0, 0.0}));
    CHECK_THROWS(srgbToRyb(Srgb{0.0, -0.2, 0.0}));
    CHECK_THROWS(hsvRybToRyb(HsvRyb{0.0, 2.0, 0.5}));
}

} // namespace

int main() {
    testForwardCorners();
    testHsvRoundTrip();
    testInverseHueOfPrimaries();
    testRejectsBadInput();
    return palette_test::summarize("conversion");
}
// Copyright Ben Paul Wise. All Rights Reserved.
