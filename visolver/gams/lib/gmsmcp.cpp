// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// buildGmsMcp implementation: pair checking, slot assignment, packing, and
// the H/G closures that expand each equation family row-major with the
// shared expression evaluator reading variables from the solver's point.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#include "gmsmcp.hpp"

#include "gmseval.hpp"

#include <cctype>
#include <cmath>
#include <map>
#include <memory>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace VINCP::Gms {

  namespace {

    string
    toLower(const string& text)
    {
      string low = text;
      for (char& c : low) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      }
      return low;
    }

    [[noreturn]] void
    fail(const string& message)
    {
      throw std::invalid_argument("buildGmsMcp: " + message);
    }

    // One eq.var pair, expanded: the equation definition, its loop sets and
    // binding keys (the definition's domain names), and the paired variable.
    struct Family {
      const GmsEquation* equation = nullptr;
      const GmsVariable* variable = nullptr;
      vector<string> bindKeys;          // lower-cased def.domain names
      vector<const GmsSet*> sets;       // base sets, one per dimension
      Index count = 0;
    };

    // Immutable evaluation plan shared by the H and G closures.
    struct Plan {
      const GmsDatabase* db = nullptr;
      vector<Family> freeFamilies;
      vector<Family> posFamilies;
      std::map<const GmsVariable*, GmsMcpSlot> slotByVar;
      Index n = 0;
      Index m = 0;
    };

    Family
    makeFamily(const GmsDatabase& db, const ModelDecl::Pair& pair)
    {
      const GmsEquation& equation = db.equation(pair.eqKey);
      if (!equation.definedP) {
        fail("equation '" + pair.eqName + "' is declared but never defined");
      }
      const GmsVariable& variable = db.variable(pair.varKey);

      const string& relation = equation.def.relation;
      if ("=l=" == relation) {
        fail("equation '" + pair.eqName
             + "' uses =l=, which is outside the censused subset for model "
               "rows");
      }
      if (!variable.positiveP && "=e=" != relation) {
        fail("free variable '" + pair.varName
             + "' must pair an =e= equation, got '" + relation + "' from '"
             + pair.eqName + "'");
      }

      Family family;
      family.equation = &equation;
      family.variable = &variable;
      // Binding names come from the DEFINITION's domain (they are the names
      // the body uses); base sets must match the variable's, dimension by
      // dimension, so both families expand in the same row-major order.
      const vector<string>& eqDomain = equation.def.domain;
      if (eqDomain.size() != variable.domainKeys.size()) {
        std::ostringstream msg;
        msg << "pair " << pair.eqName << "." << pair.varName
            << ": equation rank " << eqDomain.size() << " vs variable rank "
            << variable.domainKeys.size();
        fail(msg.str());
      }
      family.count = 1;
      for (size_t d = 0; d < eqDomain.size(); ++d) {
        const string bindKey = toLower(eqDomain[d]);
        const GmsSet& eqSet = db.resolveSet(bindKey);
        const GmsSet& varSet = db.resolveSet(variable.domainKeys[d]);
        if (&eqSet != &varSet) {
          fail("pair " + pair.eqName + "." + pair.varName + ": dimension "
               + std::to_string(d) + " loops '" + eqSet.name
               + "' on the equation but '" + varSet.name
               + "' on the variable");
        }
        family.bindKeys.push_back(bindKey);
        family.sets.push_back(&eqSet);
        family.count *= static_cast<Index>(eqSet.size());
      }
      return family;
    }

    // Evaluate one block (H over the free families or G over the positive
    // ones): expand each family row-major, binding its definition's domain
    // names, with variable references read from (x, y) via the slots.
    VectorXd
    evalBlock(const Plan& plan, const vector<Family>& families, Index total,
              const VectorXd& x, const VectorXd& y)
    {
      GmsExprEvaluator eval(
          *plan.db,
          [&plan, &x, &y](const GmsVariable& var, const string& attrKey,
                          const vector<size_t>& ordinals) -> double {
            if (!attrKey.empty()) {
              fail("model equations read variables directly; '" + var.name
                   + "." + attrKey + "' is unsupported there");
            }
            const auto slot = plan.slotByVar.find(&var);
            if (plan.slotByVar.end() == slot) {
              fail("equation references variable '" + var.name
                   + "', which no model pair covers");
            }
            // Bounds-checked flat position in the variable's own array shape.
            const size_t flat = var.level.flatIndex(ordinals);
            const Index at =
                slot->second.offset + static_cast<Index>(flat);
            return slot->second.freeP ? x[at] : y[at];
          });

      VectorXd out(total);
      Index row = 0;
      for (const Family& family : families) {
        for (size_t d = 0; d < family.bindKeys.size(); ++d) {
          eval.pushBinding(family.bindKeys[d], *family.sets[d], 0);
        }
        for (Index flat = 0; flat < family.count; ++flat) {
          // Row-major decomposition of `flat` into per-dimension ordinals.
          Index rest = flat;
          for (size_t d = family.bindKeys.size(); 0 < d; --d) {
            const Index extent =
                static_cast<Index>(family.sets[d - 1]->size());
            eval.setBindingOrdinal(family.bindKeys[d - 1],
                                   static_cast<size_t>(rest % extent));
            rest /= extent;
          }
          const double lhs = eval.eval(family.equation->def.lhs);
          const double rhs = eval.eval(family.equation->def.rhs);
          out[row] = lhs - rhs;
          ++row;
        }
        for (size_t d = 0; d < family.bindKeys.size(); ++d) {
          eval.popBinding();
        }
      }
      return out;
    }

  } // namespace

  GmsMcp
  buildGmsMcp(const GmsDatabase& db, const string& modelKey)
  {
    const GmsModel& model = db.model(modelKey);
    if (model.pairs.empty()) {
      fail("model '" + model.name + "' has no eq.var pairs");
    }

    auto plan = std::make_shared<Plan>();
    plan->db = &db;

    std::set<string> usedEquations;
    std::set<string> usedVariables;
    for (const ModelDecl::Pair& pair : model.pairs) {
      if (!usedEquations.insert(pair.eqKey).second) {
        fail("equation '" + pair.eqName + "' appears in two pairs");
      }
      if (!usedVariables.insert(pair.varKey).second) {
        fail("variable '" + pair.varName + "' appears in two pairs");
      }
      const Family family = makeFamily(db, pair);
      if (family.variable->positiveP) {
        plan->posFamilies.push_back(family);
      }
      else {
        plan->freeFamilies.push_back(family);
      }
    }

    GmsMcp mcp;

    // Slots: free families first (the x block), then positive (the y block),
    // each in pair order; offsets are within the block.
    Index freeOffset = 0;
    for (const Family& family : plan->freeFamilies) {
      GmsMcpSlot slot;
      slot.key = toLower(family.variable->name);
      slot.name = family.variable->name;
      slot.freeP = true;
      slot.offset = freeOffset;
      slot.count = family.count;
      plan->slotByVar[family.variable] = slot;
      mcp.slots.push_back(slot);
      freeOffset += family.count;
    }
    Index posOffset = 0;
    for (const Family& family : plan->posFamilies) {
      GmsMcpSlot slot;
      slot.key = toLower(family.variable->name);
      slot.name = family.variable->name;
      slot.freeP = false;
      slot.offset = posOffset;
      slot.count = family.count;
      plan->slotByVar[family.variable] = slot;
      mcp.slots.push_back(slot);
      posOffset += family.count;
    }
    plan->n = freeOffset;
    plan->m = posOffset;

    // Pack z0 and the surfaced upper bounds in slot order.
    mcp.z0 = VectorXd(plan->n + plan->m);
    mcp.upperBounds = VectorXd(plan->n + plan->m);
    auto pack = [&mcp](const Family& family, Index base) {
      const vector<double>& levels = family.variable->level.values;
      const vector<double>& uppers = family.variable->upper.values;
      for (Index i = 0; i < family.count; ++i) {
        mcp.z0[base + i] = levels[static_cast<size_t>(i)];
        mcp.upperBounds[base + i] = uppers[static_cast<size_t>(i)];
        if (std::isfinite(uppers[static_cast<size_t>(i)])) {
          mcp.anyFiniteUpperP = true;
        }
      }
    };
    {
      Index base = 0;
      for (const Family& family : plan->freeFamilies) {
        pack(family, base);
        base += family.count;
      }
      for (const Family& family : plan->posFamilies) {
        pack(family, base);
        base += family.count;
      }
    }

    mcp.model.n = plan->n;
    mcp.model.m = plan->m;
    mcp.model.H = [plan](const VectorXd& x, const VectorXd& y) {
      return evalBlock(*plan, plan->freeFamilies, plan->n, x, y);
    };
    mcp.model.G = [plan](const VectorXd& x, const VectorXd& y) {
      return evalBlock(*plan, plan->posFamilies, plan->m, x, y);
    };
    return mcp;
  }

  void
  applyMcpSolution(GmsDatabase& db, const GmsMcp& mcp, const VectorXd& z)
  {
    if (mcp.model.n + mcp.model.m != z.size()) {
      fail("applyMcpSolution: z has size " + std::to_string(z.size())
           + ", the model needs "
           + std::to_string(mcp.model.n + mcp.model.m));
    }
    for (const GmsMcpSlot& slot : mcp.slots) {
      const auto found = db.variables.find(slot.key);
      if (db.variables.end() == found) {
        fail("applyMcpSolution: no variable '" + slot.key
             + "' in the database");
      }
      GmsVariable& var = found->second;
      const Index base = slot.freeP ? slot.offset : (mcp.model.n + slot.offset);
      for (Index i = 0; i < slot.count; ++i) {
        var.level.values[static_cast<size_t>(i)] = z[base + i];
      }
    }
    return;
  }

} // namespace VINCP::Gms
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
