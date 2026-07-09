// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GP1 gate tests for the GAMS-subset parser: the construct census of
// doc/2026-07-08-gams-subset-census.md as per-file assertions, round-trip
// idempotence through the canonical echo, macro expansion, case-insensitive
// keys, and loud failures on constructs outside the subset. The corpus files
// are read in place from doc/ (VINCP_GMS_CORPUS_DIR).
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
#include "gmsparser.hpp"

#include <gtest/gtest.h>

#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace {

  using namespace VINCP::Gms;
  using std::string;

  string
  corpusPath(const string& name)
  {
    return string(VINCP_GMS_CORPUS_DIR) + "/" + name;
  }

  // Construct counts over a parsed program; each corpus file's expected
  // census is asserted field by field.
  struct GmsCensus {
    int statements = 0;
    int sets = 0;
    int aliases = 0;
    int scalarStmts = 0;
    int scalarItems = 0;
    int paramStmts = 0;
    int paramItems = 0;
    int tables = 0;
    int posVarStmts = 0;
    int posVarItems = 0;
    int freeVarStmts = 0;
    int freeVarItems = 0;
    int eqDeclStmts = 0;
    int eqDeclItems = 0;
    int eqDefs = 0;
    int eqDefsEq = 0;
    int eqDefsGe = 0;
    int assignments = 0;
    int models = 0;
    int modelPairs = 0;
    int options = 0;
    int solves = 0;
    int displays = 0;
    int displayItems = 0;
  };

  GmsCensus
  takeCensus(const Program& program)
  {
    GmsCensus census;
    census.statements = static_cast<int>(program.statements.size());
    for (const Statement& statement : program.statements) {
      if (std::holds_alternative<SetDecl>(statement)) {
        census.sets += 1;
      }
      else
      if (std::holds_alternative<AliasDecl>(statement)) {
        census.aliases += 1;
      }
      else
      if (const ScalarDecl* scalar = std::get_if<ScalarDecl>(&statement)) {
        census.scalarStmts += 1;
        census.scalarItems += static_cast<int>(scalar->items.size());
      }
      else
      if (const ParameterDecl* param = std::get_if<ParameterDecl>(&statement)) {
        census.paramStmts += 1;
        census.paramItems += static_cast<int>(param->items.size());
      }
      else
      if (std::holds_alternative<TableDecl>(statement)) {
        census.tables += 1;
      }
      else
      if (const VariableDecl* var = std::get_if<VariableDecl>(&statement)) {
        if (var->positiveP) {
          census.posVarStmts += 1;
          census.posVarItems += static_cast<int>(var->items.size());
        }
        else {
          census.freeVarStmts += 1;
          census.freeVarItems += static_cast<int>(var->items.size());
        }
      }
      else
      if (const EquationDecl* eq = std::get_if<EquationDecl>(&statement)) {
        census.eqDeclStmts += 1;
        census.eqDeclItems += static_cast<int>(eq->items.size());
      }
      else
      if (const EquationDef* def = std::get_if<EquationDef>(&statement)) {
        census.eqDefs += 1;
        if ("=e=" == def->relation) {
          census.eqDefsEq += 1;
        }
        if ("=g=" == def->relation) {
          census.eqDefsGe += 1;
        }
      }
      else
      if (std::holds_alternative<Assignment>(statement)) {
        census.assignments += 1;
      }
      else
      if (const ModelDecl* model = std::get_if<ModelDecl>(&statement)) {
        census.models += 1;
        census.modelPairs += static_cast<int>(model->pairs.size());
      }
      else
      if (std::holds_alternative<OptionStmt>(statement)) {
        census.options += 1;
      }
      else
      if (std::holds_alternative<SolveStmt>(statement)) {
        census.solves += 1;
      }
      else
      if (const DisplayStmt* display = std::get_if<DisplayStmt>(&statement)) {
        census.displays += 1;
        census.displayItems += static_cast<int>(display->items.size());
      }
    }
    return census;
  }

  void
  expectCensus(const GmsCensus& actual, const GmsCensus& expected)
  {
    EXPECT_EQ(expected.statements, actual.statements);
    EXPECT_EQ(expected.sets, actual.sets);
    EXPECT_EQ(expected.aliases, actual.aliases);
    EXPECT_EQ(expected.scalarStmts, actual.scalarStmts);
    EXPECT_EQ(expected.scalarItems, actual.scalarItems);
    EXPECT_EQ(expected.paramStmts, actual.paramStmts);
    EXPECT_EQ(expected.paramItems, actual.paramItems);
    EXPECT_EQ(expected.tables, actual.tables);
    EXPECT_EQ(expected.posVarStmts, actual.posVarStmts);
    EXPECT_EQ(expected.posVarItems, actual.posVarItems);
    EXPECT_EQ(expected.freeVarStmts, actual.freeVarStmts);
    EXPECT_EQ(expected.freeVarItems, actual.freeVarItems);
    EXPECT_EQ(expected.eqDeclStmts, actual.eqDeclStmts);
    EXPECT_EQ(expected.eqDeclItems, actual.eqDeclItems);
    EXPECT_EQ(expected.eqDefs, actual.eqDefs);
    EXPECT_EQ(expected.eqDefsEq, actual.eqDefsEq);
    EXPECT_EQ(expected.eqDefsGe, actual.eqDefsGe);
    EXPECT_EQ(expected.assignments, actual.assignments);
    EXPECT_EQ(expected.models, actual.models);
    EXPECT_EQ(expected.modelPairs, actual.modelPairs);
    EXPECT_EQ(expected.options, actual.options);
    EXPECT_EQ(expected.solves, actual.solves);
    EXPECT_EQ(expected.displays, actual.displays);
    EXPECT_EQ(expected.displayItems, actual.displayItems);
    return;
  }

} // namespace

