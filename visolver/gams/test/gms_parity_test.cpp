// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GP4 gate tests: acceptance parity by INDEPENDENT DUAL TRANSCRIPTION. Each
// test hand-codes residual formulas straight from the .gms mathematics in
// plain C++ (no AST, no evaluator) and requires the parsed model's rows to
// match at randomly sampled points (fixed seed; parity must hold at EVERY
// z, so the draws prove generality, not luck). forcepkg is checked in full
// (all 11 affine rows); the nonlinear models are checked on their hardest
// rows -- deploy's macro-built softplus chain above all.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// code is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this code is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#include "gmseval.hpp"
#include "gmsmcp.hpp"
#include "gmsparser.hpp"

#include <gtest/gtest.h>

#include <cmath>
#include <random>
#include <string>
#include <vector>

namespace {

  using namespace VINCP;
  using namespace VINCP::Gms;
  using std::string;
  using std::vector;

  const int kSampleCount = 5;
  const unsigned kSeed = 20260708;

  string
  corpusPath(const string& name)
  {
    return string(VINCP_GMS_CORPUS_DIR) + "/" + name;
  }

  // Relative comparison: the two transcriptions may associate sums
  // differently, so exact bit equality is not owed.
  void
  expectClose(double handValue, double parsedValue, const string& what)
  {
    EXPECT_NEAR(handValue, parsedValue,
                1.0e-9 * (1.0 + std::abs(handValue)))
        << what;
    return;
  }

  // A built corpus model plus the plumbing to read z components and G/H rows
  // by (variable, labels) -- rows pair elementwise with their variables, so
  // the variable's slot and flat index locate BOTH.
  struct Fixture {
    GmsDatabase db;
    GmsMcp mcp;
    std::mt19937 rng{kSeed};

    explicit Fixture(const string& file)
      : db(buildGmsDatabase(parseGmsFile(corpusPath(file))))
      , mcp()
    {
    }

    void
    build(const string& modelKey)
    {
      mcp = buildGmsMcp(db, modelKey);
      return;
    }

    double
    param(const string& key, const vector<string>& labels = {}) const
    {
      const GmsParameter& parameter = db.parameter(key);
      vector<size_t> ordinals;
      for (size_t d = 0; d < labels.size(); ++d) {
        ordinals.push_back(
            db.resolveSet(parameter.domainKeys[d]).ordinalOf(labels[d]));
      }
      return parameter.data.at(ordinals);
    }

    Index
    slotBase(const string& varKey) const
    {
      for (const GmsMcpSlot& slot : mcp.slots) {
        if (varKey == slot.key) {
          return slot.freeP ? slot.offset : (mcp.model.n + slot.offset);
        }
      }
      throw std::invalid_argument("no slot for '" + varKey + "'");
    }

    Index
    zIndex(const string& varKey, const vector<string>& labels = {}) const
    {
      const GmsVariable& var = db.variable(varKey);
      vector<size_t> ordinals;
      for (size_t d = 0; d < labels.size(); ++d) {
        ordinals.push_back(
            db.resolveSet(var.domainKeys[d]).ordinalOf(labels[d]));
      }
      return slotBase(varKey) + static_cast<Index>(
                                    var.level.flatIndex(ordinals));
    }

    // Random point: free components in [-2, 2], nonnegative in [0.1, 3]
    // (strictly positive avoids the corpus models' guarded divisions).
    VectorXd
    samplePoint()
    {
      std::uniform_real_distribution<double> freeDraw(-2.0, 2.0);
      std::uniform_real_distribution<double> posDraw(0.1, 3.0);
      VectorXd z(mcp.model.n + mcp.model.m);
      for (Index i = 0; i < mcp.model.n; ++i) {
        z[i] = freeDraw(rng);
      }
      for (Index i = 0; i < mcp.model.m; ++i) {
        z[mcp.model.n + i] = posDraw(rng);
      }
      return z;
    }

    VectorXd
    gAt(const VectorXd& z) const
    {
      return mcp.model.G(z.head(mcp.model.n), z.tail(mcp.model.m));
    }

