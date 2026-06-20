// Copyright Ben Paul Wise. All Rights Reserved.
#include "palette/minimax.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace palette {

double minimaxBackgroundLuminance(double pieceLuminance1, double pieceLuminance2) {
    if (pieceLuminance1 < 0.0 || pieceLuminance1 > 1.0 ||
        pieceLuminance2 < 0.0 || pieceLuminance2 > 1.0) {
        throw std::out_of_range("piece luminance outside [0, 1]");
    }

    // Shifted luminances l_i = L_i + 0.05, ordered l1 <= l2 (DESIGN.md §3).
    const double l1 = std::min(pieceLuminance1, pieceLuminance2) + 0.05;
    const double l2 = std::max(pieceLuminance1, pieceLuminance2) + 0.05;

    // Objective value g(beta) at each of the three candidate optima.
    const double objDarkest = l1 / 0.05;         // beta = 0.05  (binds on darker piece)
    const double objLightest = 1.05 / l2;        // beta = 1.05  (binds on lighter piece)
    const double objInterior = std::sqrt(l2 / l1); // beta* = sqrt(l1*l2) (contrasts equalize)

    if (objInterior >= objDarkest && objInterior >= objLightest) {
        return std::sqrt(l1 * l2) - 0.05; // interior geometric-mean gray
    }
    if (objDarkest >= objLightest) {
        return 0.0; // black background (beta = 0.05 -> L = 0)
    }
    return 1.0; // white background (beta = 1.05 -> L = 1)
}

} // namespace palette
// Copyright Ben Paul Wise. All Rights Reserved.
