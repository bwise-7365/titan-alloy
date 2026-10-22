// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PALETTE_MINIMAX_HPP
#define PALETTE_MINIMAX_HPP

namespace palette {

// Closed-form background-luminance minimax (DESIGN.md §3).
//
// Given the relative luminances of two fixed piece colors (each in [0, 1]),
// returns the background relative luminance L that maximizes the smaller of the
// two figure-ground contrasts contrast(B,P1), contrast(B,P2).
//
// The return value is the target luminance L (= chosen shifted optimum beta*
// minus 0.05), in [0, 1]:
//   - clustered pieces  -> an extreme wins: 0.0 (black) or 1.0 (white)
//   - spread pieces     -> interior geometric-mean gray, L = sqrt(l1*l2) - 0.05
//     where l_i = L_i + 0.05.
//
// Argument order does not matter. Throws std::out_of_range if either luminance
// is outside [0, 1].
double minimaxBackgroundLuminance(double pieceLuminance1, double pieceLuminance2);

} // namespace palette

#endif // PALETTE_MINIMAX_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