    VectorXd
    hAt(const VectorXd& z) const
    {
      return mcp.model.H(z.head(mcp.model.n), z.tail(mcp.model.m));
    }

    // The G/H row paired with a variable element sits at the same offset as
    // the element itself (within its block).
    Index
    rowOf(const string& varKey, const vector<string>& labels = {}) const
    {
      const Index at = zIndex(varKey, labels);
      const GmsVariable& var = db.variable(varKey);
      return var.positiveP ? (at - mcp.model.n) : at;
    }
  };

} // namespace

// -----------------------------------------------------------------------------
// forcepkg_ln, FULL affine parity: every NetCost and Advantage row against
// the hand transcription of the .gms formulas.
// -----------------------------------------------------------------------------

TEST(GmsParity, ForcepkgFullAffineParity)
{
  Fixture fx("forcepkg_ln.gms");
  fx.build("rp");
  const vector<string> fjs = {"1", "2", "3", "4", "5"};
  const vector<string> sks = {"1", "2", "3", "4", "5", "6"};

  for (int sample = 0; sample < kSampleCount; ++sample) {
    const VectorXd z = fx.samplePoint();
    const VectorXd g = fx.gAt(z);

    // NetCost(fj).. cs(fj) - sum(sk, beta(sk)*us(fj,sk)) =g= 0
    for (const string& fj : fjs) {
      double hand = fx.param("cs", {fj});
      for (const string& sk : sks) {
        hand -= z[fx.zIndex("beta", {sk})] * fx.param("us", {fj, sk});
      }
      expectClose(hand, g[fx.rowOf("fs", {fj})], "NetCost(" + fj + ")");
    }
    // Advantage(sk).. sum(fj, us(fj,sk)*fs(fj)) - min_ratio*es(sk) =g= 0
    for (const string& sk : sks) {
      double hand = -fx.param("min_ratio") * fx.param("es", {sk});
      for (const string& fj : fjs) {
        hand += fx.param("us", {fj, sk}) * z[fx.zIndex("fs", {fj})];
      }
      expectClose(hand, g[fx.rowOf("beta", {sk})], "Advantage(" + sk + ")");
    }
  }
}

// -----------------------------------------------------------------------------
// alloceff01cm: the nfDef definition row and the MVInf marginal-value row
// (nested sum over the alias pk, the quadratic gamma denominator).
// -----------------------------------------------------------------------------

TEST(GmsParity, AlloceffRowParity)
{
  Fixture fx("alloceff01cm.gms");
  fx.build("neinf");
  const vector<string> acts = {"A0", "A1", "A2", "A3", "A4", "A5"};
  const vector<string> opts = {"P0", "P1", "P2", "P3", "P4",
                               "P5", "P6", "P7", "P8", "P9"};
  const double alpha = fx.param("alpha");
  const double effR = fx.param("effr");

  for (int sample = 0; sample < kSampleCount; ++sample) {
    const VectorXd z = fx.samplePoint();
    const VectorXd g = fx.gAt(z);

    // nfDef(P0).. nfv(P0) =e= epsilon + sum(act, eff(act,P0))
    double supply = fx.param("epsilon");
    for (const string& act : acts) {
      supply += z[fx.zIndex("eff", {act, "P0"})];
    }
    expectClose(z[fx.zIndex("nfv", {"P0"})] - supply,
                g[fx.rowOf("nfv", {"P0"})], "nfDef(P0)");

    // MVInf(A0,P0).. beta(A0) - (alpha*effR + 2*(1-alpha)*nfv(P0))
    //   * sum(pk, sigma(pk)*(reward(A0,P0)-reward(A0,pk))) / (gamma*gamma)
    const double gamma = z[fx.zIndex("gamma")];
    double swing = 0.0;
    for (const string& pk : opts) {
      swing += z[fx.zIndex("sigma", {pk})]
               * (fx.param("reward", {"A0", "P0"})
                  - fx.param("reward", {"A0", pk}));
    }
    const double hand =
        z[fx.zIndex("beta", {"A0"})]
        - (alpha * effR + 2.0 * (1.0 - alpha) * z[fx.zIndex("nfv", {"P0"})])
              * swing / (gamma * gamma);
    expectClose(hand, g[fx.rowOf("eff", {"A0", "P0"})], "MVInf(A0,P0)");
  }
}

