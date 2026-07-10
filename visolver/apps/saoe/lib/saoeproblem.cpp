// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// SAOE problem class implementation: dispatch to the library's solver engines
// and decode the equilibrium into the pinned aggregates plus one effort split.
// ----------------------------------------------
#include "saoeproblem.hpp"

#include "alternatingchain.hpp"
#include "chooseengine.hpp"
#include "fbshyz04.hpp"
#include "josephynewton.hpp"
#include "saoe.hpp"
#include "semismoothnewton.hpp"
#include "smoothingnewton.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <limits>
#include <numeric>
#include <random>
#include <stdexcept>
#include <utility>
#include <vector>

namespace VINCP::App {

  namespace {

    // BFS path a -> b in the (undirected) spanning-forest adjacency.
    std::vector<int>
    treePath(int a, int b, const std::vector<std::vector<int>>& adj)
    {
      const std::size_t n = adj.size();
      std::vector<int> prev(n, -1);
      std::vector<char> seen(n, 0);
      std::vector<int> queue;
      queue.push_back(a);
      seen[static_cast<std::size_t>(a)] = 1;
      std::size_t head = 0;
      while (head < queue.size()) {
        const int u = queue[head++];
        if (u == b) {
          break;
        }
        for (const int v : adj[static_cast<std::size_t>(u)]) {
          if (0 == seen[static_cast<std::size_t>(v)]) {
            seen[static_cast<std::size_t>(v)] = 1;
            prev[static_cast<std::size_t>(v)] = u;
            queue.push_back(v);
          }
        }
      }
      std::vector<int> path;
      for (int cur = b; cur != -1; cur = prev[static_cast<std::size_t>(cur)]) {
        path.push_back(cur);
        if (cur == a) {
          break;
        }
      }
      std::reverse(path.begin(), path.end());   // now a -> b
      return path;
    }

    // Cancel the cycle given by the ordered node list `path` (a party/option
    // sequence) plus the closing edge back to path[0]. Alternating +/- around the
    // even-length bipartite cycle preserves every row and column sum and zeroes
    // at least one edge. `numParties` splits party nodes (< it) from option nodes.
    void
    cancelCycle(MatrixXd& e, const std::vector<int>& path, int numParties, double tol)
    {
      const std::size_t len = path.size();
      const auto entryOf = [numParties](int u, int v) -> std::pair<Index, Index> {
        const int party  = (u < numParties) ? u : v;
        const int option = ((u < numParties) ? v : u) - numParties;
        return { static_cast<Index>(party), static_cast<Index>(option) };
      };
      double delta = std::numeric_limits<double>::infinity();
      for (std::size_t i = 0; i < len; ++i) {
        if (1 == i % 2) {
          const auto [p, o] = entryOf(path[i], path[(i + 1) % len]);
          delta = std::min(delta, e(p, o));
        }
      }
      for (std::size_t i = 0; i < len; ++i) {
        const auto [p, o] = entryOf(path[i], path[(i + 1) % len]);
        if (0 == i % 2) {
          e(p, o) += delta;
        }
        else {
          e(p, o) -= delta;
        }
        if (e(p, o) < tol) {
          e(p, o) = 0.0;   // the min "-" edge (and any dust) leaves the support
        }
      }
      return;
    }

  } // namespace

  SAOE::SAOE(SaoeData instanceData)
    : data(std::move(instanceData))
  {
    return;
  }

  const std::vector<ProblemBase::Engine>&
  SAOE::honoredEngines()
  {
    // Chain first: it is SAOE's default (Engine::Default resolves to it) and the
    // only configuration shown to reach the reference equilibrium E. This list
    // is the single source of truth for the solve guard and the CLIs; keep it in
    // step with the dispatch in solve().
    static const std::vector<ProblemBase::Engine> engines = {
        ProblemBase::Engine::Chain,
        ProblemBase::Engine::Auto,
        ProblemBase::Engine::SmoothingNewton,
        ProblemBase::Engine::Fbs,
    };
    return engines;
  }

