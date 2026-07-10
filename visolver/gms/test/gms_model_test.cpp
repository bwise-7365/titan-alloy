// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GP3 gate tests: buildGmsMcp over synthetic models and the corpus. The
// synthetic mixed MCP has a unique KNOWN solution, so it verifies the
// eq/var PAIRING ORDER, not just the dimensions; forcepkg_ln (an affine
// LCP) is the end-to-end parse -> build -> solve -> converged gate; the
// other four corpus models are built, dimension-checked against hand
// counts, and evaluated at z0. Upper bounds are surfaced, not applied.
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

namespace {

  using namespace VINCP;
  using namespace VINCP::Gms;
  using std::string;

  const double kFeasTol = 1.0e-6;

  // The test gates below check to 1e-6, so the solver must be ASKED for
  // more than that: solveModelAuto's default stop is a SQUARED natural
  // residual of 1e-10 (norm ~1e-5), which is looser than the gates. Request
  // 1e-14 squared (norm ~1e-7) for a 100x margin.
  AutoModelParams
  tightParams()
  {
    AutoModelParams params;
    params.magTol = 1.0e-14;
    return params;
  }

  string
  corpusPath(const string& name)
  {
    return string(VINCP_GMS_CORPUS_DIR) + "/" + name;
  }

  const GmsMcpSlot&
  slotOf(const GmsMcp& mcp, const string& key)
  {
    for (const GmsMcpSlot& slot : mcp.slots) {
      if (key == slot.key) {
        return slot;
      }
    }
    throw std::invalid_argument("no slot for '" + key + "'");
  }

  // KKT feasibility of a mixed-NCP point: y >= 0, G >= -tol, y.G small.
  void
  expectKktFeasible(const VIModel& model, const VectorXd& z)
  {
    const VectorXd x = z.head(model.n);
    const VectorXd y = z.tail(model.m);
    const VectorXd h = model.H(x, y);
    const VectorXd g = model.G(x, y);
    for (Index i = 0; i < model.n; ++i) {
      EXPECT_NEAR(0.0, h[i], kFeasTol) << "H row " << i;
    }
    double complementarity = 0.0;
    for (Index i = 0; i < model.m; ++i) {
      EXPECT_LE(-1.0e-8, y[i]) << "y " << i;
      EXPECT_LE(-kFeasTol, g[i]) << "G row " << i;
      complementarity += std::abs(y[i] * g[i]);
    }
    // The sum pairs residual-scale errors against the SOLUTION's magnitude
    // (forcepkg's force levels run to thousands), so it scales with ||z||.
    EXPECT_NEAR(0.0, complementarity, kFeasTol * (1.0 + z.norm()));
    return;
  }

} // namespace

// -----------------------------------------------------------------------------
// A synthetic mixed MCP with a unique known solution: x = 3 (free, from
// x - 3 = 0) and y(i) = t(i) > 0 (0 <= y - t perp y >= 0 forces y = t).
// This pins the PAIRING ORDER: a row/variable shuffle would land t
// backwards or fail to converge.
// -----------------------------------------------------------------------------

TEST(GmsModel, SyntheticMixedMcpSolvesToKnownSolution)
{
  const GmsDatabase db = buildGmsDatabase(
      parseGmsString("Set i / a, b /;\n"
                     "Parameter t(i) / a 1.0, b 2.0 /;\n"
                     "Variable x;\n"
                     "Positive Variable y(i);\n"
                     "Equations fixx, comp(i);\n"
                     "fixx.. x - 3.0 =e= 0;\n"
                     "comp(i).. y(i) - t(i) =g= 0;\n"
                     "Model m / fixx.x, comp.y /;\n"
                     "Solve m using MCP;\n"));
  const GmsMcp mcp = buildGmsMcp(db, "m");
  ASSERT_EQ(1, mcp.model.n);
  ASSERT_EQ(2, mcp.model.m);

  const VIResult result = solveModelAuto(mcp.model, mcp.z0, tightParams());
  ASSERT_TRUE(result.converged);
  EXPECT_NEAR(3.0, result.z[0], 1.0e-6);   // x
  EXPECT_NEAR(1.0, result.z[1], 1.0e-6);   // y(a) = t(a)
  EXPECT_NEAR(2.0, result.z[2], 1.0e-6);   // y(b) = t(b)
  expectKktFeasible(mcp.model, result.z);
}

// -----------------------------------------------------------------------------
// forcepkg_ln end to end: parse -> evaluate -> build -> solve -> converged.
// An affine LCP (LP complementarity, skew structure): 5 force levels + 6
// scenario multipliers.
// -----------------------------------------------------------------------------

TEST(GmsModel, ForcepkgSolvesEndToEnd)
{
  const GmsDatabase db = buildGmsDatabase(parseGmsFile(
      corpusPath("forcepkg_ln.gms")));
  const GmsMcp mcp = buildGmsMcp(db, "rp");
  ASSERT_EQ(0, mcp.model.n);
  ASSERT_EQ(11, mcp.model.m);   // fs(5) + beta(6)
  EXPECT_FALSE(mcp.anyFiniteUpperP);

  const VIResult result = solveModelAuto(mcp.model, mcp.z0, tightParams());
  ASSERT_TRUE(result.converged);
  expectKktFeasible(mcp.model, result.z);

  // The advantage rows bind through positive multipliers: some force is
  // bought (fs not identically 0 -- es > 0 forces coverage).
  const GmsMcpSlot& fs = slotOf(mcp, "fs");
  double totalForce = 0.0;
  for (Index i = 0; i < fs.count; ++i) {
    totalForce += result.z[mcp.model.n + fs.offset + i];
  }
  EXPECT_LT(0.1, totalForce);
}

