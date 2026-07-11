// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Structured fleet Newton factory implementation: transcription of the
// machine-verified algebra in network/doc/fleet-newton-check.mac.
// ----------------------------------------------
#include "fleetnewton.hpp"

#include <memory>     // shared_ptr: one structural snapshot shared by every factorization
#include <stdexcept>

namespace VIMCP::Network {

  namespace {

    // Structural snapshot of the fleet LCP, fixed for the factory's
    // lifetime: everything the per-iteration factorizations need except
    // sOverY.
    struct FleetStructure {
      Index numVars = 0;
      Index numSupplyCells = 0;
      Index numTypes = 0;
      vector<Index> varMu;        // supply-cell index of variable p
      vector<Index> varType;      // vehicle type of variable p
      VectorXd varRho;            // rho_p, aligned with the y block
      vector<Index> cellStart;    // demand-cell boundaries; sentinel numVars last
      vector<double> cellQuad;    // Q_cell per cell, from M's diagonal
    };

    FleetStructure
    buildStructure(const FleetLcp& lcp)
    {
      if (0 == lcp.numVars || 0 == lcp.numSupplyCells || 0 == lcp.numTypes) {
        throw std::invalid_argument(
            "makeFleetNewtonFactory: lcp has no variables, supply cells, or "
            "types.");
      }
      const size_t numVars = static_cast<size_t>(lcp.numVars);
      if (lcp.varMuIndex.size() != numVars || lcp.varType.size() != numVars
          || lcp.varCell.size() != numVars
          || lcp.varRho.size() != lcp.numVars
          || lcp.varQuad.size() != lcp.numVars) {
        throw std::invalid_argument(
            "makeFleetNewtonFactory: lcp variable arrays disagree with "
            "numVars.");
      }
      // The dense M is optional (matrix-free builds leave it empty); when
      // present it must at least have the right shape.
      const Index dim = lcp.numVars + lcp.numSupplyCells + lcp.numTypes;
      if (0 != lcp.M.size() && (lcp.M.rows() != dim || lcp.M.cols() != dim)) {
        throw std::invalid_argument(
            "makeFleetNewtonFactory: lcp.M has the wrong dimensions.");
      }

      FleetStructure fs;
      fs.numVars = lcp.numVars;
      fs.numSupplyCells = lcp.numSupplyCells;
      fs.numTypes = lcp.numTypes;
      fs.varMu = lcp.varMuIndex;
      fs.varType = lcp.varType;
      fs.varRho = lcp.varRho;

      // Demand cells: buildFleetLcp enumerates y cell-major, so each cell's
      // variables are one contiguous run of equal varCell. Q_cell comes from
      // varQuad (present in both dense and matrix-free builds) and must be
      // positive and constant across the cell.
      for (Index p = 0; p < lcp.numVars; ++p) {
        const Index mu = lcp.varMuIndex[static_cast<size_t>(p)];
        if (0 > mu || lcp.numSupplyCells <= mu) {
          throw std::invalid_argument(
              "makeFleetNewtonFactory: variable mu index out of range.");
        }
        const Index type = lcp.varType[static_cast<size_t>(p)];
        if (0 > type || lcp.numTypes <= type) {
          throw std::invalid_argument(
              "makeFleetNewtonFactory: variable type index out of range.");
        }
        if (!(0.0 < lcp.varRho(p))) {
          throw std::invalid_argument(
              "makeFleetNewtonFactory: non-positive rho weight.");
        }
        const Index cell = lcp.varCell[static_cast<size_t>(p)];
        if (0 == p || cell != lcp.varCell[static_cast<size_t>(p - 1)]) {
          if (0 < p && cell < lcp.varCell[static_cast<size_t>(p - 1)]) {
            throw std::invalid_argument(
                "makeFleetNewtonFactory: variables are not "
                "cell-major-contiguous.");
          }
          const double quad = lcp.varQuad(p);
          if (!(0.0 < quad)) {
            throw std::invalid_argument(
                "makeFleetNewtonFactory: non-positive Q in lcp.varQuad.");
          }
          fs.cellStart.push_back(p);
          fs.cellQuad.push_back(quad);
        }
        else if (lcp.varQuad(p) != fs.cellQuad.back()) {
          throw std::invalid_argument(
              "makeFleetNewtonFactory: varQuad is not constant across a "
              "demand cell.");
        }
      }
      fs.cellStart.push_back(lcp.numVars);   // sentinel: end of the last cell
      return fs;
    }

    // y = W x on the y block, W = (Qblk + diag(Dy))^{-1}, applied per demand
    // cell in O(k_cell) by the Sherman-Morrison form (fleet-newton-check.mac,
    // checks 1 and 3b-2): y|_c = x .* invDy - c_c (u_c . x) u_c with
    // u_c = invDy on the cell and c_c = Q_c / (1 + Q_c sigma_c).
    VectorXd
    applyW(const FleetStructure& fs, const VectorXd& invDy,
           const vector<double>& smCoeff, const VectorXd& x)
    {
      VectorXd y = x.cwiseProduct(invDy);
      for (size_t n = 0; n + 1 < fs.cellStart.size(); ++n) {
        const Index lo = fs.cellStart[n];
        const Index len = fs.cellStart[n + 1] - lo;
        const double dot = invDy.segment(lo, len).dot(x.segment(lo, len));
        y.segment(lo, len) -= (smCoeff[n] * dot) * invDy.segment(lo, len);
      }
      return y;
    }

  } // namespace