// -----------------------------------------------------------------------------
// Census-as-assertions, one test per corpus file.
// -----------------------------------------------------------------------------

TEST(GmsParse, ForcepkgCensus)
{
  const Program program = parseGmsFile(corpusPath("forcepkg_ln.gms"));
  GmsCensus expected;
  expected.statements = 24;
  expected.sets = 3;
  expected.scalarStmts = 1;
  expected.scalarItems = 1;
  expected.paramStmts = 5;
  expected.paramItems = 5;
  expected.tables = 3;
  expected.posVarStmts = 1;
  expected.posVarItems = 2;
  expected.eqDeclStmts = 1;
  expected.eqDeclItems = 2;
  expected.eqDefs = 2;
  expected.eqDefsGe = 2;
  expected.assignments = 4;
  expected.models = 1;
  expected.modelPairs = 2;
  expected.options = 1;
  expected.solves = 1;
  expected.displays = 1;
  expected.displayItems = 5;
  expectCensus(takeCensus(program), expected);
  EXPECT_TRUE(program.macroNames.empty());
}

TEST(GmsParse, AlloceffCensus)
{
  const Program program = parseGmsFile(corpusPath("alloceff01cm.gms"));
  GmsCensus expected;
  expected.statements = 38;
  expected.sets = 2;
  expected.aliases = 2;
  expected.paramStmts = 3;
  expected.paramItems = 9;
  expected.tables = 1;
  expected.posVarStmts = 2;
  expected.posVarItems = 5;
  expected.eqDeclStmts = 2;
  expected.eqDeclItems = 5;
  expected.eqDefs = 5;
  expected.eqDefsEq = 3;
  expected.eqDefsGe = 2;
  expected.assignments = 15;
  expected.models = 1;
  expected.modelPairs = 5;
  expected.options = 2;   // Option MCP = NLPEC; plus 'options decimals=3'
  expected.solves = 1;
  expected.displays = 2;
  expected.displayItems = 19;
  expectCensus(takeCensus(program), expected);
}