// -----------------------------------------------------------------------------
// The other four corpus models: built, hand-counted dimensions, F(z0)
// finite (evaluateF throws on non-finite), bounds surfaced where set.
// -----------------------------------------------------------------------------

TEST(GmsModel, AlloceffBuildsAndEvaluates)
{
  const GmsDatabase db = buildGmsDatabase(parseGmsFile(
      corpusPath("alloceff01cm.gms")));
  const GmsMcp mcp = buildGmsMcp(db, "neinf");
  EXPECT_EQ(0, mcp.model.n);
  EXPECT_EQ(87, mcp.model.m);   // nfv 10 + sigma 10 + gamma 1 + beta 6 + eff 60
  EXPECT_NO_THROW(evaluateF(mcp.model, mcp.z0));

  // eff.up(act,opt) = weight(act) was surfaced: eff(A0,P0) bound is 68.
  EXPECT_TRUE(mcp.anyFiniteUpperP);
  const GmsMcpSlot& eff = slotOf(mcp, "eff");
  EXPECT_DOUBLE_EQ(68.0, mcp.upperBounds[mcp.model.n + eff.offset]);
}

TEST(GmsModel, DeployBuildsAndEvaluates)
{
  const GmsDatabase db = buildGmsDatabase(parseGmsFile(
      corpusPath("deploy_v09.gms")));
  const GmsMcp mcp = buildGmsMcp(db, "interdict");
  EXPECT_EQ(4, mcp.model.n);     // alphaR, alphaB, lambdaR, lambdaB
  EXPECT_EQ(446, mcp.model.m);   // 450 unknowns total, as in the PPD family
  EXPECT_NO_THROW(evaluateF(mcp.model, mcp.z0));

  EXPECT_TRUE(mcp.anyFiniteUpperP);
  const GmsMcpSlot& flowR = slotOf(mcp, "flowr");
  EXPECT_DOUBLE_EQ(35.0, mcp.upperBounds[mcp.model.n + flowR.offset]);
  // z0 carries the interior start: pr = 1/6.
  const GmsMcpSlot& pr = slotOf(mcp, "pr");
  EXPECT_DOUBLE_EQ(1.0 / 6.0, mcp.z0[mcp.model.n + pr.offset]);
}

TEST(GmsModel, Glra4BAndPewemBuild)
{
  const GmsDatabase glra = buildGmsDatabase(parseGmsFile(
      corpusPath("glra4B.gms")));
  const GmsMcp glraMcp = buildGmsMcp(glra, "glra4b");
  EXPECT_EQ(0, glraMcp.model.n);
  EXPECT_EQ(1951, glraMcp.model.m);   // 30+30+900 primal, 30+900+1+30+30 dual
  EXPECT_NO_THROW(evaluateF(glraMcp.model, glraMcp.z0));

  const GmsDatabase pewem = buildGmsDatabase(parseGmsFile(
      corpusPath("pewem01.gms")));
  const GmsMcp pewemMcp = buildGmsMcp(pewem, "pewem");
  EXPECT_EQ(0, pewemMcp.model.n);
  EXPECT_EQ(43, pewemMcp.model.m);   // dmnd is declared but unpaired: excluded
  EXPECT_NO_THROW(evaluateF(pewemMcp.model, pewemMcp.z0));
}

// -----------------------------------------------------------------------------
// Pairing rules: every rejection is loud.
// -----------------------------------------------------------------------------

TEST(GmsModel, PairingRulesThrow)
{
  const string prologue = "Set i / a /;\n"
                          "Variable x;\n"
                          "Positive Variable y(i);\n";
  // =l= rows are outside the subset for model rows.
  EXPECT_THROW(
      buildGmsMcp(buildGmsDatabase(parseGmsString(
                      prologue + "Equations e(i);\ne(i).. y(i) =l= 1.0;\n"
                                 "Model m / e.y /;\n")),
                  "m"),
      std::invalid_argument);
  // A free variable must pair an =e= row.
  EXPECT_THROW(
      buildGmsMcp(buildGmsDatabase(parseGmsString(
                      prologue + "Equations e;\ne.. x =g= 0.0;\n"
                                 "Model m / e.x /;\n")),
                  "m"),
      std::invalid_argument);
  // A variable cannot appear in two pairs.
  EXPECT_THROW(
      buildGmsMcp(buildGmsDatabase(parseGmsString(
                      prologue + "Equations e(i), f(i);\n"
                                 "e(i).. y(i) =g= 0.0;\n"
                                 "f(i).. y(i) =g= 1.0;\n"
                                 "Model m / e.y, f.y /;\n")),
                  "m"),
      std::invalid_argument);
  // Equation/variable shapes must match dimension by dimension.
  EXPECT_THROW(
      buildGmsMcp(buildGmsDatabase(parseGmsString(
                      prologue + "Equations e;\ne.. sum(i, y(i)) =g= 0.0;\n"
                                 "Model m / e.y /;\n")),
                  "m"),
      std::invalid_argument);
  // A declared-but-never-defined equation cannot be paired.
  EXPECT_THROW(
      buildGmsMcp(buildGmsDatabase(parseGmsString(
                      prologue + "Equations e(i);\nModel m / e.y /;\n")),
                  "m"),
      std::invalid_argument);
  // Every variable an equation references must be paired.
  EXPECT_THROW(
      [] {
        const GmsDatabase db = buildGmsDatabase(parseGmsString(
            "Set i / a /;\n"
            "Positive Variable y(i), w(i);\n"
            "Equations e(i);\n"
            "e(i).. y(i) + w(i) =g= 0.0;\n"
            "Model m / e.y /;\n"));
        const GmsMcp mcp = buildGmsMcp(db, "m");
        evaluateF(mcp.model, mcp.z0);   // the unpaired read fires on eval
      }(),
      std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
