// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GP2 gate tests: buildGmsDatabase over the corpus, checked against
// hand-computed derived values (the gate's exit criterion). Solve is
// recorded, not executed, so post-solve assignments evaluate against the
// INITIAL variable levels -- which makes several of them exactly
// predictable (finalFS = 0 under fs.L = 0; alloceff's prob = 0.1 under the
// symmetric start). Also: symbolic equation validation, model/solve
// records, and loud semantic / non-finite failures.
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
#include "gmsparser.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  using namespace VIMCP::Gms;
  using std::string;
  using std::vector;

  string
  corpusPath(const string& name)
  {
    return string(VIMCP_GMS_CORPUS_DIR) + "/" + name;
  }

  GmsDatabase
  buildFromCorpus(const string& name)
  {
    return buildGmsDatabase(parseGmsFile(corpusPath(name)));
  }

  vector<size_t>
  ordinalsOf(const GmsDatabase& db, const vector<string>& domainKeys,
             const vector<string>& labels)
  {
    vector<size_t> ordinals;
    for (size_t d = 0; d < labels.size(); ++d) {
      ordinals.push_back(db.resolveSet(domainKeys[d]).ordinalOf(labels[d]));
    }
    return ordinals;
  }

  double
  paramValue(const GmsDatabase& db, const string& key,
             const vector<string>& labels = {})
  {
    const GmsParameter& param = db.parameter(key);
    return param.data.at(ordinalsOf(db, param.domainKeys, labels));
  }

  double
  varLevel(const GmsDatabase& db, const string& key,
           const vector<string>& labels = {})
  {
    const GmsVariable& var = db.variable(key);
    return var.level.at(ordinalsOf(db, var.domainKeys, labels));
  }

  double
  varUpper(const GmsDatabase& db, const string& key,
           const vector<string>& labels = {})
  {
    const GmsVariable& var = db.variable(key);
    return var.upper.at(ordinalsOf(db, var.domainKeys, labels));
  }

  double
  varLower(const GmsDatabase& db, const string& key,
           const vector<string>& labels = {})
  {
    const GmsVariable& var = db.variable(key);
    return var.lower.at(ordinalsOf(db, var.domainKeys, labels));
  }

} // namespace

// -----------------------------------------------------------------------------
// forcepkg_ln: exact min_ratio, a hand-summed es entry, zero post-solve
// reports under the default fs.L = 0.
// -----------------------------------------------------------------------------

TEST(GmsEval, ForcepkgDerivedValues)
{
  const GmsDatabase db = buildFromCorpus("forcepkg_ln.gms");

  // min_ratio = sqrt(0.8 / 0.2) = 2 exactly.
  EXPECT_DOUBLE_EQ(2.0, paramValue(db, "min_ratio"));

  // Data spot checks: keyed parameter and table cells.
  EXPECT_DOUBLE_EQ(1.378, paramValue(db, "cs", {"2"}));
  EXPECT_DOUBLE_EQ(0.620, paramValue(db, "us", {"5", "6"}));

  // es('1') = 1.312*109.76 + 2.833*151.35 + 1.336*103.75 + 2.989*376.22,
  // hand-computed: 144.00512 + 428.77455 + 138.61 + 1124.52158.
  EXPECT_NEAR(1835.91125, paramValue(db, "es", {"1"}), 1e-9);

  // Post-solve reports under fs.L = 0: no strength, no win probability.
  EXPECT_DOUBLE_EQ(0.0, paramValue(db, "finalfs", {"1"}));
  EXPECT_DOUBLE_EQ(0.0, paramValue(db, "pwin", {"1"}));
}

// The solve record keeps the method's original spelling.
TEST(GmsEval, ForcepkgSolveRecord)
{
  const GmsDatabase db = buildFromCorpus("forcepkg_ln.gms");
  ASSERT_EQ(1u, db.solves.size());
  EXPECT_EQ("MCP", db.solves[0].method);
  EXPECT_EQ("rp", db.solves[0].modelKey);
  EXPECT_EQ(2u, db.model("rp").pairs.size());
}

