// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GP5 gate tests, step 1: reproduce the recorded GAMS solutions for the two
// models whose solves are fast -- forcepkg_ln against doc/
// forcepkg_ln.solve.lst (MILES, Optimal; exact levels) and pewem01 against
// doc/pewem01.solve.lst (MILES, Optimal; gated on the DEGENERACY-ROBUST
// aggregates -- both markets price identically, so per-route ship splits
// are not unique). The three expensive solves (alloceff, deploy, glra4B)
// go through the bounded gms_gp5_probe executable first, fleet-track
// style; their gates land after the probe measures them.
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

#include "chooseengine.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  using namespace VINCP;
  using namespace VINCP::Gms;
  using std::string;
  using std::vector;

  // The listings print three decimals, and the solves run at magTol 1e-14
  // (residual norm ~1e-7): 0.02 absolute plus a small relative term covers
  // both rounding and solve error without hiding a wrong vertex.
  double
  levelTol(double reference)
  {
    return 0.02 + 1.0e-5 * std::abs(reference);
  }

  string
  corpusPath(const string& name)
  {
    return string(VINCP_GMS_CORPUS_DIR) + "/" + name;
  }

  AutoModelParams
  tightParams()
  {
    AutoModelParams params;
    params.magTol = 1.0e-14;
    return params;
  }

  struct Solved {
    GmsDatabase db;
    GmsMcp mcp;
    VIResult result;

    explicit Solved(const string& file, const string& modelKey)
      : db(buildGmsDatabase(parseGmsFile(corpusPath(file))))
      , mcp()
      , result()
    {
      mcp = buildGmsMcp(db, modelKey);
      result = solveModelAuto(mcp.model, mcp.z0, tightParams());
    }

    double
    level(const string& varKey, const vector<string>& labels = {}) const
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
          return result.z[base
                          + static_cast<Index>(var.level.flatIndex(ordinals))];
        }
      }
      throw std::invalid_argument("no slot for '" + varKey + "'");
    }
  };

} // namespace

// -----------------------------------------------------------------------------
// forcepkg_ln vs doc/forcepkg_ln.solve.lst (MILES, Model Status 1 Optimal):
// fs = (0, 471.718, 1819.606, 855.367, 0), beta = (0, 0, 0.696, 0.169, 0,
// 0.444).
// -----------------------------------------------------------------------------

TEST(GmsSolve, ForcepkgReproducesGamsLevels)
{
  const Solved solved("forcepkg_ln.gms", "rp");
  ASSERT_TRUE(solved.result.converged);

  const vector<double> fsRef = {0.0, 471.718, 1819.606, 855.367, 0.0};
  const vector<double> betaRef = {0.0, 0.0, 0.696, 0.169, 0.0, 0.444};
  const vector<string> fjs = {"1", "2", "3", "4", "5"};
  const vector<string> sks = {"1", "2", "3", "4", "5", "6"};
  for (size_t j = 0; j < fjs.size(); ++j) {
    EXPECT_NEAR(fsRef[j], solved.level("fs", {fjs[j]}), levelTol(fsRef[j]))
        << "fs(" << fjs[j] << ")";
  }
  for (size_t k = 0; k < sks.size(); ++k) {
    EXPECT_NEAR(betaRef[k], solved.level("beta", {sks[k]}),
                levelTol(betaRef[k]))
        << "beta(" << sks[k] << ")";
  }
}

// -----------------------------------------------------------------------------
// pewem01 vs doc/pewem01.solve.lst (MILES, Model Status 1 Optimal), gated
// on degeneracy-robust quantities: prices, terminal volumes, the rf
// pipeline flows, fees/rates, and per-market ship totals. Per-route qps is
// NOT gated: both markets price at 33, so a shipper is indifferent
// between them and the split is not unique.
// -----------------------------------------------------------------------------

