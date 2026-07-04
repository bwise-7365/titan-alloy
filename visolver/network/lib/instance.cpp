// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Flow-planning instance validation and random generation.
// ----------------------------------------------
#include "instance.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <random>
#include <stdexcept>
#include <string>

using std::string;

namespace VINCP::Network {

  namespace {

    void
    require(bool okP, const string& message)
    {
      if (!okP) {
        throw std::invalid_argument("Network::Instance: " + message);
      }
      return;
    }

    // A node label: class letter + zero-padded 3-digit per-class index, e.g.
    // 'S' and 7 -> "S007". Widths above 999 simply use more digits.
    string
    formatNodeLabel(char prefix, int index)
    {
      char buf[16];
      std::snprintf(buf, sizeof(buf), "%c%03d", prefix, index);
      return string(buf);
    }

  } // namespace

  void
  validateInstance(const Instance& inst)
  {
    const Index m = inst.numNodes;
    require(0 < m, "numNodes must be positive.");
    require(inst.supplyCap.size() == m, "supplyCap size must equal numNodes.");
    require(inst.demand.size() == m, "demand size must equal numNodes.");
    require(inst.priority.size() == m, "priority size must equal numNodes.");
    require(inst.cost.rows() == m && inst.cost.cols() == m,
            "cost must be numNodes x numNodes.");
    require(0.0 <= inst.tonMileLimit, "tonMileLimit must be non-negative.");

    // Coordinates are optional (abstract instances omit them), but if either is
    // supplied both must be present and sized numNodes with finite entries.
    const bool hasCoordsP = 0 != inst.xCoord.size() || 0 != inst.yCoord.size();
    if (hasCoordsP) {
      require(inst.xCoord.size() == m && inst.yCoord.size() == m,
              "xCoord/yCoord, when present, must both have size numNodes.");
      require(inst.xCoord.allFinite() && inst.yCoord.allFinite(),
              "coordinate entries must be finite.");
    }

    // Labels are likewise optional, but sized numNodes when present.
    if (!inst.labels.empty()) {
      require(static_cast<Index>(inst.labels.size()) == m,
              "labels, when present, must have size numNodes.");
    }

    for (Index i = 0; i < m; ++i) {
      require(std::isfinite(inst.supplyCap(i)) && 0.0 <= inst.supplyCap(i),
              "supplyCap entries must be finite and non-negative.");
      require(std::isfinite(inst.demand(i)) && 0.0 <= inst.demand(i),
              "demand entries must be finite and non-negative.");
      require(std::isfinite(inst.priority(i)),
              "priority entries must be finite.");
      if (0.0 < inst.demand(i)) {
        require(0.0 < inst.priority(i),
                "priority must be positive at demand nodes.");
      }
      for (Index j = 0; j < m; ++j) {
        require(std::isfinite(inst.cost(i, j)) && 0.0 < inst.cost(i, j),
                "cost entries must be finite and positive.");
      }
    }
    return;
  }

  vector<Index>
  sourceNodes(const Instance& inst)
  {
    vector<Index> nodes;
    for (Index i = 0; i < inst.numNodes; ++i) {
      if (0.0 < inst.supplyCap(i)) {
        nodes.push_back(i);
      }
    }
    return nodes;
  }

  vector<Index>
  sinkNodes(const Instance& inst)
  {
    vector<Index> nodes;
    for (Index i = 0; i < inst.numNodes; ++i) {
      if (0.0 < inst.demand(i)) {
        nodes.push_back(i);
      }
    }
    return nodes;
  }

  double
  totalSupplyCap(const Instance& inst)
  {
    return inst.supplyCap.sum();
  }

  double
  totalDemand(const Instance& inst)
  {
    return inst.demand.sum();
  }

  string
  nodeLabel(const Instance& inst, Index node)
  {
    if (static_cast<Index>(inst.labels.size()) == inst.numNodes
        && 0 <= node && node < inst.numNodes) {
      return inst.labels[static_cast<size_t>(node)];
    }
    return "#" + std::to_string(node);
  }

  void
  validateProfile(const InstanceProfile& profile)
  {
    require(0 <= profile.numSupplyOnly && 0 <= profile.numBoth
                && 0 <= profile.numDemandOnly && 0 <= profile.numNeither,
            "profile node counts must be non-negative.");
    require(0 < profile.numSupplyOnly + profile.numBoth,
            "profile must include at least one source node.");
    require(0 < profile.numBoth + profile.numDemandOnly,
            "profile must include at least one demand node.");
    require(0.0 < profile.supplyLo && profile.supplyLo <= profile.supplyHi,
            "supply range must be positive and ordered.");
    require(0.0 < profile.demandLo && profile.demandLo <= profile.demandHi,
            "demand range must be positive and ordered.");
    require(0.0 < profile.priorityLo && profile.priorityLo <= profile.priorityHi,
            "priority range must be positive and ordered.");
    require(0.0 < profile.selfCostLo && profile.selfCostLo <= profile.selfCostHi,
            "self-cost range must be positive and ordered.");
    require(0.0 < profile.squareSide, "squareSide must be positive.");
    require(0.0 < profile.costFloor, "costFloor must be positive.");
    require(0.0 <= profile.milesPerUnit, "milesPerUnit must be non-negative.");
    require(0.0 <= profile.asymmetryMax, "asymmetryMax must be non-negative.");
    require(0 <= profile.laydownType, "laydownType must be non-negative.");
    require(profile.laydownType <= 1, "laydownType 2+ is not yet defined.");
    if (1 == profile.laydownType) {
      require(0.0 < profile.bandXWidth, "bandXWidth must be positive.");
      require(0.0 <= profile.bandXStep, "bandXStep must be non-negative.");
      require(profile.bandYLo <= profile.bandYHi,
              "band y-range must be ordered.");
      require(0.0 <= profile.jitterHalfWidth && profile.jitterHalfWidth < 1.0,
              "jitterHalfWidth must lie in [0, 1).");
      require(0.0 < profile.bandMinCostLo
                  && profile.bandMinCostLo <= profile.bandMinCostHi,
              "band min-cost range must be positive and ordered.");
    }
    return;
  }

