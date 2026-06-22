// Copyright Ben Paul Wise. All Rights Reserved.

#include <cstdio>

#include "palette/color.h"
#include "palette/conversion.h"
#include "palette/harmony.h"
#include "palette/modes.h"
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

// B -> {P1,P2} (Mode 1) -> B2 (Mode 3). The *hue* of B2 must recover the hue of
// B to within 10 degrees; luminance is NOT expected to round-trip (the two
// directions use different luminance rules). Prints the actual hue drift. With
// srgbToRyb a true inverse of the forward cube, the generated pieces invert
// essentially exactly, so the drift should be a fraction of a degree.
void runScheme(const char* name, Harmony t) {
    const Constraints c;
    const Srgb boards[] = {
        {0.85, 0.10, 0.10}, // red
        {0.90, 0.55, 0.05}, // orange
        {0.10, 0.65, 0.20}, // green
        {0.10, 0.45, 0.85}, // blue
        {0.45, 0.15, 0.75}, // purple
    };

    for (const Srgb& b : boards) {
        const Palette fwd = fromBackground(b, t, c);
        const Palette rev = fromTwoPieces(fwd.piece1, fwd.piece2, c);

        const double hueB = hueOfSrgb(b);
        const double hueB2 = hueOfSrgb(rev.background);
        const bool recognized = !hasWarning(rev, Warning::NoHarmonicBackground);
        const double drift = hueDistance(hueB, hueB2);

        std::printf(
            "  [%-15s] B=(%.2f,%.2f,%.2f) hueB=%6.1f -> hueB2=%6.1f  drift=%5.1f  %s\n",
            name, b.r, b.g, b.b, hueB, hueB2, drift,
            recognized ? "recognized" : "NEUTRAL (scheme not recognized)");

        CHECK(recognized);
        if (recognized) {
            CHECK(drift <= 10.0);
        }
    }
}

void testBackgroundHueRoundTrip() {
    runScheme("Triad", Harmony::Triad);
    runScheme("SplitComplement", Harmony::SplitComplement);
}

} // namespace

int main() {
    testBackgroundHueRoundTrip();
    return palette_test::summarize("roundtrip");
}
// Copyright Ben Paul Wise. All Rights Reserved.