TEST(GmsParse, Glra4BCensus)
{
  // Exercises $include: the data section lives in glra4B.inc.
  const Program program = parseGmsFile(corpusPath("glra4B.gms"));
  GmsCensus expected;
  expected.statements = 46;
  expected.sets = 1;
  expected.aliases = 1;   // the 5-way alias(ni, nj, nk, nn, nm)
  expected.paramStmts = 6;
  expected.paramItems = 20;
  expected.tables = 3;
  expected.posVarStmts = 2;
  expected.posVarItems = 8;
  expected.eqDeclStmts = 1;
  expected.eqDeclItems = 8;
  expected.eqDefs = 8;
  expected.eqDefsGe = 8;
  expected.assignments = 19;
  expected.models = 1;
  expected.modelPairs = 8;
  expected.options = 1;
  expected.solves = 1;
  expected.displays = 2;
  expected.displayItems = 18;
  expectCensus(takeCensus(program), expected);

  // Spot-check the include's data actually arrived: 30 nodes, dense 30x30.
  const SetDecl* nodes = nullptr;
  const TableDecl* dist = nullptr;
  for (const Statement& statement : program.statements) {
    if (const SetDecl* set = std::get_if<SetDecl>(&statement)) {
      nodes = set;
    }
    if (const TableDecl* table = std::get_if<TableDecl>(&statement)) {
      if ("dist" == table->decl.key) {
        dist = table;
      }
    }
  }
  ASSERT_NE(nullptr, nodes);
  EXPECT_EQ(30u, nodes->elements.size());
  ASSERT_NE(nullptr, dist);
  EXPECT_EQ(30u, dist->columns.size());
  ASSERT_EQ(30u, dist->rows.size());
  EXPECT_EQ(30u, dist->rows[0].values.size());
}

TEST(GmsParse, PewemCensus)
{
  const Program program = parseGmsFile(corpusPath("pewem01.gms"));
  GmsCensus expected;
  expected.statements = 30;
  expected.sets = 3;
  expected.scalarStmts = 1;
  expected.scalarItems = 1;
  expected.paramStmts = 4;
  expected.paramItems = 13;
  expected.posVarStmts = 1;
  expected.posVarItems = 13;
  expected.eqDeclStmts = 2;
  expected.eqDeclItems = 12;
  expected.eqDefs = 12;
  expected.eqDefsGe = 12;
  expected.assignments = 3;
  expected.models = 1;
  expected.modelPairs = 12;
  expected.options = 1;
  expected.solves = 1;
  expected.displays = 1;
  expected.displayItems = 12;
  expectCensus(takeCensus(program), expected);

  // Spot-check the two-key data form: cpl(ip,jm) has 8 keyed entries.
  for (const Statement& statement : program.statements) {
    if (const ParameterDecl* param = std::get_if<ParameterDecl>(&statement)) {
      for (const ParameterDecl::Item& item : param->items) {
        if ("cpl" == item.decl.key) {
          ASSERT_TRUE(item.hasDataP);
          ASSERT_EQ(8u, item.data.size());
          EXPECT_EQ(2u, item.data[0].keys.size());
          EXPECT_EQ("nrwy", item.data[0].keys[0]);
          EXPECT_EQ("eum", item.data[0].keys[1]);
          EXPECT_DOUBLE_EQ(100.0, item.data[0].value);
        }
      }
    }
  }
}

