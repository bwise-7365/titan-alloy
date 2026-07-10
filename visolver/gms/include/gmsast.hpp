// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// AST for the GAMS subset (GP1): value-semantic statement and expression
// nodes with defaulted deep equality. Deliberately position-free: parse
// errors carry positions from tokens; semantic positions are GP2's concern.
// Names keep their original spelling for echo; every name travels with a
// lower-cased key (GAMS identifiers are case-insensitive).
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VINCP_GMS_GMSAST_HPP
#define VINCP_GMS_GMSAST_HPP

#include <string>
#include <variant>
#include <vector>

namespace VINCP::Gms {

  using std::string;
  using std::vector;

  // --------------------------------------------------------------------------
  // Expressions
  // --------------------------------------------------------------------------

  struct Expr {
    enum class Kind {
      Number,      // number
      SymbolRef,   // name(indices...)
      AttrRef,     // name.attr(indices...)   e.g. fs.L(fj), gamma.L
      Unary,       // op = "-" (or "+"), args[0]
      Binary,      // op in + - * / **, args[0] op args[1]
      Call,        // name(args...)           exp, log, sqrt, sqr, max, ...
      Sum,         // sum(sumIndices, args[0])
    };

    Kind kind = Kind::Number;
    double number = 0.0;
    string name;                 // symbol / function name, original spelling
    string key;                  // lower-cased name
    string attr;                 // AttrRef only: attribute, original spelling
    string attrKey;              // AttrRef only: lower-cased attribute
    string op;                   // Unary / Binary operator text
    vector<string> indices;      // SymbolRef / AttrRef index labels (original)
    vector<string> sumIndices;   // Sum controlling indices (original)
    vector<Expr> args;           // operands / call arguments / sum body

    bool operator==(const Expr&) const = default;
  };

  // --------------------------------------------------------------------------
  // Statements
  // --------------------------------------------------------------------------

  // One declared symbol inside a declaration statement.
  struct DeclItem {
    string name;
    string key;
    vector<string> domain;   // empty when undomained
    string description;      // quotes stripped; empty when absent
    bool operator==(const DeclItem&) const = default;
  };

  // One keyed numeric datum in a / ... / block: keys... [=] value.
  struct DataEntry {
    vector<string> keys;
    double value = 0.0;
    bool operator==(const DataEntry&) const = default;
  };

  struct SetDecl {
    DeclItem item;
    vector<string> elements;   // element labels, original spelling
    bool operator==(const SetDecl&) const = default;
  };

  struct AliasDecl {
    vector<string> names;
    vector<string> keys;
    bool operator==(const AliasDecl&) const = default;
  };

  struct ScalarDecl {
    struct Item {
      DeclItem decl;
      bool hasValueP = false;
      double value = 0.0;
      bool operator==(const Item&) const = default;
    };
    vector<Item> items;
    bool operator==(const ScalarDecl&) const = default;
  };

  struct ParameterDecl {
    struct Item {
      DeclItem decl;
      bool hasDataP = false;
      vector<DataEntry> data;
      bool operator==(const Item&) const = default;
    };
    vector<Item> items;
    bool operator==(const ParameterDecl&) const = default;
  };

  struct TableDecl {
    struct Row {
      string label;
      vector<double> values;   // dense: exactly one value per column
      bool operator==(const Row&) const = default;
    };
    DeclItem decl;             // name, 2-D domain, description
    vector<string> columns;    // column labels, original spelling
    vector<Row> rows;
    bool operator==(const TableDecl&) const = default;
  };

  struct VariableDecl {
    bool positiveP = false;
    vector<DeclItem> items;
    bool operator==(const VariableDecl&) const = default;
  };

  struct EquationDecl {
    vector<DeclItem> items;
    bool operator==(const EquationDecl&) const = default;
  };

  struct EquationDef {
    string name;
    string key;
    vector<string> domain;
    Expr lhs;
    string relation;   // "=e=", "=g=", or "=l="
    Expr rhs;
    bool operator==(const EquationDef&) const = default;
  };

  // name[.attr][(indices)] = expr;  -- parameter assignment, variable
  // attribute assignment (x.L, x.UP), or model attribute (m.optfile).
  struct Assignment {
    string name;
    string key;
    string attr;       // empty for a plain assignment
    string attrKey;
    vector<string> indices;
    Expr value;
    bool operator==(const Assignment&) const = default;
  };

  struct ModelDecl {
    struct Pair {
      string eqName;
      string eqKey;
      string varName;
      string varKey;
      bool operator==(const Pair&) const = default;
    };
    string name;
    string key;
    vector<Pair> pairs;
    bool operator==(const ModelDecl&) const = default;
  };

  struct OptionStmt {
    string name;    // e.g. MCP, decimals
    string key;
    string value;   // right-hand text: MILES, 3, ...
    bool operator==(const OptionStmt&) const = default;
  };

  struct SolveStmt {
    string modelName;
    string modelKey;
    string method;   // e.g. MCP (as written)
    bool operator==(const SolveStmt&) const = default;
  };

  struct DisplayStmt {
    struct Item {
      string name;
      string key;
      string attr;   // empty, or L etc.
      bool operator==(const Item&) const = default;
    };
    vector<Item> items;
    bool operator==(const DisplayStmt&) const = default;
  };

  using Statement =
      std::variant<SetDecl, AliasDecl, ScalarDecl, ParameterDecl, TableDecl,
                   VariableDecl, EquationDecl, EquationDef, Assignment,
                   ModelDecl, OptionStmt, SolveStmt, DisplayStmt>;

  struct Program {
    vector<Statement> statements;
    vector<string> macroNames;   // $macro definitions seen, in order

    // Equality is on the statements only: macroNames is lexing metadata, and
    // the canonical echo emits macro-EXPANDED text (no $macro survives), so
    // a round-tripped program legitimately has an empty macro list.
    bool operator==(const Program& other) const
    {
      return statements == other.statements;
    }
  };

} // namespace VINCP::Gms

#endif // VINCP_GMS_GMSAST_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