  SaoeData
  SAOE::generate(const SaoeRandomSpec& spec)
  {
    if (spec.numActors <= 0 || spec.numOptions <= 0) {
      throw std::invalid_argument(
          "SAOE::generate: numActors and numOptions must be positive.");
    }
    if (spec.rewardHi < spec.rewardLo || spec.strengthHi < spec.strengthLo) {
      throw std::invalid_argument(
          "SAOE::generate: an upper bound is below its lower bound.");
    }

    std::mt19937_64 rng(spec.seed);
    std::uniform_real_distribution<double> rewardDist(spec.rewardLo, spec.rewardHi);
    std::uniform_real_distribution<double> strengthDist(spec.strengthLo, spec.strengthHi);

    SaoeData out;
    out.R.resize(spec.numActors, spec.numOptions);
    for (int i = 0; i < spec.numActors; ++i) {
      for (int j = 0; j < spec.numOptions; ++j) {
        out.R(i, j) = rewardDist(rng);
      }
    }
    out.S.resize(spec.numActors);
    for (int i = 0; i < spec.numActors; ++i) {
      double s = strengthDist(rng);
      if (spec.roundStrengthTenthsP) {
        s = std::round(s * 10.0) / 10.0;
      }
      out.S(i) = s;
    }
    return out;
  }

  const char*
  SAOE::answerContractNote()
  {
    return
        "At the SAOE equilibrium the per-option aggregates -- the option\n"
        "probabilities, and hence each actor's expected reward -- are unique\n"
        "(pinned). The per-actor effort split that realizes them is NOT unique:\n"
        "the effort matrix below is one representative of the solution set.";
  }

  // ---------------------------------------------------------------------------
  // The robust default: the alternating globalizer/finisher chain, in exactly
  // the configuration the saoe_chain_test proves reaches equilibrium E. This is
  // a production copy of the test-only makeSaoeChain recipe (test/saoesupport.hpp):
  // Josephy-Newton over an interior-point inner solver as globalizer (under the
  // no-progress cutoff), then semismooth Newton with nonmonotone memory as
  // finisher.
  // ---------------------------------------------------------------------------
  static VIResult
  solveSaoeChain(const VIModel& model, const VectorXd& z0, const SaoeParams& params)
  {
    JosephyNewtonParams jnParams;
    jnParams.outerTol     = params.magTol;
    jnParams.outerIterMax = params.jnOuterIterMax;
    jnParams.stallIterMax = params.jnStallIterMax;
    const InnerSolver inner =
        makeMehrotraIpmSolver(model.n, params.ipmInnerMagTol, params.ipmInnerIterMax, 0);
    const StageSolver globalizer = [&model, inner, jnParams](const VectorXd& start) {
      return solveVI(model, start, inner, jnParams);
    };

    SemismoothNewtonParams ssnParams;
    ssnParams.magTol            = params.magTol;
    ssnParams.iterMax           = params.ssnIterMax;
    ssnParams.nonmonotoneMemory = params.ssnNonmonotoneMemory;
    const StageSolver finisher = [&model, ssnParams](const VectorXd& start) {
      return semismoothNewtonSolve(model, start, ssnParams);
    };

    AlternatingChainParams chainParams;
    chainParams.magTol       = params.magTol;
    chainParams.roundsMax    = params.chainRoundsMax;
    chainParams.perturbScale = params.chainPerturbScale;

    ChainStageLogger stageLog = ChainStageLogger{};
    if (params.verbose) {
      stageLog = [](int round, const char* stage, double stageResidual,
                    double bestResidual, const string& note) {
        if (note.empty()) {
          std::printf("    saoe-chain round %d %s: residual^2 %.3e (best %.3e)\n",
                      round, stage, stageResidual, bestResidual);
        }
        else {
          std::printf("    saoe-chain round %d %s threw (stalled stage): %s\n",
                      round, stage, note.c_str());
        }
        std::fflush(stdout);
        return;
      };
    }

    return alternatingChainSolve(model, z0, globalizer, finisher, chainParams, stageLog);
  }