// -----------------------------------------------------------------------------
// alloceff01cm: counting sums, exact weight algebra, the symmetric start
// making prob exactly 0.1, and the expected-value row mean.
// -----------------------------------------------------------------------------

TEST(GmsEval, AlloceffDerivedValues)
{
  const GmsDatabase db = buildFromCorpus("alloceff01cm.gms");

  EXPECT_DOUBLE_EQ(6.0, paramValue(db, "numact"));
  EXPECT_DOUBLE_EQ(10.0, paramValue(db, "numopt"));
  EXPECT_DOUBLE_EQ(583.0, paramValue(db, "totalw"));
  EXPECT_DOUBLE_EQ(53.0, paramValue(db, "effr"));   // 583 / 11
  // epsilon = sqrt((68^2+66^2+125^2+101^2+127^2+96^2)/6)/1000, sum = 60151.
  EXPECT_DOUBLE_EQ(std::sqrt(60151.0 / 6.0) / 1000.0,
                   paramValue(db, "epsilon"));

  // Initial values written through .L / .up.
  EXPECT_DOUBLE_EQ(0.5, varLevel(db, "beta", {"A1"}));
  EXPECT_DOUBLE_EQ(68.0 / 11.0, varLevel(db, "eff", {"A0", "P0"}));
  EXPECT_DOUBLE_EQ(125.0, varUpper(db, "eff", {"A2", "P3"}));

  const double epsilon = paramValue(db, "epsilon");
  EXPECT_NEAR(epsilon + 53.0, varLevel(db, "nfv", {"P0"}), 1e-9);
  const double nfv = varLevel(db, "nfv", {"P0"});
  // alpha = 1: sigma = nfv * effR exactly (the quadratic term is 0 * ...).
  EXPECT_NEAR(nfv * 53.0, varLevel(db, "sigma", {"P0"}), 1e-6);
  EXPECT_NEAR(10.0 * varLevel(db, "sigma", {"P0"}), varLevel(db, "gamma"),
              1e-6);

  // The symmetric start makes every option's probability round to exactly
  // 0.1, so expVal(A0) is the A0 reward-row mean: 287.56 / 10.
  EXPECT_DOUBLE_EQ(0.1, paramValue(db, "prob", {"P0"}));
  EXPECT_NEAR(28.756, paramValue(db, "expval", {"A0"}), 1e-9);

  // optfile = 1 recorded on the model.
  EXPECT_DOUBLE_EQ(1.0, db.model("neinf").attrs.at("optfile"));
}

// -----------------------------------------------------------------------------
// glra4B (with its $include data): derived weights, the rho = 0 identity
// sigma == tau, and halved initial levels.
// -----------------------------------------------------------------------------

TEST(GmsEval, Glra4BDerivedValues)
{
  const GmsDatabase db = buildFromCorpus("glra4B.gms");

  EXPECT_DOUBLE_EQ(0.015, paramValue(db, "tinyr"));
  EXPECT_DOUBLE_EQ(0.0, paramValue(db, "rho"));
  EXPECT_DOUBLE_EQ(50000.0, paramValue(db, "movetotalmax"));
  EXPECT_DOUBLE_EQ(2.2097E-11, paramValue(db, "mu"));

  // Wght(N000) = Value / (tinyR + Rqrt)^2 with Value = 6, Rqrt = 322.
  EXPECT_DOUBLE_EQ(6.0 / ((0.015 + 322.0) * (0.015 + 322.0)),
                   paramValue(db, "wght", {"N000"}));

  // rho = 0 makes sigma(ni,nj) = tau(ni,nj) exactly (the loss factor is
  // 2.71828183 ** 0 = 1).
  EXPECT_DOUBLE_EQ(1.0, paramValue(db, "sigma", {"N000", "N001"}));
  EXPECT_DOUBLE_EQ(0.0, paramValue(db, "sigma", {"N000", "N005"}));
  EXPECT_DOUBLE_EQ(paramValue(db, "tau", {"N013", "N019"}),
                   paramValue(db, "sigma", {"N013", "N019"}));

  // Halved / tenth'd initial levels.
  EXPECT_DOUBLE_EQ(100.0, varLevel(db, "qnty", {"N004"}));   // SCap 200 / 2
  EXPECT_DOUBLE_EQ(161.0, varLevel(db, "dlvr", {"N000"}));   // Rqrt 322 / 2
  EXPECT_DOUBLE_EQ(14.5, varLevel(db, "flow", {"N000", "N001"}));   // 145/10

  // Positive multipliers: lower 0, upper +inf.
  EXPECT_DOUBLE_EQ(0.0, varLower(db, "eta", {"N000"}));
  EXPECT_TRUE(std::isinf(varUpper(db, "eta", {"N000"})));
}