  Instance
  makeRandomInstance(const InstanceProfile& profile, std::uint64_t seed)
  {
    validateProfile(profile);
    const Index numClassed =
        profile.numSupplyOnly + profile.numBoth + profile.numDemandOnly;
    const Index m = numClassed + profile.numNeither;

    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<double> supplyDist(profile.supplyLo,
                                                      profile.supplyHi);
    std::uniform_real_distribution<double> demandDist(profile.demandLo,
                                                      profile.demandHi);
    std::uniform_real_distribution<double> priorityDist(profile.priorityLo,
                                                        profile.priorityHi);
    std::uniform_real_distribution<double> selfCostDist(profile.selfCostLo,
                                                        profile.selfCostHi);
    // Off-diagonal jitter band per laydown: type 0 inflates only
    // (U[1, 1+asymmetryMax]); type 1 is symmetric (U[1-h, 1+h]).
    const double jitterLo =
        (0 == profile.laydownType) ? 1.0 : 1.0 - profile.jitterHalfWidth;
    const double jitterHi = (0 == profile.laydownType)
                                ? 1.0 + profile.asymmetryMax
                                : 1.0 + profile.jitterHalfWidth;
    std::uniform_real_distribution<double> jitterDist(jitterLo, jitterHi);

    // Node coordinates, drawn first so the geometry is independent of the
    // tonnage draws below. Type 0: uniform in the square. Type 1: each node
    // class gets its own x-band (alternate-laydown.txt), shared y-band.
    VectorXd xs(m), ys(m);
    if (0 == profile.laydownType) {
      std::uniform_real_distribution<double> coordDist(0.0, profile.squareSide);
      for (Index i = 0; i < m; ++i) {
        xs(i) = coordDist(rng);
        ys(i) = coordDist(rng);
      }
    }
    else {
      std::uniform_real_distribution<double> widthDist(0.0, profile.bandXWidth);
      std::uniform_real_distribution<double> yDist(profile.bandYLo,
                                                   profile.bandYHi);
      const double fullSpan = 2.0 * profile.bandXStep + profile.bandXWidth;
      std::uniform_real_distribution<double> spanDist(0.0, fullSpan);
      for (Index i = 0; i < m; ++i) {
        if (numClassed <= i) {
          xs(i) = spanDist(rng);                  // transit: anywhere in the span
        }
        else {
          Index group = 0;                        // A: supply-only
          if (profile.numSupplyOnly + profile.numBoth <= i) {
            group = 2;                            // C: demand-only
          }
          else if (profile.numSupplyOnly <= i) {
            group = 1;                            // B: both
          }
          xs(i) = static_cast<double>(group) * profile.bandXStep
                  + widthDist(rng);
        }
        ys(i) = yDist(rng);
      }
    }

    Instance inst;
    inst.numNodes = m;
    inst.supplyCap = VectorXd::Zero(m);
    inst.demand = VectorXd::Zero(m);
    inst.priority = VectorXd::Zero(m);
    inst.cost = MatrixXd::Zero(m, m);

    // Node classes are contiguous blocks:
    // [supply-only | both | demand-only | transit].
    // Each class gets its own zero-based, zero-padded label counter.
    inst.labels.resize(static_cast<size_t>(m));
    int nextS = 0, nextM = 0, nextD = 0, nextT = 0;
    for (Index i = 0; i < m; ++i) {
      const bool suppliesP = i < profile.numSupplyOnly + profile.numBoth;
      const bool demandsP = profile.numSupplyOnly <= i && i < numClassed;
      if (suppliesP) {
        inst.supplyCap(i) = supplyDist(rng);
      }
      if (demandsP) {
        inst.demand(i) = demandDist(rng);
      }
      inst.priority(i) = priorityDist(rng);

      char prefix;
      int index;
      if (suppliesP && demandsP) {
        prefix = 'M';
        index = nextM++;
      }
      else if (suppliesP) {
        prefix = 'S';
        index = nextS++;
      }
      else if (demandsP) {
        prefix = 'D';
        index = nextD++;
      }
      else {
        prefix = 'T';
        index = nextT++;
      }
      inst.labels[static_cast<size_t>(i)] = formatNodeLabel(prefix, index);
    }

    std::uniform_real_distribution<double> bandFloorDist(profile.bandMinCostLo,
                                                         profile.bandMinCostHi);
    for (Index i = 0; i < m; ++i) {
      for (Index j = 0; j < m; ++j) {
        if (i == j) {
          inst.cost(i, i) = selfCostDist(rng);
        }
        else if (0 == profile.laydownType) {
          const double separation = std::hypot(xs(i) - xs(j), ys(i) - ys(j));
          const double base =
              profile.costFloor + profile.milesPerUnit * separation;
          inst.cost(i, j) = base * jitterDist(rng);
        }
        else {
          // Type 1: bare Euclidean distance, floored per ordered pair so
          // overlap-region neighbors cannot undercut sensible arc costs.
          const double separation = std::hypot(xs(i) - xs(j), ys(i) - ys(j));
          inst.cost(i, j) = std::max(separation * jitterDist(rng),
                                     bandFloorDist(rng));
        }
      }
    }

    // Retain the placement so the viewer can draw the true geometry (both
    // laydowns are geometric; type 0 keeps its uniform-square coordinates).
    inst.xCoord = xs;
    inst.yCoord = ys;

    validateInstance(inst);
    return inst;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
