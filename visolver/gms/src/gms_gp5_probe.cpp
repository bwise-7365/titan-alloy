// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GP5 bounded probe (run manually, fleet-benchmark style; NOT a ctest
// gate): attempt the expensive corpus solves with per-iteration heartbeats
// and comparisons against the recorded GAMS solutions. Engines are called
// DIRECTLY (no solveModelAuto): a measurement probe must stay bounded, and
// the auto path's chain fallback is unbounded by design.
//
//   gms_gp5_probe [alloceff|deploy|glra4b|all] [iterMax] [magTol]
//
// Defaults: all 50 1e-10. alloceff / deploy run the semismooth engine with
// heartbeats (the default 4th-order FD Jacobian costs ~4*dim F-evaluations
// PER ITERATION -- the heartbeat shows where the time goes). glra4B is
// AFFINE, so the probe assembles the exact constant M column by column
// (dim+1 F-evaluations ONCE), verifies affinity at a random point, and
// runs the interior-point engine on (M, q) -- engine class matched to
// problem class. Suggested runs:
//   gms_gp5_probe alloceff
//   gms_gp5_probe deploy 20
//   gms_gp5_probe glra4b
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#include "gmseval.hpp"
#include "gmsmcp.hpp"
#include "gmsparser.hpp"

#include "mehrotraipm.hpp"
#include "semismoothnewton.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <functional>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  using namespace VIMCP;
  using namespace VIMCP::Gms;
  using std::cout;
  using std::string;
  using std::vector;

  using Clock = std::chrono::steady_clock;

  double
  secondsSince(Clock::time_point start)
  {
    return std::chrono::duration<double>(Clock::now() - start).count();
  }

  struct Reference {
    string varKey;
    vector<string> labels;
    double value;
  };

  double
  levelOf(const GmsDatabase& db, const GmsMcp& mcp, const VectorXd& z,
          const string& varKey, const vector<string>& labels)
  {
    const GmsVariable& var = db.variable(varKey);
    vector<size_t> ordinals;
    for (size_t d = 0; d < labels.size(); ++d) {
      ordinals.push_back(
          db.resolveSet(var.domainKeys[d]).ordinalOf(labels[d]));
    }
    for (const GmsMcpSlot& slot : mcp.slots) {
      if (slot.key == varKey) {
        const Index base =
            slot.freeP ? slot.offset : (mcp.model.n + slot.offset);
        return z[base + static_cast<Index>(var.level.flatIndex(ordinals))];
      }
    }
    throw std::invalid_argument("no slot for '" + varKey + "'");
  }

  void
  feasibilitySummary(const VIModel& model, const VectorXd& z)
  {
    const VectorXd x = z.head(model.n);
    const VectorXd y = z.tail(model.m);
    const VectorXd h = model.H(x, y);
    const VectorXd g = model.G(x, y);
    double hMax = 0.0;
    for (Index i = 0; i < model.n; ++i) {
      hMax = std::max(hMax, std::abs(h[i]));
    }
    double yMin = 0.0;
    double gMin = 0.0;
    double comp = 0.0;
    for (Index i = 0; i < model.m; ++i) {
      yMin = std::min(yMin, y[i]);
      gMin = std::min(gMin, g[i]);
      comp += std::abs(y[i] * g[i]);
    }
    cout << "  feasibility: max|H| " << hMax << ", min y " << yMin
         << ", min G " << gMin << ", sum|y.G| " << comp << "\n";
    return;
  }

  void
  compareToReference(const GmsDatabase& db, const GmsMcp& mcp,
                     const VectorXd& z, const vector<Reference>& refs)
  {
    double worst = 0.0;
    for (const Reference& ref : refs) {
      const double ours = levelOf(db, mcp, z, ref.varKey, ref.labels);
      const double dev = std::abs(ours - ref.value);
      worst = std::max(worst, dev);
      cout << "  " << ref.varKey;
      for (const string& label : ref.labels) {
        cout << "." << label;
      }
      cout << ": ours " << ours << " vs GAMS " << ref.value << " (dev "
           << dev << ")\n";
    }
    cout << "  worst deviation vs the GAMS listing: " << worst
         << (worst < 1.0e-2 ? "  -- MATCHES" : "  -- DIFFERENT point")
         << "\n";
    return;
  }

  // Heartbeat logger: iteration, squared residual, elapsed seconds.
  IterationLogger
  heartbeat(Clock::time_point start)
  {
    return [start](int iter, int iterMax, double mag, double magTol) {
      cout << "  [iter " << iter << "/" << iterMax << "] mag " << mag
           << " (tol " << magTol << "), elapsed " << secondsSince(start)
           << " s\n"
           << std::flush;
      return;
    };
  }

  using Reporter = std::function<void(GmsDatabase& db, const GmsMcp& mcp,
                                      const VectorXd& z)>;

  struct Built {
    GmsDatabase db;
    GmsMcp mcp;

    Built(const string& file, const string& modelKey)
      : db(buildGmsDatabase(parseGmsFile(string(VIMCP_GMS_CORPUS_DIR) + "/"
                                         + file)))
      , mcp()
    {
      mcp = buildGmsMcp(db, modelKey);
    }
  };

  void
  finish(Built& built, const VIResult& result, const vector<Reference>& refs,
         const Reporter& reporter)
  {
    feasibilitySummary(built.mcp.model, result.z);
    if (!refs.empty()) {
      compareToReference(built.db, built.mcp, result.z, refs);
    }
    if (reporter) {
      reporter(built.db, built.mcp, result.z);
    }
    cout << std::flush;
    return;
  }

  // Semismooth probe (alloceff, deploy): direct engine call, heartbeat per
  // iteration, NO fallback.
  void
  runSsnProbe(const string& name, const string& file, const string& modelKey,
              const vector<Reference>& refs, int iterMax, double magTol,
              const Reporter& reporter = Reporter())
  {
    cout << "==== " << name << " (semismooth, iterMax " << iterMax
         << ") ====\n"
         << std::flush;
    const auto start = Clock::now();
    Built built(file, modelKey);
    cout << "  built: n " << built.mcp.model.n << ", m " << built.mcp.model.m
         << " in " << secondsSince(start) << " s\n";

    const auto fStart = Clock::now();
    evaluateF(built.mcp.model, built.mcp.z0);
    const double fSeconds = secondsSince(fStart);
    const Index dim = built.mcp.model.n + built.mcp.model.m;
    cout << "  one F evaluation: " << fSeconds << " s; the default FD "
         << "Jacobian needs ~" << 4 * dim << " of them per iteration (~"
         << 4 * static_cast<double>(dim) * fSeconds << " s/iter)\n"
         << std::flush;

    SemismoothNewtonParams params;
    params.magTol = magTol;
    params.iterMax = iterMax;
    params.iterFreq = 1;
    params.nonmonotoneMemory = 4;
    const auto solveStart = Clock::now();
    try {
      const VIResult result = semismoothNewtonSolve(
          built.mcp.model, built.mcp.z0, params, heartbeat(solveStart));
      cout << "  solve: " << secondsSince(solveStart) << " s, converged "
           << (result.converged ? "YES" : "NO") << ", residual "
           << result.residual << " (squared), iters " << result.iter << "\n";
      finish(built, result, refs, reporter);
    }
    catch (const std::exception& ex) {
      cout << "  solve THREW after " << secondsSince(solveStart)
           << " s: " << ex.what() << "\n"
           << std::flush;
    }
    return;
  }

  // Affine probe (glra4B): assemble the exact constant M once, verify
  // affinity, and run the interior-point engine on (M, q).
  void
  runAffineIpmProbe(const string& name, const string& file,
                    const string& modelKey, int iterMax, double magTol,
                    const Reporter& reporter = Reporter())
  {
    cout << "==== " << name << " (affine + interior point) ====\n"
         << std::flush;
    const auto start = Clock::now();
    Built built(file, modelKey);
    const Index dim = built.mcp.model.n + built.mcp.model.m;
    cout << "  built: n " << built.mcp.model.n << ", m " << built.mcp.model.m
         << " in " << secondsSince(start) << " s\n"
         << std::flush;

    // q = F(0); M column j = F(e_j) - q. Exact when F is affine.
    const auto assembleStart = Clock::now();
    const VectorXd zero = VectorXd::Zero(dim);
    const VectorXd q = evaluateF(built.mcp.model, zero);
    MatrixXd M(dim, dim);
    for (Index j = 0; j < dim; ++j) {
      VectorXd unit = VectorXd::Zero(dim);
      unit[j] = 1.0;
      M.col(j) = evaluateF(built.mcp.model, unit) - q;
      if (0 == (j + 1) % 100 || dim == j + 1) {
        cout << "  [assemble] column " << (j + 1) << "/" << dim
             << ", elapsed " << secondsSince(assembleStart) << " s\n"
             << std::flush;
      }
    }

    // Affinity check at a random point: F(z) must equal M z + q.
    std::mt19937 rng(20260708);
    std::uniform_real_distribution<double> draw(0.1, 3.0);
    VectorXd probe(dim);
    for (Index i = 0; i < dim; ++i) {
      probe[i] = draw(rng);
    }
    const double affineError =
        (evaluateF(built.mcp.model, probe) - (M * probe + q)).norm()
        / (1.0 + q.norm());
    cout << "  affinity check: relative deviation " << affineError << "\n";
    if (1.0e-8 < affineError) {
      cout << "  NOT affine -- skipping the interior-point path.\n"
           << std::flush;
      return;
    }

    const auto solveStart = Clock::now();
    try {
      const VIResult result =
          mehrotraIpm(M, q, built.mcp.model.n, magTol, iterMax, 1,
                      MehrotraIpmParams{}, heartbeat(solveStart));
      cout << "  ipm solve: " << secondsSince(solveStart) << " s, converged "
           << (result.converged ? "YES" : "NO") << ", residual "
           << result.residual << " (squared), iters " << result.iter << "\n";
      finish(built, result, {}, reporter);
    }
    catch (const std::exception& ex) {
      cout << "  ipm solve THREW after " << secondsSince(solveStart)
           << " s: " << ex.what() << "\n"
           << std::flush;
    }
    return;
  }

} // namespace