// -----------------------------------------------------------------------------
// deploy_v09: the macro-built softplus chain (C_B_Red_Esc) and a budget row
// (L_M_Red_Log). The hand code below re-derives spx/sgx/fRsurv/FRnode/denx
// from the .gms text -- independently of the $macro expander.
// -----------------------------------------------------------------------------

TEST(GmsParity, DeployRowParity)
{
  Fixture fx("deploy_v09.gms");
  fx.build("interdict");
  const vector<string> reds = {"RL1", "RL2", "RL3"};
  const vector<string> blues = {"BL1", "BL2", "BL3"};
  const vector<string> clocs = {"CL1", "CL2", "CL3", "CL4"};
  const vector<string> bstrats = {"BS1", "BS2", "BS3", "BS4", "BS5"};
  const double aSm = fx.param("asm");
  const double epsNode = fx.param("epsnode");

  const auto sp = [aSm](double t) {
    return std::log(1.0 + std::exp(aSm * t)) / aSm;
  };
  const auto sg = [aSm](double t) {
    return 1.0 / (1.0 + std::exp(-aSm * t));
  };

  for (int sample = 0; sample < kSampleCount; ++sample) {
    const VectorXd z = fx.samplePoint();
    const VectorXd g = fx.gAt(z);
    const auto at = [&](const string& key, const vector<string>& labels) {
      return z[fx.zIndex(key, labels)];
    };

    // Survivor chains at fixed (r = RS1, b), summed over locations.
    const string r = "RS1";
    const auto fRsurv = [&](const string& b, const string& m,
                            const string& k) {
      const double a2 = sp(at("bat", {b, m, k}) - at("res", {r, m, k}));
      const double flow = at("flowr", {r, m, k});
      return flow - a2 + sp(a2 - flow);
    };
    const auto fBsurv = [&](const string& b, const string& n,
                            const string& k) {
      const double a2 = sp(at("rat", {r, n, k}) - at("bes", {b, n, k}));
      const double flow = at("flowb", {b, n, k});
      return flow - a2 + sp(a2 - flow);
    };
    const auto frNode = [&](const string& b, const string& k) {
      double total = 0.0;
      for (const string& m : reds) {
        total += fRsurv(b, m, k);
      }
      return total;
    };
    const auto fbNode = [&](const string& b, const string& k) {
      double total = 0.0;
      for (const string& n : blues) {
        total += fBsurv(b, n, k);
      }
      return total;
    };

    // C_B_Red_Esc(RS1,RL1,CL1).. muER(RS1) - pr(RS1)*sum(b, pb(b)*VR(RS1,CL1)
    //   *(FBnode+epsNode)/sqr(denx)*dfR_dE) =g= 0
    const string m0 = "RL1";
    const string k0 = "CL1";
    double benefit = 0.0;
    for (const string& b : bstrats) {
      const double den = frNode(b, k0) + fbNode(b, k0) + epsNode;
      const double a2 = sp(at("bat", {b, m0, k0}) - at("res", {r, m0, k0}));
      const double dfrde = sg(at("bat", {b, m0, k0}) - at("res", {r, m0, k0}))
                           * (1.0 - sg(a2 - at("flowr", {r, m0, k0})));
      benefit += at("pb", {b}) * fx.param("vr", {r, k0})
                 * (fbNode(b, k0) + epsNode) / (den * den) * dfrde;
    }
    const double escRow = at("muer", {r}) - at("pr", {r}) * benefit;
    expectClose(escRow, g[fx.rowOf("res", {r, m0, k0})],
                "C_B_Red_Esc(RS1,RL1,CL1)");

    // L_M_Red_Log(RS1).. LogR - sum((m,k), DistR(m,k)*flowR(RS1,m,k)) =g= 0
    double miles = fx.param("logr");
    for (const string& m : reds) {
      for (const string& k : clocs) {
        miles -= fx.param("distr", {m, k}) * at("flowr", {r, m, k});
      }
    }
    expectClose(miles, g[fx.rowOf("etar", {r})], "L_M_Red_Log(RS1)");
  }
}

