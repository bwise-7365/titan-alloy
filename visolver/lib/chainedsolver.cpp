// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Chained Solodov-Svaiter -> bsHe94b solver implementation.
// ----------------------------------------------
#include "chainedsolver.hpp"

#include <stdexcept>

namespace VINCP {

  VIResult
  chainedSolodovHe(const VectorXd& x0,
                   const MatrixXd& M,
                   const VectorXd& q,
                   const Projector& Pr,
                   double magTol,
                   int iterMax,
                   int iterFreq,
                   const ChainedSolverParams& params,
                   const IterationLogger& logger)
  {
    if (!(0.0 < params.roughMagTol) || 0 >= params.roughIterMax) {
      throw std::invalid_argument(
          "chainedSolodovHe: roughMagTol must be positive and "
          "roughIterMax > 0.");
    }

    // Phase 1: rough globalization. Hitting the cap is fine -- the iterate
    // is a warm start either way. (Component validates the shared inputs.)
    const VIResult rough =
        solodovSvaiter(x0, M, q, Pr, params.roughMagTol, params.roughIterMax,
                       iterFreq, params.rough, logger);

    // Phase 2: tight finish from the warm start.
    VIResult result = bsHe94b(rough.z, M, q, Pr, magTol, iterMax, iterFreq,
                              params.finish, logger);
    result.innerIters = rough.iter + result.iter;   // composite accounting
    return result;
  }

} // namespace VINCP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
