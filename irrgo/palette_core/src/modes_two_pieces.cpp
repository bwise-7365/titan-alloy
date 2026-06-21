// Copyright Ben Paul Wise. All Rights Reserved.

#include "palette/modes.h"
#include "modes_internal.h"

namespace palette {

// Mode 3 (DESIGN.md §4/§3): both pieces are fixed (possibly disharmonious); the
// background luminance is the closed-form minimax and its hue completes the
// pieces' scheme or goes neutral. Never throws on a clashing-but-valid pair;
// emits least-bad-accommodation warnings instead.
Palette fromTwoPieces(const Srgb& piece1, const Srgb& piece2,
                      const Constraints& c) {
    // The pieces are user-fixed, so no harmony template generated them; label the
    // diagnostics with SplitComplement (the cleanest background-slot scheme).
    return detail::completeFromPieces(piece1, piece2, Harmony::SplitComplement, c);
}

} // namespace palette
// Copyright Ben Paul Wise. All Rights Reserved.
