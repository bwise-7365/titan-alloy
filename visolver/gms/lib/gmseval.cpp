// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GmsExprEvaluator and buildGmsDatabase implementations. The expression
// evaluator is shared: the GP2 statement walker reads variable ATTRIBUTES
// through it (bare variables are illegal in assignments), and the GP3 model
// builder reads bare variables from the solver's current point.
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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace VIMCP::Gms {

  namespace {

    const double kInf = std::numeric_limits<double>::infinity();

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
    evalFail(const string& message)
    {
      throw std::invalid_argument("GMS evaluation: " + message);
    }

  } // namespace

  // ---------------------------------------------------------------------------
  // GmsExprEvaluator
  // ---------------------------------------------------------------------------

  GmsExprEvaluator::GmsExprEvaluator(const GmsDatabase& db,
                                     VariableReader reader)
    : db_(db)
    , reader_(reader)
  {
    if (!reader_) {
      throw std::invalid_argument(
          "GmsExprEvaluator: the variable reader must be set");
    }
  }

  void
  GmsExprEvaluator::pushBinding(const string& key, const GmsSet& set,
                                size_t ordinal)
  {
    env_.push_back(Binding{key, &set, ordinal});
    return;
  }

  void
  GmsExprEvaluator::setBindingOrdinal(const string& key, size_t ordinal)
  {
    for (size_t i = env_.size(); 0 < i; --i) {
      if (env_[i - 1].key == key) {
        env_[i - 1].ordinal = ordinal;
        return;
      }
    }
    evalFail("no binding named '" + key + "' to retarget");
  }

  void
  GmsExprEvaluator::popBinding()
  {
    env_.pop_back();
    return;
  }

  const GmsExprEvaluator::Binding*
  GmsExprEvaluator::findBinding(const string& key) const
  {
    for (size_t i = env_.size(); 0 < i; --i) {
      if (env_[i - 1].key == key) {
        return &env_[i - 1];
      }
    }
    return nullptr;
  }

  vector<size_t>
  GmsExprEvaluator::ordinalsFor(const string& symbolName,
                                const vector<string>& domainKeys,
                                const vector<string>& indices) const
  {
    if (indices.size() != domainKeys.size()) {
      std::ostringstream msg;
      msg << "'" << symbolName << "' takes " << domainKeys.size()
          << " indices, got " << indices.size();
      evalFail(msg.str());
    }
    vector<size_t> ordinals;
    for (size_t d = 0; d < indices.size(); ++d) {
      const string key = toLower(indices[d]);
      const Binding* binding = findBinding(key);
      if (nullptr != binding) {
        ordinals.push_back(binding->ordinal);
        continue;
      }
      // Literal label of the d-th domain set.
      ordinals.push_back(db_.resolveSet(domainKeys[d]).ordinalOf(indices[d]));
    }
    return ordinals;
  }

  double
  GmsExprEvaluator::eval(const Expr& expr)
  {
    switch (expr.kind) {
    case Expr::Kind::Number:
      return expr.number;
    case Expr::Kind::Unary: {
      const double value = eval(expr.args[0]);
      return ("-" == expr.op) ? -value : value;
    }
    case Expr::Kind::Binary:
      return evalBinary(expr);
    case Expr::Kind::Call:
      return evalCall(expr);
    case Expr::Kind::Sum:
      return sumOver(expr, 0);
    case Expr::Kind::SymbolRef: {
      const auto param = db_.parameters.find(expr.key);
      if (db_.parameters.end() != param) {
        return param->second.data.at(ordinalsFor(
            expr.name, param->second.domainKeys, expr.indices));
      }
      const auto var = db_.variables.find(expr.key);
      if (db_.variables.end() != var) {
        return reader_(var->second, string(),
                       ordinalsFor(expr.name, var->second.domainKeys,
                                   expr.indices));
      }
      evalFail("reference to undeclared symbol '" + expr.name + "'");
    }
    case Expr::Kind::AttrRef: {
      const auto var = db_.variables.find(expr.key);
      if (db_.variables.end() == var) {
        evalFail("'" + expr.name + "' is not a variable (attribute '"
                 + expr.attr + "' read)");
      }
      return reader_(var->second, expr.attrKey,
                     ordinalsFor(expr.name, var->second.domainKeys,
                                 expr.indices));
    }
    default:
      break;
    }
    evalFail("unsupported expression kind");
  }

  double
  GmsExprEvaluator::evalBinary(const Expr& expr)
  {
    const double a = eval(expr.args[0]);
    const double b = eval(expr.args[1]);
    if ("+" == expr.op) {
      return a + b;
    }
    if ("-" == expr.op) {
      return a - b;
    }
    if ("*" == expr.op) {
      return a * b;
    }
    if ("/" == expr.op) {
      return a / b;
    }
    if ("**" == expr.op) {
      return std::pow(a, b);
    }
    evalFail("unsupported operator '" + expr.op + "'");
  }

  double
  GmsExprEvaluator::evalCall(const Expr& expr)
  {
    const string& fn = expr.key;
    if ("card" == fn) {
      if (1 != expr.args.size() || Expr::Kind::SymbolRef != expr.args[0].kind
          || !expr.args[0].indices.empty()) {
        evalFail("card() takes one set (or alias) name");
      }
      return static_cast<double>(db_.resolveSet(expr.args[0].key).size());
    }
    vector<double> args;
    for (const Expr& arg : expr.args) {
      args.push_back(eval(arg));
    }
    if ("exp" == fn) {
      return std::exp(args.at(0));
    }
    if ("log" == fn) {
      return std::log(args.at(0));
    }
    if ("sqrt" == fn) {
      return std::sqrt(args.at(0));
    }
    if ("sqr" == fn) {
      return args.at(0) * args.at(0);
    }
    if ("abs" == fn) {
      return std::fabs(args.at(0));
    }
    if ("power" == fn) {
      return std::pow(args.at(0), args.at(1));
    }
    if ("max" == fn || "min" == fn) {
      if (args.empty()) {
        evalFail(fn + "() needs at least one argument");
      }
      double best = args[0];
      for (const double value : args) {
        best = ("max" == fn) ? std::max(best, value) : std::min(best, value);
      }
      return best;
    }
    if ("round" == fn) {
      if (1 == args.size()) {
        return std::round(args[0]);
      }
      if (2 == args.size()) {
        const double scale = std::pow(10.0, args[1]);
        return std::round(args[0] * scale) / scale;
      }
      evalFail("round() takes one or two arguments");
    }
    evalFail("unsupported function '" + expr.name + "'");
  }

  double
  GmsExprEvaluator::sumOver(const Expr& expr, size_t dim)
  {
    if (expr.sumIndices.size() == dim) {
      return eval(expr.args[0]);
    }
    const string key = toLower(expr.sumIndices[dim]);
    const GmsSet& set = db_.resolveSet(key);
    double total = 0.0;
    pushBinding(key, set, 0);
    for (size_t ordinal = 0; ordinal < set.size(); ++ordinal) {
      env_.back().ordinal = ordinal;
      total += sumOver(expr, dim + 1);
    }
    popBinding();
    return total;
  }

  // ---------------------------------------------------------------------------
  // buildGmsDatabase
  // ---------------------------------------------------------------------------

  namespace {

    // Executes ONE assignment statement (parameter, variable attribute, or
    // model attribute), looping indexed lvalues over their domains. Shared
    // by the full statement walk (Evaluator) and rerunPostSolveAssignments.
    class AssignmentExecutor {
    public:
      explicit AssignmentExecutor(GmsDatabase& db)
        : db_(db)
        , expr_(db, [this](const GmsVariable& var, const string& attrKey,
                           const vector<size_t>& ordinals) {
            return this->readVariableAttr(var, attrKey, ordinals);
          })
      {
      }

      void
      execute(const Assignment& assign)
      {
        // Model attribute (m.optfile = 1) goes to the model record.
        if (!assign.attr.empty() && 0 < db_.models.count(assign.key)) {
          if (!assign.indices.empty()) {
            evalFail("a model attribute takes no indices");
          }
          db_.models[assign.key].attrs[assign.attrKey] =
              expr_.eval(assign.value);
          return;
        }

        GmsArray* target = nullptr;
        vector<string> domainKeys;
        if (assign.attr.empty()) {
          const auto found = db_.parameters.find(assign.key);
          if (db_.parameters.end() == found) {
            evalFail("assignment to undeclared parameter '" + assign.name
                     + "'");
          }
          GmsParameter& param = found->second;
          if (param.domainOpenP) {
            // First assignment to a domain-less parameter fixes its shape
            // (rank 0 when unindexed): every index must name a set/alias.
            param.domainOpenP = false;
            if (!assign.indices.empty()) {
              vector<string> keys;
              for (const string& indexName : assign.indices) {
                const string key = toLower(indexName);
                if (!db_.setP(key)) {
                  evalFail("first assignment to domain-less parameter '"
                           + assign.name + "' must index by sets, got '"
                           + indexName + "'");
                }
                keys.push_back(key);
              }
              param.domainKeys = keys;
              param.data = GmsArray::filled(shapeOf(keys), 0.0);
            }
          }
          target = &param.data;
          domainKeys = param.domainKeys;
        }
        else {
          const auto found = db_.variables.find(assign.key);
          if (db_.variables.end() == found) {
            evalFail("attribute assignment to undeclared variable '"
                     + assign.name + "'");
          }
          target =
              &attrArrayMutable(found->second, assign.attrKey, assign.name);
          domainKeys = found->second.domainKeys;
        }
        if (assign.indices.size() != domainKeys.size()) {
          evalFail("assignment to '" + assign.name
                   + "' has the wrong number of indices");
        }

        // Each lvalue index is a set/alias to LOOP over, or a literal label
        // to pin. Loop dimensions bind under the index's own name.
        vector<Dim> dims;
        for (size_t d = 0; d < assign.indices.size(); ++d) {
          Dim dim;
          const string key = toLower(assign.indices[d]);
          if (db_.setP(key)) {
            dim.loopP = true;
            dim.key = key;
            dim.set = &db_.resolveSet(key);
          }
          else {
            dim.fixed =
                db_.resolveSet(domainKeys[d]).ordinalOf(assign.indices[d]);
          }
          dims.push_back(dim);
        }
        assignLoop(assign, *target, dims, 0, vector<size_t>());
        return;
      }

    protected:
    private:
      struct Dim {
        bool loopP = false;
        string key;               // binding key when looping
        const GmsSet* set = nullptr;
        size_t fixed = 0;         // ordinal when pinned
      };

      GmsDatabase& db_;
      GmsExprEvaluator expr_;

      // Assignments read variables through their attribute arrays only.
      double
      readVariableAttr(const GmsVariable& var, const string& attrKey,
                       const vector<size_t>& ordinals) const
      {
        if (attrKey.empty()) {
          evalFail("bare variable '" + var.name
                   + "' in an assignment; read .L / .UP / .LO");
        }
        if ("l" == attrKey) {
          return var.level.at(ordinals);
        }
        if ("up" == attrKey) {
          return var.upper.at(ordinals);
        }
        if ("lo" == attrKey) {
          return var.lower.at(ordinals);
        }
        evalFail("unsupported variable attribute '" + var.name + "." + attrKey
                 + "' (only .L / .UP / .LO)");
      }

      GmsArray&
      attrArrayMutable(GmsVariable& var, const string& attrKey,
                       const string& varName)
      {
        if ("l" == attrKey) {
          return var.level;
        }
        if ("up" == attrKey) {
          return var.upper;
        }
        if ("lo" == attrKey) {
          return var.lower;
        }
        evalFail("unsupported variable attribute '" + varName + "." + attrKey
                 + "' (only .L / .UP / .LO)");
      }

      vector<size_t>
      shapeOf(const vector<string>& domainKeys) const
      {
        vector<size_t> shape;
        for (const string& key : domainKeys) {
          shape.push_back(db_.resolveSet(key).size());
        }
        return shape;
      }

      void
      assignLoop(const Assignment& assign, GmsArray& target,
                 const vector<Dim>& dims, size_t d, vector<size_t> ordinals)
      {
        if (dims.size() == d) {
          const double value = expr_.eval(assign.value);
          if (!std::isfinite(value)) {
            throw std::runtime_error("GMS evaluation: assignment to '"
                                     + assign.name
                                     + "' produced a non-finite value");
          }
          target.at(ordinals) = value;
          return;
        }
        const Dim& dim = dims[d];
        if (!dim.loopP) {
          ordinals.push_back(dim.fixed);
          assignLoop(assign, target, dims, d + 1, ordinals);
          return;
        }
        expr_.pushBinding(dim.key, *dim.set, 0);
        ordinals.push_back(0);
        for (size_t ordinal = 0; ordinal < dim.set->size(); ++ordinal) {
          expr_.setBindingOrdinal(dim.key, ordinal);
          ordinals.back() = ordinal;
          assignLoop(assign, target, dims, d + 1, ordinals);
        }
        expr_.popBinding();
        return;
      }
    };

    class Evaluator {
    public:
      explicit Evaluator(GmsDatabase& db)
        : db_(db)
        , executor_(db)
      {
      }

      void
      run(const Program& program)
      {
        for (const Statement& statement : program.statements) {
          std::visit([this](const auto& stmt) { this->handle(stmt); },
                     statement);
        }
        return;
      }

    protected:
    private:
      GmsDatabase& db_;
      AssignmentExecutor executor_;

      [[noreturn]] void
      fail(const string& message) const
      {
        evalFail(message);
      }

      void
      requireNewSymbol(const string& key, const string& name) const
      {
        const bool takenP = db_.sets.count(key) || db_.aliases.count(key)
                            || db_.parameters.count(key)
                            || db_.variables.count(key)
                            || db_.equations.count(key)
                            || db_.models.count(key);
        if (takenP) {
          fail("symbol '" + name + "' is declared twice");
        }
        return;
      }

      vector<size_t>
      shapeOf(const vector<string>& domainKeys) const
      {
        vector<size_t> shape;
        for (const string& key : domainKeys) {
          shape.push_back(db_.resolveSet(key).size());
        }
        return shape;
      }

      vector<string>
      lowerAll(const vector<string>& names) const
      {
        vector<string> keys;
        for (const string& name : names) {
          keys.push_back(toLower(name));
        }
        return keys;
      }

      // --- symbolic validation of stored equations ---------------------------

      void
      validateExpr(const Expr& expr, std::set<string>& bound) const
      {
        switch (expr.kind) {
        case Expr::Kind::Number:
          return;
        case Expr::Kind::Unary:
        case Expr::Kind::Binary:
          for (const Expr& arg : expr.args) {
            validateExpr(arg, bound);
          }
          return;
        case Expr::Kind::Call:
          if ("card" == expr.key) {
            db_.resolveSet(expr.args.at(0).key);
            return;
          }
          for (const Expr& arg : expr.args) {
            validateExpr(arg, bound);
          }
          return;
        case Expr::Kind::Sum: {
          vector<string> added;
          for (const string& indexName : expr.sumIndices) {
            const string key = toLower(indexName);
            db_.resolveSet(key);   // must be a set or alias
            if (bound.insert(key).second) {
              added.push_back(key);
            }
          }
          validateExpr(expr.args[0], bound);
          for (const string& key : added) {
            bound.erase(key);
          }
          return;
        }
        case Expr::Kind::SymbolRef:
        case Expr::Kind::AttrRef: {
          vector<string> domainKeys;
          if (db_.parameters.count(expr.key)) {
            domainKeys = db_.parameter(expr.key).domainKeys;
            if (Expr::Kind::AttrRef == expr.kind) {
              fail("'" + expr.name + "' is a parameter and has no attributes");
            }
          }
          else
          if (db_.variables.count(expr.key)) {
            domainKeys = db_.variable(expr.key).domainKeys;
          }
          else {
            fail("equation references undeclared symbol '" + expr.name + "'");
          }
          if (expr.indices.size() != domainKeys.size()) {
            std::ostringstream msg;
            msg << "'" << expr.name << "' takes " << domainKeys.size()
                << " indices, got " << expr.indices.size();
            fail(msg.str());
          }
          for (size_t d = 0; d < expr.indices.size(); ++d) {
            const string key = toLower(expr.indices[d]);
            if (0 < bound.count(key)) {
              continue;
            }
            // Not a bound index: accept a literal label of the domain set.
            db_.resolveSet(domainKeys[d]).ordinalOf(expr.indices[d]);
          }
          return;
        }
        default:
          break;
        }
        fail("unsupported expression kind in an equation");
      }

      // --- statement handlers -----------------------------------------------

      void
      handle(const SetDecl& decl)
      {
        requireNewSymbol(decl.item.key, decl.item.name);
        GmsSet set;
        set.name = decl.item.name;
        for (const string& label : decl.elements) {
          const string low = toLower(label);
          if (0 < set.ordinals.count(low)) {
            fail("set '" + decl.item.name + "' repeats label '" + label + "'");
          }
          set.ordinals[low] = set.labels.size();
          set.labels.push_back(label);
        }
        db_.sets[decl.item.key] = set;
        return;
      }

      void
      handle(const AliasDecl& decl)
      {
        // Exactly one of the names must already be a set; the others become
        // its aliases.
        string baseKey;
        for (const string& key : decl.keys) {
          if (0 < db_.sets.count(key)) {
            if (!baseKey.empty()) {
              fail("alias lists two existing sets");
            }
            baseKey = key;
          }
        }
        if (baseKey.empty()) {
          fail("alias needs one existing set among its names");
        }
        for (size_t i = 0; i < decl.keys.size(); ++i) {
          if (decl.keys[i] == baseKey) {
            continue;
          }
          requireNewSymbol(decl.keys[i], decl.names[i]);
          db_.aliases[decl.keys[i]] = baseKey;
        }
        return;
      }

      void
      handle(const ScalarDecl& decl)
      {
        for (const ScalarDecl::Item& item : decl.items) {
          requireNewSymbol(item.decl.key, item.decl.name);
          GmsParameter param;
          param.name = item.decl.name;
          param.data = GmsArray::filled({}, 0.0);
          if (item.hasValueP) {
            param.data.values[0] = item.value;
          }
          db_.parameters[item.decl.key] = param;
        }
        return;
      }

      void
      handle(const ParameterDecl& decl)
      {
        for (const ParameterDecl::Item& item : decl.items) {
          requireNewSymbol(item.decl.key, item.decl.name);
          GmsParameter param;
          param.name = item.decl.name;
          param.domainKeys = lowerAll(item.decl.domain);
          param.data = GmsArray::filled(shapeOf(param.domainKeys), 0.0);
          // Domain-less and data-less: the first assignment fixes the shape.
          param.domainOpenP = param.domainKeys.empty() && !item.hasDataP;
          if (item.hasDataP) {
            for (const DataEntry& entry : item.data) {
              if (entry.keys.size() != param.domainKeys.size()) {
                fail("data entry for '" + param.name
                     + "' has the wrong number of keys");
              }
              vector<size_t> ordinals;
              for (size_t d = 0; d < entry.keys.size(); ++d) {
                ordinals.push_back(db_.resolveSet(param.domainKeys[d])
                                       .ordinalOf(entry.keys[d]));
              }
              param.data.at(ordinals) = entry.value;
            }
          }
          db_.parameters[item.decl.key] = param;
        }
        return;
      }

      void
      handle(const TableDecl& decl)
      {
        requireNewSymbol(decl.decl.key, decl.decl.name);
        if (2 != decl.decl.domain.size()) {
          fail("table '" + decl.decl.name + "' needs a 2-dimensional domain");
        }
        GmsParameter param;
        param.name = decl.decl.name;
        param.domainKeys = lowerAll(decl.decl.domain);
        param.data = GmsArray::filled(shapeOf(param.domainKeys), 0.0);
        const GmsSet& rowSet = db_.resolveSet(param.domainKeys[0]);
        const GmsSet& colSet = db_.resolveSet(param.domainKeys[1]);
        vector<size_t> colOrdinals;
        for (const string& column : decl.columns) {
          colOrdinals.push_back(colSet.ordinalOf(column));
        }
        for (const TableDecl::Row& row : decl.rows) {
          const size_t rowOrdinal = rowSet.ordinalOf(row.label);
          for (size_t c = 0; c < row.values.size(); ++c) {
            param.data.at({rowOrdinal, colOrdinals[c]}) = row.values[c];
          }
        }
        db_.parameters[decl.decl.key] = param;
        return;
      }

      void
      handle(const VariableDecl& decl)
      {
        for (const DeclItem& item : decl.items) {
          requireNewSymbol(item.key, item.name);
          GmsVariable var;
          var.name = item.name;
          var.positiveP = decl.positiveP;
          var.domainKeys = lowerAll(item.domain);
          const vector<size_t> shape = shapeOf(var.domainKeys);
          var.level = GmsArray::filled(shape, 0.0);
          var.lower = GmsArray::filled(shape, decl.positiveP ? 0.0 : -kInf);
          var.upper = GmsArray::filled(shape, kInf);
          db_.variables[item.key] = var;
        }
        return;
      }

      void
      handle(const EquationDecl& decl)
      {
        for (const DeclItem& item : decl.items) {
          requireNewSymbol(item.key, item.name);
          GmsEquation equation;
          equation.name = item.name;
          equation.domainKeys = lowerAll(item.domain);
          db_.equations[item.key] = equation;
        }
        return;
      }

      void
      handle(const EquationDef& def)
      {
        const auto found = db_.equations.find(def.key);
        if (db_.equations.end() == found) {
          fail("definition of undeclared equation '" + def.name + "'");
        }
        GmsEquation& equation = found->second;
        if (equation.definedP) {
          fail("equation '" + def.name + "' is defined twice");
        }
        std::set<string> bound;
        for (const string& indexName : def.domain) {
          const string key = toLower(indexName);
          db_.resolveSet(key);
          bound.insert(key);
        }
        validateExpr(def.lhs, bound);
        validateExpr(def.rhs, bound);
        equation.definedP = true;
        equation.def = def;
        // The declaration may have carried no domain (deploy declares bare
        // names, then defines with domains); adopt the definition's.
        if (equation.domainKeys.empty() && !def.domain.empty()) {
          equation.domainKeys = lowerAll(def.domain);
        }
        return;
      }

      void
      handle(const Assignment& assign)
      {
        executor_.execute(assign);
        return;
      }

      void
      handle(const ModelDecl& decl)
      {
        requireNewSymbol(decl.key, decl.name);
        GmsModel model;
        model.name = decl.name;
        for (const ModelDecl::Pair& pair : decl.pairs) {
          if (0 == db_.equations.count(pair.eqKey)) {
            fail("model '" + decl.name + "' pairs undeclared equation '"
                 + pair.eqName + "'");
          }
          if (0 == db_.variables.count(pair.varKey)) {
            fail("model '" + decl.name + "' pairs undeclared variable '"
                 + pair.varName + "'");
          }
        }
        model.pairs = decl.pairs;
        db_.models[decl.key] = model;
        return;
      }

      void
      handle(const OptionStmt& stmt)
      {
        db_.options.push_back(stmt);
        return;
      }

      void
      handle(const SolveStmt& stmt)
      {
        db_.model(stmt.modelKey);   // must exist
        db_.solves.push_back(stmt);
        return;
      }

      void
      handle(const DisplayStmt& stmt)
      {
        for (const DisplayStmt::Item& item : stmt.items) {
          const bool knownP = db_.parameters.count(item.key)
                              || db_.variables.count(item.key);
          if (!knownP) {
            fail("Display of undeclared symbol '" + item.name + "'");
          }
        }
        return;
      }
    };

  } // namespace

  GmsDatabase
  buildGmsDatabase(const Program& program)
  {
    GmsDatabase db;
    Evaluator evaluator(db);
    evaluator.run(program);
    return db;
  }

  void
  rerunPostSolveAssignments(GmsDatabase& db, const Program& program)
  {
    AssignmentExecutor executor(db);
    bool pastSolveP = false;
    for (const Statement& statement : program.statements) {
      if (std::holds_alternative<SolveStmt>(statement)) {
        pastSolveP = true;
        continue;
      }
      if (!pastSolveP) {
        continue;
      }
      if (const Assignment* assign = std::get_if<Assignment>(&statement)) {
        executor.execute(*assign);
      }
    }
    return;
  }

} // namespace VIMCP::Gms
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
