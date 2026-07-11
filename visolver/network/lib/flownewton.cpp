// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Structured Newton factory implementation: transcription of the
// machine-verified algebra in network/doc/ns2-newton-check.mac.
// ----------------------------------------------
#include "flownewton.hpp"

#include <memory>     // shared_ptr: one structural snapshot shared by every factorization
#include <stdexcept>

namespace VIMCP::Network {

  namespace {

    // Structural snapshot of the flow LCP, fixed for the factory's lifetime:
    // everything the per-iteration factorizations need except sOverY.
    struct FlowStructure {
      Index numPairs = 0;
      Index numSources = 0;
      vector<Index> pairSource;   // s(p): source position of pair p
      VectorXd pairCost;          // d_p, aligned with the t block
      vector<Index> sliceStart;   // sink slice boundaries; sentinel numPairs last
      vector<double> sliceQuad;   // Q_n = 2 P_n / D_n^2 per slice
    };

    FlowStructure
    buildStructure(const FlowLcp& lcp)
    {
      if (0 == lcp.numPairs || 0 == lcp.numSources) {
        throw std::invalid_argument(
            "makeFlowNewtonFactory: lcp has no pairs or no sources.");
      }
      const size_t numPairs = static_cast<size_t>(lcp.numPairs);
      if (lcp.pairSourcePos.size() != numPairs
          || lcp.pairSinkPos.size() != numPairs
          || lcp.pairCost.size() != lcp.numPairs) {
        throw std::invalid_argument(
            "makeFlowNewtonFactory: lcp pair arrays disagree with numPairs.");
      }
      const Index dim = lcp.numPairs + lcp.numSources + 1;
      if (lcp.M.rows() != dim || lcp.M.cols() != dim) {
        throw std::invalid_argument(
            "makeFlowNewtonFactory: lcp.M has the wrong dimensions.");
      }

      FlowStructure fs;
      fs.numPairs = lcp.numPairs;
      fs.numSources = lcp.numSources;
      fs.pairSource = lcp.pairSourcePos;
      fs.pairCost = lcp.pairCost;

      // Sink slices: buildFlowLcp enumerates t sink-major, so each sink's
      // pairs are one contiguous run of equal pairSinkPos. Q_n is the Q-block
      // diagonal, M(p, p), constant across the slice.
      for (Index p = 0; p < lcp.numPairs; ++p) {
        const Index source = lcp.pairSourcePos[static_cast<size_t>(p)];
        if (0 > source || lcp.numSources <= source) {
          throw std::invalid_argument(
              "makeFlowNewtonFactory: pair source position out of range.");
        }
        const Index sink = lcp.pairSinkPos[static_cast<size_t>(p)];
        if (0 == p || sink != lcp.pairSinkPos[static_cast<size_t>(p - 1)]) {
          if (0 < p && sink < lcp.pairSinkPos[static_cast<size_t>(p - 1)]) {
            throw std::invalid_argument(
                "makeFlowNewtonFactory: pairs are not sink-major-contiguous.");
          }
          const double quad = lcp.M(p, p);
          if (!(0.0 < quad)) {
            throw std::invalid_argument(
                "makeFlowNewtonFactory: non-positive Q diagonal in lcp.M.");
          }
          fs.sliceStart.push_back(p);
          fs.sliceQuad.push_back(quad);
        }
      }
      fs.sliceStart.push_back(lcp.numPairs);   // sentinel: end of the last slice
      return fs;
    }

    // y = W x on the t block, W = (Qblk + diag(Dt))^{-1}, applied per sink
    // slice in O(k_n) by the Sherman-Morrison form (ns2-newton-check.mac,
    // checks 1 and 3b-2): y|_n = x .* invDt - c_n (u_n . x) u_n with
    // u_n = invDt on the slice and c_n = Q_n / (1 + Q_n sigma_n).
    VectorXd
    applyW(const FlowStructure& fs, const VectorXd& invDt,
           const vector<double>& smCoeff, const VectorXd& x)
    {
      VectorXd y = x.cwiseProduct(invDt);
      for (size_t n = 0; n + 1 < fs.sliceStart.size(); ++n) {
        const Index lo = fs.sliceStart[n];
        const Index len = fs.sliceStart[n + 1] - lo;
        const double dot = invDt.segment(lo, len).dot(x.segment(lo, len));
        y.segment(lo, len) -= (smCoeff[n] * dot) * invDt.segment(lo, len);
      }
      return y;
    }

  } // namespace