  NewtonSolverFactory
  makeFleetNewtonFactory(const FleetLcp& lcp)
  {
    const auto fs =
        std::make_shared<const FleetStructure>(buildStructure(lcp));

    return [fs](const VectorXd& sOverY,
                double freeRegularization) -> NewtonSolve {
      if (0.0 != freeRegularization) {
        throw std::invalid_argument(
            "makeFleetNewtonFactory: the fleet LCP has no free block, so "
            "freeRegularization must be 0.");
      }
      const Index numVars = fs->numVars;
      const Index numDual = fs->numSupplyCells + fs->numTypes;
      if (sOverY.size() != numVars + numDual) {
        throw std::invalid_argument(
            "makeFleetNewtonFactory: sOverY has the wrong size for this "
            "lcp.");
      }
      if (!(0.0 < sOverY.minCoeff())) {
        throw std::invalid_argument(
            "makeFleetNewtonFactory: sOverY must be strictly positive.");
      }

      // Sherman-Morrison data for W on this iteration's Dy.
      const VectorXd invDy = sOverY.head(numVars).cwiseInverse();
      vector<double> smCoeff(fs->cellQuad.size(), 0.0);
      for (size_t n = 0; n + 1 < fs->cellStart.size(); ++n) {
        const Index lo = fs->cellStart[n];
        const Index len = fs->cellStart[n + 1] - lo;
        const double sigma = invDy.segment(lo, len).sum();
        smCoeff[n] = fs->cellQuad[n] / (1.0 + fs->cellQuad[n] * sigma);
      }

      // Dual Schur complement S = diag(Dmu | Dla) + B^T W B, B = [E R],
      // assembled in two parts (fleet-newton-check.mac, check 3b-3):
      // B^T diag(invDy) B accumulated per variable (each row of B has two
      // nonzeros: 1 at its supply cell, rho_p at its type), minus the
      // per-cell rank-one downdates c_c (B^T u_c)(B^T u_c)^T on the cell's
      // (distinct sources + distinct types) support.
      MatrixXd S = MatrixXd::Zero(numDual, numDual);
      S.diagonal() = sOverY.segment(numVars, numDual);
      for (Index p = 0; p < numVars; ++p) {
        const Index m = fs->varMu[static_cast<size_t>(p)];
        const Index l =
            fs->numSupplyCells + fs->varType[static_cast<size_t>(p)];
        const double a = invDy(p);
        const double rho = fs->varRho(p);
        S(m, m) += a;
        S(m, l) += rho * a;
        S(l, m) += rho * a;
        S(l, l) += rho * rho * a;
      }
      VectorXd g = VectorXd::Zero(numDual);
      vector<char> markP(static_cast<size_t>(numDual), 0);
      vector<Index> touched;
      for (size_t n = 0; n + 1 < fs->cellStart.size(); ++n) {
        const Index lo = fs->cellStart[n];
        const Index hi = fs->cellStart[n + 1];
        touched.clear();
        for (Index p = lo; p < hi; ++p) {
          const Index m = fs->varMu[static_cast<size_t>(p)];
          const Index l =
              fs->numSupplyCells + fs->varType[static_cast<size_t>(p)];
          if (0 == markP[static_cast<size_t>(m)]) {
            markP[static_cast<size_t>(m)] = 1;
            touched.push_back(m);
          }
          if (0 == markP[static_cast<size_t>(l)]) {
            markP[static_cast<size_t>(l)] = 1;
            touched.push_back(l);
          }
          g(m) += invDy(p);
          g(l) += fs->varRho(p) * invDy(p);
        }
        const double c = smCoeff[n];
        for (const Index i : touched) {
          for (const Index j : touched) {
            S(i, j) -= c * g(i) * g(j);
          }
        }
        for (const Index i : touched) {
          g(i) = 0.0;
          markP[static_cast<size_t>(i)] = 0;
        }
      }

      LLT<MatrixXd> lltS(S);
      if (Success != lltS.info()) {
        throw std::runtime_error(
            "makeFleetNewtonFactory: the dual Schur complement failed its "
            "LLT (not numerically positive definite).");
      }

      // One factorization, shared by the predictor and corrector solves:
      // S [dmu; dla] = [rmu + E^T W ry ; rla + R^T W ry], then back out
      // dy = W (ry - E dmu - R dla) (fleet-newton-check.mac, checks
      // 3a/3b-4/3c).
      return [fs, invDy, smCoeff, lltS](const VectorXd& rhs) -> VectorXd {
        const Index numVars = fs->numVars;
        const Index numDual = fs->numSupplyCells + fs->numTypes;
        if (rhs.size() != numVars + numDual) {
          throw std::invalid_argument(
              "makeFleetNewtonFactory: rhs has the wrong size for this "
              "lcp.");
        }
        const VectorXd wry = applyW(*fs, invDy, smCoeff, rhs.head(numVars));

        VectorXd schurRhs = rhs.segment(numVars, numDual);
        for (Index p = 0; p < numVars; ++p) {
          schurRhs(fs->varMu[static_cast<size_t>(p)]) += wry(p);
          schurRhs(fs->numSupplyCells
                   + fs->varType[static_cast<size_t>(p)]) +=
              fs->varRho(p) * wry(p);
        }
        const VectorXd dual = lltS.solve(schurRhs);

        VectorXd yvec(numVars);
        for (Index p = 0; p < numVars; ++p) {
          yvec(p) = rhs(p) - dual(fs->varMu[static_cast<size_t>(p)])
                    - fs->varRho(p)
                          * dual(fs->numSupplyCells
                                 + fs->varType[static_cast<size_t>(p)]);
        }

        VectorXd d(numVars + numDual);
        d.head(numVars) = applyW(*fs, invDy, smCoeff, yvec);
        d.tail(numDual) = dual;
        return d;
      };
    };
  }

} // namespace VIMCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