// -----------------------------------------------------------------------------
// pewem01: the CES market-clearing row (the ** operator) and a linear
// profit row.
// -----------------------------------------------------------------------------

TEST(GmsParity, PewemRowParity)
{
  Fixture fx("pewem01.gms");
  fx.build("pewem");
  const vector<string> shippers = {"usap", "rowp"};
  const vector<string> pipers = {"nrwy", "nthr", "eup", "rf"};

  for (int sample = 0; sample < kSampleCount; ++sample) {
    const VectorXd z = fx.samplePoint();
    const VectorXd g = fx.gAt(z);

    // XSupplyNG(eum).. sum(is, qps) + sum(ip, qpp)
    //   - dbase*(pbase/p)**elas =g= 0
    double supply = 0.0;
    for (const string& is : shippers) {
      supply += z[fx.zIndex("qps", {is, "eum"})];
    }
    for (const string& ip : pipers) {
      supply += z[fx.zIndex("qpp", {ip, "eum"})];
    }
    const double demand =
        fx.param("dbase", {"eum"})
        * std::pow(fx.param("pbase", {"eum"}) / z[fx.zIndex("p", {"eum"})],
                   fx.param("elas", {"eum"}));
    expectClose(supply - demand, g[fx.rowOf("p", {"eum"})], "XSupplyNG(eum)");

    // ProdSProfit(usap,eum).. mcps + srate + tau + rps - p =g= 0
    const double profit = fx.param("mcps", {"usap"})
                          + z[fx.zIndex("srate", {"usap", "eum"})]
                          + z[fx.zIndex("tau", {"eum"})]
                          + z[fx.zIndex("rps", {"usap"})]
                          - z[fx.zIndex("p", {"eum"})];
    expectClose(profit, g[fx.rowOf("qps", {"usap", "eum"})],
                "ProdSProfit(usap,eum)");
  }
}

// -----------------------------------------------------------------------------
// glra4B: the flow cost-benefit row (whole-network mu term over 900 flows)
// and the total-movement slack row.
// -----------------------------------------------------------------------------

TEST(GmsParity, Glra4BRowParity)
{
  Fixture fx("glra4B.gms");
  fx.build("glra4b");
  const GmsSet& nodes = fx.db.resolveSet("ni");

  for (int sample = 0; sample < kSampleCount; ++sample) {
    const VectorXd z = fx.samplePoint();
    const VectorXd g = fx.gAt(z);

    double networkMiles = 0.0;   // sum((nn,nm), flow(nn,nm)*dist(nn,nm))
    for (const string& nn : nodes.labels) {
      for (const string& nm : nodes.labels) {
        networkMiles += z[fx.zIndex("flow", {nn, nm})]
                        * fx.param("dist", {nn, nm});
      }
    }

    // CompFlow(N000,N001).. (beta + lambda*dist + phi(ni) + eta(nj)*sigma
    //   + 2*mu*networkMiles) - (eta(ni) + phi(nj)*sigma) =g= 0
    const string ni = "N000";
    const string nj = "N001";
    const double sigma = fx.param("sigma", {ni, nj});
    const double hand =
        z[fx.zIndex("beta", {ni, nj})]
        + z[fx.zIndex("lambda")] * fx.param("dist", {ni, nj})
        + z[fx.zIndex("phi", {ni})] + z[fx.zIndex("eta", {nj})] * sigma
        + 2.0 * fx.param("mu") * networkMiles
        - (z[fx.zIndex("eta", {ni})] + z[fx.zIndex("phi", {nj})] * sigma);
    expectClose(hand, g[fx.rowOf("flow", {ni, nj})], "CompFlow(N000,N001)");

    // SlackMovement.. MoveTotalMax - sum((ni,nj), dist*flow) =g= 0
    expectClose(fx.param("movetotalmax") - networkMiles,
                g[fx.rowOf("lambda")], "SlackMovement");
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