  // The chooseEngine dispatcher: semismooth first, alternating-chain fallback.
  static VIResult
  solveSaoeAuto(const VIModel& model, const VectorXd& z0, const SaoeParams& params)
  {
    AutoModelParams autoParams;
    autoParams.magTol               = params.magTol;
    autoParams.ssnIterMax           = params.ssnIterMax;
    autoParams.ssnNonmonotoneMemory = params.ssnNonmonotoneMemory;
    autoParams.jnOuterIterMax       = params.jnOuterIterMax;
    autoParams.jnStallIterMax       = params.jnStallIterMax;
    autoParams.ipmInnerMagTol       = params.ipmInnerMagTol;
    autoParams.ipmInnerIterMax      = params.ipmInnerIterMax;
    autoParams.chainRoundsMax       = params.chainRoundsMax;
    autoParams.chainPerturbScale    = params.chainPerturbScale;
    if (params.verbose) {
      autoParams.onChoice = [](EngineChoice, const char* reason) {
        std::printf("    saoe-auto: %s\n", reason);
        std::fflush(stdout);
        return;
      };
    }
    return solveModelAuto(model, z0, autoParams);
  }

  // Forward-backward splitting (He-Yuan-Zhang 2004): a matrix-free,
  // non-interior-path solve of the VI over K directly on the model's field. It
  // already returns a VIResult in the model's z-layout with the squared natural
  // residual, so it drops straight into the decode path.
  static VIResult
  solveSaoeFbs(const VIModel& model, const VectorXd& z0, const SaoeParams& params)
  {
    const int kFbsIterMax = 200000;   // FBS is cheap per iteration (2 F-evals)
    const VectorField field = [&model](const VectorXd& z) {
      return evaluateF(model, z);
    };
    const Projector projector = makeMixedProjector(model.n);
    return fbsHyz04(z0, field, projector, params.magTol, kFbsIterMax, 0);
  }

  // Non-interior smoothing Newton (Zhang-Liu-Liu). It solves the mixed NCP from
  // (H, G) and returns z = [u, x, y, s]. SAOE is a PURE orthant NCP (model.n ==
  // 0), but smoothingNewtonSolve requires a non-empty free block, so we embed the
  // problem with ONE trivial free variable pinned to zero by H(x, y) = x: the
  // complementarity block G(y) -- and hence the solution y -- is unchanged, and
  // the dummy variable is discarded on decode. The result is repacked into the
  // model's z = y and its squared natural residual recomputed, so it is
  // comparable with the other engines.
  static VIResult
  solveSaoeSmoothing(const VIModel& model, const VectorXd& z0, const SaoeParams& params)
  {
    const Index m = model.m;
    const MixedField pinnedH = [](const VectorXd& x, const VectorXd&) { return x; };
    const MixedField orthantG = [&model](const VectorXd&, const VectorXd& y) {
      return model.G(VectorXd(0), y);
    };
    const VIResult raw =
        smoothingNewtonSolve(pinnedH, orthantG, VectorXd::Zero(1), z0.tail(m));
    const SmoothingSolution decoded = smoothingDecode(raw, 1, m);

    const VectorXd z = decoded.y;   // SAOE z has no free block
    const Projector projector = makeMixedProjector(model.n);
    const VectorXd natural = z - projector(z - evaluateF(model, z));

    VIResult out;
    out.z          = z;
    out.residual   = natural.squaredNorm();
    out.iter       = raw.iter;
    out.innerIters = raw.innerIters;
    out.converged  = (out.residual < params.magTol);
    return out;
  }