// -----------------------------------------------------------------------------
// pewem01: keyed 2-D data, base-year prices as initial levels, and the
// zero post-solve profits under default quantity levels.
// -----------------------------------------------------------------------------

TEST(GmsEval, PewemDerivedValues)
{
  const GmsDatabase db = buildFromCorpus("pewem01.gms");

  EXPECT_DOUBLE_EQ(600.0, paramValue(db, "wwship"));
  EXPECT_DOUBLE_EQ(150.0, paramValue(db, "cpl", {"rf", "eum"}));
  EXPECT_DOUBLE_EQ(0.0, paramValue(db, "cpl", {"nthr", "rowm"}));
  EXPECT_DOUBLE_EQ(28.0, paramValue(db, "mcpp", {"rf"}));

  EXPECT_DOUBLE_EQ(33.0, varLevel(db, "p", {"eum"}));
  EXPECT_DOUBLE_EQ(33.0, varLevel(db, "p", {"rowm"}));
  EXPECT_DOUBLE_EQ(0.0, varLevel(db, "qps", {"usap", "eum"}));

  // psNetProfit = sum(jm, qps.L * rps.L) = 0 under the default levels.
  EXPECT_DOUBLE_EQ(0.0, paramValue(db, "psnetprofit", {"usap"}));
}

// -----------------------------------------------------------------------------
// deploy_v09: card() arithmetic in the initial values, pool bounds, the
// macro-built equations validated and recorded, and post-solve identities
// (SentR = TotalR, LogUsedR = mean flow times total distance).
// -----------------------------------------------------------------------------

TEST(GmsEval, DeployDerivedValues)
{
  const GmsDatabase db = buildFromCorpus("deploy_v09.gms");

  EXPECT_DOUBLE_EQ(35.0, paramValue(db, "totalr"));
  EXPECT_DOUBLE_EQ(0.25, paramValue(db, "asm"));
  EXPECT_DOUBLE_EQ(0.5, paramValue(db, "epsnode"));
  EXPECT_DOUBLE_EQ(8.3, paramValue(db, "vr", {"RS5", "CL4"}));
  EXPECT_DOUBLE_EQ(20.0, paramValue(db, "distb", {"BL3", "CL1"}));

  // Initial values built from card(): 35/(3+1), 1/(5+1), 35/(3*4), 10/13.
  EXPECT_DOUBLE_EQ(8.75, varLevel(db, "rd", {"RL1"}));
  EXPECT_DOUBLE_EQ(1.0 / 6.0, varLevel(db, "pr", {"RS2"}));
  EXPECT_DOUBLE_EQ(35.0 / 12.0, varLevel(db, "flowr", {"RS1", "RL1", "CL1"}));
  EXPECT_DOUBLE_EQ(10.0 / 13.0, varLevel(db, "res", {"RS1", "RL1", "CL1"}));
  EXPECT_DOUBLE_EQ(35.0, varUpper(db, "flowr", {"RS1", "RL1", "CL1"}));
  EXPECT_DOUBLE_EQ(11.0, varUpper(db, "bat", {"BS1", "RL1", "CL1"}));

  // Free multipliers really are free.
  EXPECT_TRUE(std::isinf(varLower(db, "alphar")));
  EXPECT_DOUBLE_EQ(0.0, varLevel(db, "alphar"));

  // Post-solve identities at the symmetric start: everything is sent, and
  // route-miles = (TotalR/12) * (sum of all DistR entries = 178).
  EXPECT_NEAR(26.25, paramValue(db, "totalrd"), 1e-12);   // 3 * 8.75
  EXPECT_NEAR(35.0, paramValue(db, "sentr", {"RS1"}), 1e-9);
  EXPECT_NEAR(35.0 / 12.0 * 178.0, paramValue(db, "logusedr", {"RS1"}), 1e-6);

  // The macro-built equations were validated and stored; the model and its
  // optfile arrived.
  const GmsEquation& flowEq = db.equation("c_b_red_flow");
  EXPECT_TRUE(flowEq.definedP);
  ASSERT_EQ(3u, flowEq.domainKeys.size());
  EXPECT_EQ(24u, db.model("interdict").pairs.size());
  EXPECT_DOUBLE_EQ(0.0, db.model("interdict").attrs.at("optfile"));
  ASSERT_EQ(1u, db.solves.size());
}

