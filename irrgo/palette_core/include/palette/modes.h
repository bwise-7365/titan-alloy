// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PALETTE_MODES_HPP
#define PALETTE_MODES_HPP

#include "palette/color.h"
#include "palette/types.h"

// The three pure entry modes (DESIGN.md §4). Each returns a complete `Palette`
// (chosen colors + diagnostics + any least-bad-accommodation warnings). They
// NEVER throw on an infeasible (clashing) but well-formed input: legibility is
// always satisfied and harmony is relaxed instead (DESIGN.md §5). They DO throw
// std::out_of_range if a given sRGB color has a channel outside [0,1].
namespace palette {

// Mode 1: the user fixed the board background; complete the two piece colors.
Palette fromBackground(const Srgb& background, Harmony t, const Constraints& c);

// Mode 2: the user fixed one piece; complete the opposing piece and background.
Palette fromOnePiece(const Srgb& piece1, Harmony t, const Constraints& c);

// Mode 3: the user fixed both pieces; complete the background.
Palette fromTwoPieces(const Srgb& piece1, const Srgb& piece2, const Constraints& c);

} // namespace palette

#endif // PALETTE_MODES_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