TEST(GmsParse, DeployCensus)
{
  const Program program = parseGmsFile(corpusPath("deploy_v09.gms"));
  GmsCensus expected;
  expected.statements = 97;
  expected.sets = 5;
  expected.aliases = 2;
  expected.scalarStmts = 1;
  expected.scalarItems = 6;
  expected.paramStmts = 4;
  expected.paramItems = 32;
  expected.tables = 4;
  expected.posVarStmts = 2;
  expected.posVarItems = 20;
  expected.freeVarStmts = 1;
  expected.freeVarItems = 4;
  expected.eqDeclStmts = 1;
  expected.eqDeclItems = 24;
  expected.eqDefs = 24;
  expected.eqDefsEq = 4;
  expected.eqDefsGe = 20;
  expected.assignments = 49;
  expected.models = 1;
  expected.modelPairs = 24;
  expected.options = 1;
  expected.solves = 1;
  expected.displays = 1;
  expected.displayItems = 22;
  expectCensus(takeCensus(program), expected);

  // The 15 softplus/salvo macros were captured (and expanded before parse).
  EXPECT_EQ(15u, program.macroNames.size());
}

// -----------------------------------------------------------------------------
// Round trip: parse(echo(parse(F))) == parse(F) for every corpus file.
// -----------------------------------------------------------------------------

TEST(GmsParse, RoundTripIsIdempotent)
{
  const std::vector<string> files = {"forcepkg_ln.gms", "alloceff01cm.gms",
                                     "glra4B.gms", "pewem01.gms",
                                     "deploy_v09.gms"};
  for (const string& file : files) {
    const Program first = parseGmsFile(corpusPath(file));
    const string canonical = echoProgram(first);
    const Program second = parseGmsString(canonical, file + " (echo)");
    EXPECT_TRUE(first == second) << file << ": echo round trip changed the AST";
    EXPECT_EQ(first.statements.size(), second.statements.size()) << file;
  }
}

// -----------------------------------------------------------------------------
// Macros: nested expansion equals the hand-expanded text.
// -----------------------------------------------------------------------------

TEST(GmsParse, NestedMacroExpansionMatchesHandExpansion)
{
  const string withMacros = "$macro twice(x) ((x) + (x))\n"
                            "$macro quad(x) (twice(twice(x)))\n"
                            "Parameter a;\n"
                            "a = quad(3);\n";
  const string handExpanded = "Parameter a;\n"
                              "a = (((3) + (3)) + ((3) + (3)));\n";
  const Program macroProgram = parseGmsString(withMacros, "macro.gms");
  const Program plainProgram = parseGmsString(handExpanded, "plain.gms");
  EXPECT_EQ(2u, macroProgram.macroNames.size());
  EXPECT_TRUE(macroProgram == plainProgram);
}

// -----------------------------------------------------------------------------
// Case-insensitivity: keys are lower-cased; keywords match in any case.
// -----------------------------------------------------------------------------

TEST(GmsParse, IdentifiersAndKeywordsAreCaseInsensitive)
{
  const Program program = parseGmsString("SET MySet / A1 /;\n"
                                         "PARAMETER P(MySet);\n"
                                         "P(MYSET) = 1.0;\n",
                                         "case.gms");
  ASSERT_EQ(3u, program.statements.size());
  const SetDecl& set = std::get<SetDecl>(program.statements[0]);
  EXPECT_EQ("MySet", set.item.name);
  EXPECT_EQ("myset", set.item.key);
  const Assignment& assign = std::get<Assignment>(program.statements[2]);
  EXPECT_EQ("p", assign.key);
}

// -----------------------------------------------------------------------------
// Loud failures: constructs outside the censused subset throw.
// -----------------------------------------------------------------------------

TEST(GmsParse, RejectsConstructsOutsideTheSubset)
{
  // Unsupported directive.
  EXPECT_THROW(parseGmsString("$ontext\nprose\n$offtext\n", "t1.gms"),
               std::invalid_argument);
  // Dollar-conditional assignment.
  EXPECT_THROW(parseGmsString("Parameter p(i);\np(i)$(1) = 1.0;\n", "t2.gms"),
               std::invalid_argument);
  // Control flow.
  EXPECT_THROW(parseGmsString("loop(i, p(i) = 1.0;);\n", "t3.gms"),
               std::invalid_argument);
  // Sparse table row.
  EXPECT_THROW(parseGmsString("Table t(i,j)\n c1 c2\n r1 1.0\n;\n", "t4.gms"),
               std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