// -----------------------------------------------------------------------------
// Loud failures: semantic problems throw invalid_argument; a computed
// non-finite value throws runtime_error.
// -----------------------------------------------------------------------------

TEST(GmsEval, SemanticErrorsThrow)
{
  // Equation referencing an undeclared symbol.
  EXPECT_THROW(
      buildGmsDatabase(parseGmsString("Set i / a /;\n"
                                      "Positive Variable x(i);\n"
                                      "Equations e(i);\n"
                                      "e(i).. x(i) - y(i) =g= 0;\n")),
      std::invalid_argument);
  // Arity mismatch inside an equation.
  EXPECT_THROW(
      buildGmsDatabase(parseGmsString("Set i / a /;\n"
                                      "Positive Variable x(i);\n"
                                      "Equations e(i);\n"
                                      "e(i).. x(i, i) =g= 0;\n")),
      std::invalid_argument);
  // Data entry with a label outside the domain set.
  EXPECT_THROW(buildGmsDatabase(parseGmsString("Set i / a /;\n"
                                               "Parameter p(i) / b 1.0 /;\n")),
               std::invalid_argument);
  // Assignment index that is neither a set/alias nor a label.
  EXPECT_THROW(buildGmsDatabase(parseGmsString("Set i / a /;\n"
                                               "Parameter p(i);\n"
                                               "p(j) = 1.0;\n")),
               std::invalid_argument);
  // Model pairing an undeclared variable.
  EXPECT_THROW(
      buildGmsDatabase(parseGmsString("Set i / a /;\n"
                                      "Positive Variable x(i);\n"
                                      "Equations e(i);\n"
                                      "e(i).. x(i) =g= 0;\n"
                                      "Model m / e.z /;\n")),
      std::invalid_argument);
}

// A parameter declared without a domain takes its shape from its first
// assignment (GAMS universal-domain behavior; deploy_v09's LogUsedR).
TEST(GmsEval, OpenDomainParameterTakesShapeFromFirstAssignment)
{
  const GmsDatabase db =
      buildGmsDatabase(parseGmsString("Set i / a, b /;\n"
                                      "Parameter u 'declared bare';\n"
                                      "u(i) = 2.0;\n"
                                      "Parameter w;\n"
                                      "w = 3.0;\n"));
  EXPECT_DOUBLE_EQ(2.0, paramValue(db, "u", {"a"}));
  EXPECT_DOUBLE_EQ(2.0, paramValue(db, "u", {"b"}));
  EXPECT_DOUBLE_EQ(3.0, paramValue(db, "w"));
  // Once fixed, the shape binds: a scalar assignment to u now mismatches.
  EXPECT_THROW(buildGmsDatabase(parseGmsString("Set i / a /;\n"
                                               "Parameter u;\n"
                                               "u(i) = 2.0;\n"
                                               "u = 3.0;\n")),
               std::invalid_argument);
}

TEST(GmsEval, NonFiniteComputationThrows)
{
  EXPECT_THROW(buildGmsDatabase(parseGmsString("Scalar s / 0.0 /;\n"
                                               "Parameter q;\n"
                                               "q = 1.0 / s;\n")),
               std::runtime_error);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