TEST(GmsSolve, PewemReproducesGamsAggregates)
{
  const Solved solved("pewem01.gms", "pewem");
  ASSERT_TRUE(solved.result.converged);

  EXPECT_NEAR(33.0, solved.level("p", {"eum"}), levelTol(33.0));
  EXPECT_NEAR(33.0, solved.level("p", {"rowm"}), levelTol(33.0));
  EXPECT_NEAR(75.0, solved.level("vol", {"eum"}), levelTol(75.0));
  EXPECT_NEAR(450.0, solved.level("vol", {"rowm"}), levelTol(450.0));
  EXPECT_NEAR(150.0, solved.level("qpp", {"rf", "eum"}), levelTol(150.0));
  EXPECT_NEAR(50.0, solved.level("qpp", {"rf", "rowm"}), levelTol(50.0));
  EXPECT_NEAR(1.0, solved.level("tau", {"eum"}), levelTol(1.0));
  EXPECT_NEAR(1.0, solved.level("tau", {"rowm"}), levelTol(1.0));
  EXPECT_NEAR(0.0, solved.level("rs"), levelTol(0.0));
  for (const string& is : {string("usap"), string("rowp")}) {
    for (const string& jm : {string("eum"), string("rowm")}) {
      EXPECT_NEAR(2.0, solved.level("srate", {is, jm}), levelTol(2.0))
          << "srate(" << is << "," << jm << ")";
    }
  }
  // Per-market ship totals (robust even though the split is degenerate).
  const double eumShipped = solved.level("qps", {"usap", "eum"})
                            + solved.level("qps", {"rowp", "eum"});
  const double rowmShipped = solved.level("qps", {"usap", "rowm"})
                             + solved.level("qps", {"rowp", "rowm"});
  EXPECT_NEAR(75.0, eumShipped, levelTol(75.0));
  EXPECT_NEAR(450.0, rowmShipped, levelTol(450.0));

  // The write-back + post-solve rerun machinery: psNetProfit recomputes
  // from the SOLVED levels (usap earns rent 1 on 50 shipped units...
  // whatever the degenerate split, profit = rps * total shipped by usap).
  GmsDatabase db = buildGmsDatabase(parseGmsFile(corpusPath("pewem01.gms")));
  const GmsMcp mcp = buildGmsMcp(db, "pewem");
  const VIResult result = solveModelAuto(mcp.model, mcp.z0, tightParams());
  ASSERT_TRUE(result.converged);
  applyMcpSolution(db, mcp, result.z);
  rerunPostSolveAssignments(db, parseGmsFile(corpusPath("pewem01.gms")));
  const GmsParameter& profit = db.parameter("psnetprofit");
  const double usapProfit = profit.data.values[0];
  const double usapShipped = db.variable("qps").level.values[0]
                             + db.variable("qps").level.values[1];
  const double usapRent = db.variable("rps").level.values[0];
  EXPECT_NEAR(usapRent * usapShipped, usapProfit,
              levelTol(usapRent * usapShipped));
}

// -----------------------------------------------------------------------------
// alloceff01cm vs doc/alloceff01cm.solve.lst (PATH, Model Status 1
// Optimal), gated on the AGGREGATE equilibrium: gamma and the per-option
// totals nfv / sigma (support {P2,P3,P4,P5,P9}; the floor is epsilon ~
// 0.1001, which the listing rounds to 0.100). The actor-level attribution
// (beta, eff) is NOT gated: the 2026-07-08 probe showed the per-option
// totals matching PATH to listing rounding while the individual efforts --
// and hence the per-actor shadow prices beta -- split differently, the
// aggregative-game analogue of pewem's degenerate ship split.
// -----------------------------------------------------------------------------

TEST(GmsSolve, AlloceffReproducesPathAggregates)
{
  const Solved solved("alloceff01cm.gms", "neinf");
  ASSERT_TRUE(solved.result.converged);

  EXPECT_NEAR(30952.067, solved.level("gamma"), levelTol(30952.067));
  const vector<double> nfvRef = {0.100, 0.100,   101.100, 66.100, 164.100,
                                 127.100, 0.100, 0.100,   0.100,  125.100};
  const vector<double> sigmaRef = {5.307,    5.307,    5358.307, 3503.307,
                                   8697.307, 6736.307, 5.307,    5.307,
                                   5.307,    6630.307};
  const vector<string> opts = {"P0", "P1", "P2", "P3", "P4",
                               "P5", "P6", "P7", "P8", "P9"};
  for (size_t p = 0; p < opts.size(); ++p) {
    EXPECT_NEAR(nfvRef[p], solved.level("nfv", {opts[p]}),
                levelTol(nfvRef[p]))
        << "nfv(" << opts[p] << ")";
    EXPECT_NEAR(sigmaRef[p], solved.level("sigma", {opts[p]}),
                levelTol(sigmaRef[p]))
        << "sigma(" << opts[p] << ")";
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
