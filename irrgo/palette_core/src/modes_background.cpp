// Copyright Ben Paul Wise. All Rights Reserved.

#include "palette/modes.h"
#include "modes_internal.h"
#include "palette/conversion.h"
#include "palette/harmony.h"
#include "palette/legibility.h"

namespace palette {

// Mode 1 (DESIGN.md §4): the board background is fixed; derive two piece colors
// that are harmonic with the board, mutually distinct, and clear the contrast
// floor against the board.
Palette fromBackground(const Srgb& background, Harmony t, const Constraints& c) {
    const double lBg = relativeLuminance(background); // throws on bad channels
    const double hBg = hueOfSrgb(background);

    const HuePair hues = pieceHuesForBackground(hBg, t, c.splitComplementAlphaDeg);

    // Piece luminances on the far side of the floor from the board: lighter
    // pieces need L >= floor*(lBg+0.05)-0.05; darker need L <= (lBg+0.05)/floor-0.05.
    const double floor = c.minPieceBgContrast;
    const double lighterL = floor * (lBg + 0.05) - 0.05;
    const double darkerL = (lBg + 0.05) / floor - 0.05;
    const bool canLighter = lighterL <= 1.0;
    const bool canDarker = darkerL >= 0.0;

    // When the template gives the two pieces the same hue (Complement), force a
    // luminance split so the opponents remain distinguishable.
    const bool huesCoincide = hueDistance(hues.first, hues.second) < 1.0;
    const bool spread = c.allowPieceLuminanceSpread || huesCoincide;

    double target1 = 0.0;
    double target2 = 0.0;
    if (canLighter && canDarker && spread) {
        target1 = detail::clampUnit(lighterL); // one lighter
        target2 = detail::clampUnit(darkerL);  // one darker
    } else if (canLighter) {
        target1 = detail::clampUnit(lighterL);
        target2 = target1;
    } else if (canDarker) {
        target1 = detail::clampUnit(darkerL);
        target2 = target1;
    } else {
        target1 = 1.0; // very high floor: best effort, pieces as light as possible
        target2 = 1.0;
    }

    // Pieces are realized at full available chroma (more chromatic than the board).
    const Srgb piece1 = detail::realizeAtLuminance(hues.first, target1, 1.0);
    const Srgb piece2 = detail::realizeAtLuminance(hues.second, target2, 1.0);

    Palette pal;
    pal.background = background;
    pal.piece1 = piece1;
    pal.piece2 = piece2;
    pal.diagnostics = detail::makeDiagnostics(background, piece1, piece2, t, lBg);
    return pal;
}

} // namespace palette
// Copyright Ben Paul Wise. All Rights Reserved.