int
main(int argc, char** argv)
{
  const string which = (1 < argc) ? argv[1] : "all";
  const int iterMax = (2 < argc) ? std::atoi(argv[2]) : 50;
  const double magTol = (3 < argc) ? std::atof(argv[3]) : 1.0e-10;
  if ("all" != which && "alloceff" != which && "deploy" != which
      && "glra4b" != which) {
    cout << "gms_gp5_probe: unknown model '" << which << "'\n"
         << "usage: gms_gp5_probe [alloceff|deploy|glra4b|all] [iterMax] "
            "[magTol]\n";
    return 1;
  }
  if (iterMax <= 0 || !(0.0 < magTol) || 1.0 <= magTol) {
    cout << "gms_gp5_probe: iterMax must be positive and magTol in (0, 1); "
         << "got iterMax " << iterMax << ", magTol " << magTol << "\n";
    return 1;
  }
  cout << "gms_gp5_probe: " << which << ", iterMax " << iterMax
       << ", magTol " << magTol << " (squared)\n"
       << std::flush;

  if ("all" == which || "alloceff" == which) {
    // PATH equilibrium (doc/alloceff01cm.solve.lst): support {P2,P3,P4,
    // P5,P9}; nfv floor is epsilon ~ 0.1001. The 2026-07-08 probe run
    // matched all AGGREGATES; only the actor-level attribution differed.
    runSsnProbe("alloceff01cm", "alloceff01cm.gms", "neinf",
                {{"gamma", {}, 30952.067},
                 {"beta", {"A0"}, 0.282},
                 {"beta", {"A4"}, 0.580},
                 {"nfv", {"P2"}, 101.100},
                 {"nfv", {"P4"}, 164.100},
                 {"nfv", {"P0"}, 0.100}},
                iterMax, magTol);
  }
  if ("all" == which || "deploy" == which) {
    // PATH equilibrium (doc/deploy_v09.solve.lst): pr = (0,.4,.6,0,0),
    // pb = (0,0,0,.4,.6), RD = (0,0,35), BD = (18.637,18.363,0).
    runSsnProbe("deploy_v09", "deploy_v09.gms", "interdict",
                {{"pr", {"RS2"}, 0.400},
                 {"pr", {"RS3"}, 0.600},
                 {"pb", {"BS4"}, 0.400},
                 {"pb", {"BS5"}, 0.600},
                 {"rd", {"RL3"}, 35.000},
                 {"bd", {"BL1"}, 18.637},
                 {"alphar", {}, 54.118},
                 {"alphab", {}, 51.394}},
                iterMax, magTol);
  }
  if ("all" == which || "glra4b" == which) {
    // No clean listing (the MILES .lst is the flagged budget-violating
    // interrupt); the .gms comments' PATH/NLPEC targets are the
    // comparators: delivered 527.556 and shortfalls N000 13.977,
    // N007 3.256, N019 2.440, N024 4.373. The reporter reruns the
    // post-solve assignments at the solved point and prints them.
    runAffineIpmProbe(
        "glra4B", "glra4B.gms", "glra4b", std::max(iterMax, 100), magTol,
        [](GmsDatabase& db, const GmsMcp& mcp, const VectorXd& z) {
          applyMcpSolution(db, mcp, z);
          rerunPostSolveAssignments(
              db, parseGmsFile(string(VIMCP_GMS_CORPUS_DIR) + "/glra4B.gms"));
          cout << "  TotalDlvrd " << db.parameter("totaldlvrd").data.values[0]
               << " (PATH/NLPEC 527.556)\n"
               << "  TotalUsed " << db.parameter("totalused").data.values[0]
               << " (budget 50000)\n"
               << "  ValAchieved "
               << db.parameter("valachieved").data.values[0] << "\n";
          const GmsParameter& shortfall = db.parameter("shortfall");
          const GmsSet& nodes = db.resolveSet("nj");
          cout << "  Shortfall(%) above 0.5:";
          for (size_t i = 0; i < nodes.size(); ++i) {
            if (0.5 < std::abs(shortfall.data.values[i])) {
              cout << " " << nodes.labels[i] << " "
                   << shortfall.data.values[i];
            }
          }
          cout << "  (PATH: N000 13.977, N007 3.256, N019 2.440, "
                  "N024 4.373)\n";
        });
  }
  cout << "gms_gp5_probe done.\n";
  return 0;
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
