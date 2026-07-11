// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Fleet preprocessing: shared shortest routes on the distance matrix and the
// per-asset reduced source-sink problems with ROUND-TRIP mileage
// (fleet-formulation.md section 9, Lemma FL3).
// ----------------------------------------------
#ifndef VIMCP_NETWORK_FLEETREDUCTION_HPP
#define VIMCP_NETWORK_FLEETREDUCTION_HPP

#include "fleetinstance.hpp"
#include "reduction.hpp"

namespace VIMCP::Network {

  // The reduced conservative fleet problem. One shortest-route map is shared
  // by all assets (one distance matrix, G7); each asset gets its own
  // source/sink/kept structure, REUSING ReducedProblem with one twist: its
  // shipCost holds the ROUND-TRIP reduced mileage
  //   rho-hat(s, t) = d-hat(s, t) + d-hat(t, s)   (s != t)
  //   rho-hat(s, s) = selfDistance(s)             (the >= 1-arc loop)
  // because every loaded leg pays a deadhead return (G-F3); the kept lists
  // are therefore ordered by round-trip miles. `routes` keeps the ONE-WAY
  // successor matrix for the unpack path walk (Lemma FL4 walks the outbound
  // route and the reverse route separately).
  struct FleetReducedProblem {
    ShortestRoutes routes;            // one-way d-hat + successors (unpack)
    vector<ReducedProblem> perAsset;  // shipCost = rho-hat, kept per sink
    MatrixXd kappa;                   // numAssets x numTypes units/vehicle;
                                      // 0 = incapable (no variable, G-F1)
  };

  // Build the reduction under the given per-asset screen (the base count/gap
  // rules, applied to round-trip mileage). Throws std::invalid_argument if
  // any asset with demand has no supply or no capable vehicle type: such an
  // asset cannot be optimized and indicates a modeling error.
  FleetReducedProblem makeFleetReducedProblem(const FleetInstance& inst,
                                              const ScreenParams& screen = {});

} // namespace VIMCP::Network

#endif // VIMCP_NETWORK_FLEETREDUCTION_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
