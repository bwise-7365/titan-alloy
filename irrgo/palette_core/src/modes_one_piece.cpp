// Copyright Ben Paul Wise. All Rights Reserved.
#include "palette/modes.hpp"

#include "modes_internal.hpp"
#include "palette/conversion.hpp"
#include "palette/harmony.hpp"
#include "palette/legibility.hpp"

namespace palette {

// Mode 2 (DESIGN.md §4): one piece is fixed; choose the opposing piece as its
// harmony partner, then run the Mode-3 background logic on the pair.
Palette fromOnePiece(const Srgb& piece1, Harmony t, const Constraints& c) {
    const double l1 = relativeLuminance(piece1); // throws on bad channels
    const double h1 = hueOfSrgb(piece1);
    const double h2 = partnerHueForPiece(h1, t, c.splitComplementAlphaDeg);

    // The partner's luminance defaults to the first piece's (hue distinguishes
    // them); with spread enabled, nudge it away for a bonus distinction.
    double target2 = l1;
    if (c.allowPieceLuminanceSpread) {
        if (l1 <= 0.5) {
            target2 = detail::clampUnit(l1 + 0.20);
        } else {
            target2 = detail::clampUnit(l1 - 0.20);
        }
    }

    const Srgb piece2 = detail::realizeAtLuminance(h2, target2, 1.0);
    return detail::completeFromPieces(piece1, piece2, t, c);
}

} // namespace palette
// Copyright Ben Paul Wise. All Rights Reserved.