  NewtonSolverFactory
  makeFlowNewtonFactory(const FlowLcp& lcp)
  {
    const auto fs = std::make_shared<const FlowStructure>(buildStructure(lcp));

    return [fs](const VectorXd& sOverY, double freeRegularization) -> NewtonSolve {
      if (0.0 != freeRegularization) {
        throw std::invalid_argument(
            "makeFlowNewtonFactory: the flow LCP has no free block, so "
            "freeRegularization must be 0.");
      }
      const Index dim = fs->numPairs + fs->numSources + 1;
      if (sOverY.size() != dim) {
        throw std::invalid_argument(
            "makeFlowNewtonFactory: sOverY has the wrong size for this lcp.");
      }
      if (!(0.0 < sOverY.minCoeff())) {
        throw std::invalid_argument(
            "makeFlowNewtonFactory: sOverY must be strictly positive.");
      }
      const Index numSources = fs->numSources;
      const Index laPos = numSources;              // budget slot within the Schur system

      // Sherman-Morrison data for W on this iteration's Dt.
      const VectorXd invDt = sOverY.head(fs->numPairs).cwiseInverse();
      vector<double> smCoeff(fs->sliceQuad.size(), 0.0);
      for (size_t n = 0; n + 1 < fs->sliceStart.size(); ++n) {
        const Index lo = fs->sliceStart[n];
        const Index len = fs->sliceStart[n + 1] - lo;
        const double sigma = invDt.segment(lo, len).sum();
        smCoeff[n] = fs->sliceQuad[n] / (1.0 + fs->sliceQuad[n] * sigma);
      }

      // Dual Schur complement S = diag(Dmu | Dla) + B^T W B, B = [E d],
      // assembled in two parts (ns2-newton-check.mac, check 3b-3):
      // B^T diag(invDt) B accumulated per pair, minus the per-sink rank-one
      // corrections c_n (B^T u_n)(B^T u_n)^T on their O(k_n^2) support.
      MatrixXd S = MatrixXd::Zero(numSources + 1, numSources + 1);
      S.diagonal().head(numSources) = sOverY.segment(fs->numPairs, numSources);
      S(laPos, laPos) = sOverY(fs->numPairs + numSources);
      for (Index p = 0; p < fs->numPairs; ++p) {
        const Index s = fs->pairSource[static_cast<size_t>(p)];
        const double a = invDt(p);
        const double cost = fs->pairCost(p);
        S(s, s) += a;
        S(s, laPos) += cost * a;
        S(laPos, s) += cost * a;
        S(laPos, laPos) += cost * cost * a;
      }
      for (size_t n = 0; n + 1 < fs->sliceStart.size(); ++n) {
        const Index lo = fs->sliceStart[n];
        const Index hi = fs->sliceStart[n + 1];
        double gLa = 0.0;                          // budget component of B^T u_n
        for (Index p = lo; p < hi; ++p) {
          gLa += fs->pairCost(p) * invDt(p);
        }
        const double c = smCoeff[n];
        for (Index p = lo; p < hi; ++p) {
          const Index sp = fs->pairSource[static_cast<size_t>(p)];
          const double ap = invDt(p);
          for (Index r = lo; r < hi; ++r) {
            S(sp, fs->pairSource[static_cast<size_t>(r)]) -= c * ap * invDt(r);
          }
          S(sp, laPos) -= c * ap * gLa;
          S(laPos, sp) -= c * gLa * ap;
        }
        S(laPos, laPos) -= c * gLa * gLa;
      }

      LLT<MatrixXd> lltS(S);
      if (Success != lltS.info()) {
        throw std::runtime_error(
            "makeFlowNewtonFactory: the dual Schur complement failed its LLT "
            "(not numerically positive definite).");
      }

      // One factorization, shared by the predictor and corrector solves:
      // S [dmu; dla] = [rmu + E^T W rt ; rla + d^T W rt], then back out
      // dt = W (rt - E dmu - d dla) (ns2-newton-check.mac, checks 3a/3b-4).
      return [fs, invDt, smCoeff, lltS](const VectorXd& rhs) -> VectorXd {
        const Index numPairs = fs->numPairs;
        const Index numSources = fs->numSources;
        const Index laPos = numSources;
        if (rhs.size() != numPairs + numSources + 1) {
          throw std::invalid_argument(
              "makeFlowNewtonFactory: rhs has the wrong size for this lcp.");
        }
        const VectorXd wrt = applyW(*fs, invDt, smCoeff, rhs.head(numPairs));

        VectorXd schurRhs(numSources + 1);
        schurRhs.head(numSources) = rhs.segment(numPairs, numSources);
        schurRhs(laPos) = rhs(numPairs + numSources);
        for (Index p = 0; p < numPairs; ++p) {
          schurRhs(fs->pairSource[static_cast<size_t>(p)]) += wrt(p);
          schurRhs(laPos) += fs->pairCost(p) * wrt(p);
        }
        const VectorXd dmuLa = lltS.solve(schurRhs);

        VectorXd tvec(numPairs);
        for (Index p = 0; p < numPairs; ++p) {
          tvec(p) = rhs(p) - dmuLa(fs->pairSource[static_cast<size_t>(p)])
                    - fs->pairCost(p) * dmuLa(laPos);
        }

        VectorXd d(numPairs + numSources + 1);
        d.head(numPairs) = applyW(*fs, invDt, smCoeff, tvec);
        d.segment(numPairs, numSources) = dmuLa.head(numSources);
        d(numPairs + numSources) = dmuLa(laPos);
        return d;
      };
    };
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
