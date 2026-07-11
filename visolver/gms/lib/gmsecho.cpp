// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// gmsecho implementation: canonical text for each statement and expression
// node. Numbers print with 17 significant digits so doubles survive the
// round trip exactly.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#include "gmsecho.hpp"

#include <iomanip>
#include <sstream>
#include <variant>

namespace VIMCP::Gms {

  namespace {

    string
    numberText(double value)
    {
      std::ostringstream out;
      out << std::setprecision(17) << value;
      return out.str();
    }

    string
    joined(const vector<string>& parts, const string& separator)
    {
      string text;
      for (size_t i = 0; i < parts.size(); ++i) {
        if (0 < i) {
          text += separator;
        }
        text += parts[i];
      }
      return text;
    }

    // name or name(d1,d2)
    string
    headerText(const string& name, const vector<string>& domain)
    {
      if (domain.empty()) {
        return name;
      }
      return name + "(" + joined(domain, ", ") + ")";
    }

    string
    describedHeader(const DeclItem& item)
    {
      string text = headerText(item.name, item.domain);
      if (!item.description.empty()) {
        text += " '" + item.description + "'";
      }
      return text;
    }

    string
    dataEntryText(const DataEntry& entry)
    {
      return joined(entry.keys, " . ") + " = " + numberText(entry.value);
    }

    struct EchoVisitor {
      std::ostringstream& out;

      void
      operator()(const SetDecl& decl) const
      {
        out << "Set " << describedHeader(decl.item) << " / "
            << joined(decl.elements, ", ") << " /;\n";
        return;
      }

      void
      operator()(const AliasDecl& decl) const
      {
        out << "Alias (" << joined(decl.names, ", ") << ");\n";
        return;
      }

      void
      operator()(const ScalarDecl& decl) const
      {
        out << "Scalars\n";
        for (const ScalarDecl::Item& item : decl.items) {
          out << "  " << describedHeader(item.decl);
          if (item.hasValueP) {
            out << " / " << numberText(item.value) << " /";
          }
          out << "\n";
        }
        out << ";\n";
        return;
      }

      void
      operator()(const ParameterDecl& decl) const
      {
        out << "Parameters\n";
        for (const ParameterDecl::Item& item : decl.items) {
          out << "  " << describedHeader(item.decl);
          if (item.hasDataP) {
            out << " /";
            for (size_t i = 0; i < item.data.size(); ++i) {
              out << ((0 < i) ? ", " : " ") << dataEntryText(item.data[i]);
            }
            out << " /";
          }
          out << "\n";
        }
        out << ";\n";
        return;
      }

      void
      operator()(const TableDecl& decl) const
      {
        out << "Table " << describedHeader(decl.decl) << "\n  "
            << joined(decl.columns, " ") << "\n";
        for (const TableDecl::Row& row : decl.rows) {
          out << "  " << row.label;
          for (const double value : row.values) {
            out << " " << numberText(value);
          }
          out << "\n";
        }
        out << ";\n";
        return;
      }

      void
      operator()(const VariableDecl& decl) const
      {
        out << (decl.positiveP ? "Positive Variables\n" : "Variables\n");
        for (const DeclItem& item : decl.items) {
          out << "  " << describedHeader(item) << "\n";
        }
        out << ";\n";
        return;
      }

      void
      operator()(const EquationDecl& decl) const
      {
        out << "Equations\n";
        for (const DeclItem& item : decl.items) {
          out << "  " << describedHeader(item) << "\n";
        }
        out << ";\n";
        return;
      }

      void
      operator()(const EquationDef& def) const
      {
        out << headerText(def.name, def.domain) << ".. " << echoExpr(def.lhs)
            << " " << def.relation << " " << echoExpr(def.rhs) << ";\n";
        return;
      }

      void
      operator()(const Assignment& assign) const
      {
        out << assign.name;
        if (!assign.attr.empty()) {
          out << "." << assign.attr;
        }
        if (!assign.indices.empty()) {
          out << "(" << joined(assign.indices, ", ") << ")";
        }
        out << " = " << echoExpr(assign.value) << ";\n";
        return;
      }

      void
      operator()(const ModelDecl& decl) const
      {
        out << "Model " << decl.name << " /";
        for (size_t i = 0; i < decl.pairs.size(); ++i) {
          out << ((0 < i) ? ", " : " ") << decl.pairs[i].eqName << "."
              << decl.pairs[i].varName;
        }
        out << " /;\n";
        return;
      }

      void
      operator()(const OptionStmt& stmt) const
      {
        out << "Option " << stmt.name << " = " << stmt.value << ";\n";
        return;
      }

      void
      operator()(const SolveStmt& stmt) const
      {
        out << "Solve " << stmt.modelName << " using " << stmt.method
            << ";\n";
        return;
      }

      void
      operator()(const DisplayStmt& stmt) const
      {
        out << "Display ";
        for (size_t i = 0; i < stmt.items.size(); ++i) {
          if (0 < i) {
            out << ", ";
          }
          out << stmt.items[i].name;
          if (!stmt.items[i].attr.empty()) {
            out << "." << stmt.items[i].attr;
          }
        }
        out << ";\n";
        return;
      }
    };

  } // namespace

  string
  echoExpr(const Expr& expr)
  {
    switch (expr.kind) {
    case Expr::Kind::Number:
      return numberText(expr.number);
    case Expr::Kind::SymbolRef:
      if (expr.indices.empty()) {
        return expr.name;
      }
      return expr.name + "(" + joined(expr.indices, ", ") + ")";
    case Expr::Kind::AttrRef: {
      string text = expr.name + "." + expr.attr;
      if (!expr.indices.empty()) {
        text += "(" + joined(expr.indices, ", ") + ")";
      }
      return text;
    }
    case Expr::Kind::Unary:
      return "(" + expr.op + echoExpr(expr.args[0]) + ")";
    case Expr::Kind::Binary:
      return "(" + echoExpr(expr.args[0]) + " " + expr.op + " "
             + echoExpr(expr.args[1]) + ")";
    case Expr::Kind::Call: {
      vector<string> argTexts;
      for (const Expr& arg : expr.args) {
        argTexts.push_back(echoExpr(arg));
      }
      return expr.name + "(" + joined(argTexts, ", ") + ")";
    }
    case Expr::Kind::Sum: {
      string indexText = (1 == expr.sumIndices.size())
                             ? expr.sumIndices[0]
                             : "(" + joined(expr.sumIndices, ", ") + ")";
      return expr.name + "(" + indexText + ", " + echoExpr(expr.args[0]) + ")";
    }
    default:
      break;
    }
    return string();
  }

  string
  echoProgram(const Program& program)
  {
    std::ostringstream out;
    EchoVisitor visitor{out};
    for (const Statement& statement : program.statements) {
      std::visit(visitor, statement);
    }
    return out.str();
  }

} // namespace VIMCP::Gms
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