  SAOE::Solution
  SAOE::solve(const Params& params) const
  {
    const VIModel model =
        saoeModel(data.R, data.S, params.riskAversion, params.epsWeight);
    const VectorXd z0 = saoeDefaultStart(data.R, data.S);

    VIResult vi;
    switch (params.engine) {
      case ProblemBase::Engine::Default:
      case ProblemBase::Engine::Chain:
        vi = solveSaoeChain(model, z0, params);
        break;
      case ProblemBase::Engine::Auto:
        vi = solveSaoeAuto(model, z0, params);
        break;
      case ProblemBase::Engine::SmoothingNewton:
        vi = solveSaoeSmoothing(model, z0, params);
        break;
      case ProblemBase::Engine::Fbs:
        vi = solveSaoeFbs(model, z0, params);
        break;
      default:
        throw std::invalid_argument(
            std::string("SAOE::solve: engine ") + engineName(params.engine)
            + " is not honored; use one of: "
            + engineTokenList(honoredEngines()) + ".");
    }

    const Index M = data.R.rows();
    const Index N = data.R.cols();
    const SaoeSolution decoded = saoeDecode(vi, M, N);
    const double eps = (0.0 < params.epsWeight) ? params.epsWeight : saoeEps(data.R);

    SaoeResult result;
    result.probabilities = saoeProbabilities(decoded.e, eps);
    result.utilities     = saoeUtilities(data.R, decoded.e, eps);
    result.e             = decoded.e;
    result.lambda        = decoded.lambda;

    return Solution{ vi, result };
  }

  MatrixXd
  sparsifyEffortMatrix(const MatrixXd& effort)
  {
    MatrixXd e = effort;
    const Index M = e.rows();
    const Index N = e.cols();
    if (0 == e.size()) {
      return e;
    }
    const double tol = std::max(1.0e-12, 1.0e-9 * e.maxCoeff());
    const int nodes = static_cast<int>(M + N);   // parties 0..M-1, options M..M+N-1
    const long cap = static_cast<long>(M) * static_cast<long>(N) + 1;

    for (long guard = 0; guard <= cap; ++guard) {
      std::vector<int> parent(static_cast<std::size_t>(nodes));
      std::iota(parent.begin(), parent.end(), 0);
      const auto find = [&parent](int x) {
        while (parent[static_cast<std::size_t>(x)] != x) {
          parent[static_cast<std::size_t>(x)] =
              parent[static_cast<std::size_t>(parent[static_cast<std::size_t>(x)])];
          x = parent[static_cast<std::size_t>(x)];
        }
        return x;
      };
      std::vector<std::vector<int>> adj(static_cast<std::size_t>(nodes));

      // Scan support entries, building a spanning forest; the first entry whose
      // endpoints are already connected closes a cycle -> cancel it and restart.
      bool cancelledP = false;
      for (Index m = 0; m < M && !cancelledP; ++m) {
        for (Index j = 0; j < N; ++j) {
          if (e(m, j) <= tol) {
            continue;
          }
          const int a = static_cast<int>(m);
          const int b = static_cast<int>(M + j);
          const int ra = find(a);
          const int rb = find(b);
          if (ra != rb) {
            parent[static_cast<std::size_t>(ra)] = rb;
            adj[static_cast<std::size_t>(a)].push_back(b);
            adj[static_cast<std::size_t>(b)].push_back(a);
          }
          else {
            cancelCycle(e, treePath(a, b, adj), static_cast<int>(M), tol);
            cancelledP = true;
            break;
          }
        }
      }
      if (!cancelledP) {
        return e;   // support is a forest -> a vertex (identity if it never moved)
      }
    }
    throw std::logic_error(
        "sparsifyEffortMatrix: could not reach a vertex (support remained cyclic).");
  }

  SaoeResult
  SAOE::sparsify(const SaoeResult& result) const
  {
    // Identity-or-project: the effort matrix is driven to a vertex of its
    // transportation polytope; the probabilities, utilities, and lambda (the
    // pinned quantities and the multiplier) are carried over unchanged.
    SaoeResult out = result;
    out.e = sparsifyEffortMatrix(result.e);
    return out;
  }

} // namespace VINCP::App
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
