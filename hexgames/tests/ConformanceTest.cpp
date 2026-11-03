// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The HexRules rule conformance cases (HexRules corpus/cases, xsd/hexcase.xsd), one test per case,
// answered by the engine's default policies: SixHexZoc for zoc, CountStacking for stacking-excess,
// AdjacentRetreat for retreat-candidates, OddsTableResolver's table lookup for crt and
// Session::reachable for reach. A case whose rules use an option the defaults do not read, where
// that option changes the answer, is skipped with the option named. A wrong answer fails, naming
// the case file, the expected answer and the engine's.
// ----------------------------------------------
#include "hexengine/Defaults.h"
#include "hexengine/Session.h"
#include "hexmodel/BoardBuilder.h"
#include "hexmodel/PositionBuilder.h"
#include "hexmodel/RosterBuilder.h"
#include "hexrules/Package.h"
#include "hexrules/RuleSetBuilder.h"
#include "hexxml/CaseDoc.h"
#include "hexxml/CounterSetDoc.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/SaveDoc.h"
#include "hexxml/SheetDoc.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <map>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  namespace fs = std::filesystem;
  using HexModel::HexIndex;
  using HexModel::UnitId;

  fs::path
  casesDir()
  {
    return fs::path(HEXRULES_DIR) / "corpus" / "cases";
  }

  // Every document under corpus/cases whose root element is <case>, in path order. The paths are
  // strings because GoogleTest prints a parameter, and it cannot print a std::filesystem::path.
  std::vector<std::string>
  caseFiles()
  {
    std::vector<std::string> out;
    if (!fs::exists(casesDir())) {
      return out;
    }
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(casesDir())) {
      if (!entry.is_regular_file() || ".xml" != entry.path().extension().string()) {
        continue;
      }
      if ("case" == HexXml::XmlDocument::load(entry.path()).root().name()) {
        out.push_back(entry.path().generic_string());
      }
    }
    std::sort(out.begin(), out.end());
    return out;
  }

  std::string
  joined(const std::vector<std::string>& tokens)
  {
    std::string out = "{";
    for (const std::string& token : tokens) {
      out += (1 == out.size() ? "" : " ") + token;
    }
    return out + "}";
  }

  std::vector<std::string>
  sorted(std::vector<std::string> tokens)
  {
    std::sort(tokens.begin(), tokens.end());
    return tokens;
  }

  HexXml::RulesDoc
  readRules(const fs::path& file)
  {
    return HexXml::RulesDoc::parse(HexXml::XmlDocument::load(file));
  }

  const HexXml::ResolverDoc*
  firstOddsTable(const HexXml::RulesDoc& rules)
  {
    for (const HexXml::ResolverDoc& resolver : rules.combat.resolvers) {
      if ("odds-table" == resolver.kind && !resolver.tables.empty()) {
        return &resolver;
      }
    }
    return nullptr;
  }

  bool
  wholeP(double value)
  {
    return value == std::floor(value);
  }

  // The rules options the default policies do not read and that change the answer to a query of
  // this kind, with the policy that does not read them; nullopt when the defaults can answer it.
  std::optional<std::string>
  unsupported(const HexXml::CaseDoc& c, const HexXml::RulesDoc& rules)
  {
    const std::string& kind = c.query.kind;
    if ("reach" == kind && "enter" != rules.movement.costRule) {
      return "movement/@cost-rule='" + rules.movement.costRule +
             "': TerrainMovement charges the cost of the hex entered, and reads no other cost rule";
    }
    if ("retreat-candidates" == kind && rules.retreat && rules.retreat->throughZoc &&
        "forbidden" == *rules.retreat->throughZoc) {
      return std::string("retreat/@through-zoc='forbidden': AdjacentRetreat reads zoc/@blocks, not "
                         "retreat/@through-zoc");
    }
    if ("crt" == kind) {
      const HexXml::ResolverDoc* first = firstOddsTable(rules);
      if (nullptr != first && *c.query.resolver != first->id) {
        return "query/@resolver='" + *c.query.resolver +
               "': OddsTableResolver reads only the first odds-table resolver, '" + first->id + "'";
      }
      if (nullptr != first && first->roundUpFrom) {
        return std::string(
            "resolver/@round-up-from: OddsTableResolver rounds by resolver/@rounding alone");
      }
      if (!wholeP(*c.query.attack) || !wholeP(*c.query.defence)) {
        return std::string(
            "query/@attack or @defence is fractional: a hexgames Strength is a whole number");
      }
    }
    if ("supply" == kind) {
      return std::string("query kind 'supply': this test does not yet ask BoundedTrace");
    }
    return std::nullopt;
  }

  // ---- the position -----------------------------------------------------------------------------
  // hexgames' roster holds one unit per counter id, whereas a save may place several units printed
  // from one counter (R1 to R4 are all 'red-armor'). Each save unit therefore becomes a counter of
  // its own, named by its unit id and carrying its counter's faces; the package binds that counter
  // to the unit's type and its style to the unit's owner. The sheet's terrain ids are the rules'
  // own, so the package binds no terrain.

  struct Game {
    std::shared_ptr<const HexRules::GameDefinition> definition;
    HexModel::Position position;
    std::uint64_t seed = 0;
  };

  fs::path
  countersOf(const HexXml::SaveDoc& save, const fs::path& saveFile)
  {
    for (const HexXml::SaveFileDoc& file : save.files) {
      if ("counters" == file.role) {
        return saveFile.parent_path() / file.path;
      }
    }
    throw std::invalid_argument("ConformanceTest: save '" + saveFile.generic_string() +
                                "' names no counters file in its <package>");
  }

  HexXml::CounterSetDoc
  countersPerUnit(const HexXml::CounterSetDoc& printed, const HexXml::SaveDoc& save)
  {
    HexXml::CounterSetDoc out = printed;
    out.counters.clear();
    out.sheets.clear();
    for (const HexXml::SaveUnitDoc& unit : save.units) {
      const auto found = std::find_if(
          printed.counters.begin(), printed.counters.end(),
          [&](const HexXml::CounterDoc& counter) { return unit.counter == counter.id; });
      if (printed.counters.end() == found) {
        throw std::invalid_argument("ConformanceTest: unit '" + unit.id + "' names counter '" +
                                    unit.counter + "', which the counters document lacks");
      }
      HexXml::CounterDoc own = *found;
      own.id = unit.id;
      own.count = 1;
      out.counters.push_back(std::move(own));
    }
    return out;
  }

  HexXml::PackageDoc
  bindingsFor(const HexXml::CaseDoc& c, const HexXml::CounterSetDoc& perUnit,
              const HexXml::SaveDoc& save)
  {
    HexXml::PackageDoc package;
    package.id = c.id;
    std::map<std::string, std::vector<std::string>> stylesOfOwner;  // ordered, never unordered_*
    for (std::size_t i = 0; i < save.units.size(); ++i) {
      const HexXml::SaveUnitDoc& unit = save.units[i];
      const std::optional<std::string>& style = perUnit.counters[i].front.style;
      if (!style) {
        throw std::invalid_argument("ConformanceTest: counter '" + unit.counter +
                                    "' has no front style, so no side can be bound to it");
      }
      std::vector<std::string>& styles = stylesOfOwner[unit.owner];
      if (styles.end() == std::find(styles.begin(), styles.end(), *style)) {
        styles.push_back(*style);
      }
      package.unit.push_back(HexXml::PackageUnitBindingDoc{unit.type, {unit.id}, std::nullopt});
    }
    for (const auto& [owner, styles] : stylesOfOwner) {
      package.side.push_back(HexXml::PackageSideBindingDoc{owner, styles});
    }
    return package;
  }

  Game
  gameOf(const fs::path& file, const HexXml::CaseDoc& c, const HexXml::RulesDoc& rulesDoc)
  {
    const fs::path& saveFile = *c.save;
    const HexXml::SaveDoc printed = HexXml::SaveDoc::parse(HexXml::XmlDocument::load(saveFile));
    const HexXml::CounterSetDoc counters = countersPerUnit(
        HexXml::CounterSetDoc::parse(HexXml::XmlDocument::load(countersOf(printed, saveFile))),
        printed);
    const HexXml::PackageDoc package = bindingsFor(c, counters, printed);
    HexXml::SaveDoc save = printed;
    for (HexXml::SaveUnitDoc& unit : save.units) {
      unit.counter = unit.id;
    }

    auto rules =
        std::make_shared<const HexRules::RuleSet>(HexRules::RuleSetBuilder::build(rulesDoc));
    const HexXml::SheetDoc sheet = HexXml::SheetDoc::parse(HexXml::XmlDocument::load(*c.sheet));
    auto board = std::make_shared<const HexModel::Board>(
        HexModel::BoardBuilder::build(sheet, *rules, package));
    auto roster = std::make_shared<const HexModel::Roster>(
        HexModel::RosterBuilder::build(counters, *rules, package, &HexEngine::defaultValueLine));
    auto definition = std::make_shared<const HexRules::GameDefinition>(
        HexRules::GameDefinition{rules, board, roster, file});

    const HexEngine::NoGameStateCodec state(*rules);
    const HexEngine::NoObligationCodec obligations;
    return Game{definition,
                HexModel::PositionBuilder::build(save, *board, *roster, *rules, state, obligations),
                save.seed};
  }

  HexEngine::Ctx
  ctxOf(const Game& game)
  {
    return HexEngine::Ctx{*game.definition->board, *game.definition->rules,
                          *game.definition->roster, game.position};
  }

  std::string
  hexName(const Game& game, HexIndex hex)
  {
    return game.definition->board->id(hex).text;
  }

  std::string
  unitName(const Game& game, UnitId unit)
  {
    return game.definition->roster->unit(unit).counter.text;
  }

  UnitId
  unitNamed(const Game& game, const std::string& id)
  {
    const std::optional<UnitId> found = game.definition->roster->find(HexModel::CounterId{id});
    if (!found) {
      throw std::invalid_argument("ConformanceTest: the save has no unit '" + id + "'");
    }
    return *found;
  }

  std::optional<HexIndex>
  hexOf(const HexModel::Position& position, UnitId unit)
  {
    const HexModel::UnitState& state = position.unit(unit);
    if (!state.where || !std::holds_alternative<HexIndex>(*state.where)) {
      return std::nullopt;
    }
    return std::get<HexIndex>(*state.where);
  }

  HexIndex
  originOf(const Game& game, UnitId unit)
  {
    const std::optional<HexIndex> hex = hexOf(game.position, unit);
    if (!hex) {
      throw std::invalid_argument("ConformanceTest: unit '" + unitName(game, unit) +
                                  "' is not on a hex");
    }
    return *hex;
  }

  std::vector<HexIndex>
  everyHex(const Game& game)
  {
    std::vector<HexIndex> out;
    for (std::size_t h = 0; h < game.definition->board->hexCount(); ++h) {
      out.push_back(HexIndex{static_cast<std::uint32_t>(h)});
    }
    return out;
  }

  // ---- the answers ------------------------------------------------------------------------------

  // Every hex into which some unit of the side projects a zone of control, its own hex included.
  std::vector<std::string>
  zocAnswer(const Game& game, const std::string& sideId)
  {
    const HexEngine::Ctx ctx = ctxOf(game);
    const HexEngine::SixHexZoc zoc(ctx.rules);
    const std::vector<UnitId> units = ctx.roster.ofSide(ctx.rules.side(sideId));
    std::vector<std::string> out;
    for (HexIndex to : everyHex(game)) {
      const bool coveredP = std::any_of(units.begin(), units.end(), [&](UnitId unit) {
        const std::optional<HexIndex> from = hexOf(ctx.position, unit);
        return from && zoc.projectsIntoP(ctx, unit, *from, to);
      });
      if (coveredP) {
        out.push_back(hexName(game, to));
      }
    }
    return out;
  }

  std::vector<std::string>
  stackingAnswer(const Game& game)
  {
    const HexEngine::Ctx ctx = ctxOf(game);
    const HexEngine::CountStacking stacking(ctx.rules);
    std::vector<std::string> out;
    for (HexIndex hex : everyHex(game)) {
      for (UnitId unit : stacking.excess(ctx, hex)) {
        out.push_back(unitName(game, unit));
      }
    }
    return out;
  }

  std::vector<std::string>
  retreatAnswer(const Game& game, const std::string& unitId)
  {
    const HexEngine::Ctx ctx = ctxOf(game);
    const HexEngine::SixHexZoc zoc(ctx.rules);
    const HexEngine::AdjacentRetreat retreat(ctx.rules, zoc);
    const UnitId unit = unitNamed(game, unitId);
    std::vector<std::string> out;
    for (HexIndex hex : retreat.candidates(ctx, unit, originOf(game, unit))) {
      out.push_back(hexName(game, hex));
    }
    return out;
  }

  // Session::reachable, the search behind every legal move, leaves out the unit's own hex;
  // hexcase's reach includes it.
  std::vector<std::string>
  reachAnswer(const Game& game, const std::string& unitId)
  {
    const HexEngine::DefaultPolicySet policies(*game.definition, HexEngine::GameSteps::Withheld);
    const HexEngine::Session session(game.definition, policies.policies(), game.position,
                                     game.seed);
    const UnitId unit = unitNamed(game, unitId);
    const HexEngine::Reachability field =
        session.reachable(std::span<const UnitId>(&unit, 1), HexModel::ModeId{0});
    std::vector<std::string> out{hexName(game, originOf(game, unit))};
    for (HexIndex hex : field.hexes) {
      out.push_back(hexName(game, hex));
    }
    return out;
  }

  std::vector<std::string>
  setAnswer(const Game& game, const HexXml::CaseQueryDoc& query)
  {
    if ("zoc" == query.kind) {
      return zocAnswer(game, *query.side);
    }
    if ("stacking-excess" == query.kind) {
      return stackingAnswer(game);
    }
    if ("retreat-candidates" == query.kind) {
      return retreatAnswer(game, *query.unit);
    }
    if ("reach" == query.kind) {
      return reachAnswer(game, *query.unit);
    }
    throw std::invalid_argument("ConformanceTest: no set answer for query kind '" + query.kind +
                                "'");
  }

  // The table's code for the given strengths and die value: the resolver's own column and table
  // lookups, with no modifier, no column shift and no die drawn.
  std::string
  crtAnswer(const HexXml::CaseQueryDoc& query, const HexXml::RulesDoc& rulesDoc)
  {
    const HexRules::RuleSet rules = HexRules::RuleSetBuilder::build(rulesDoc);
    const HexEngine::OddsTableResolver resolver(rules);
    const HexModel::OddsOutcome column =
        resolver.column(HexModel::Strength{static_cast<int>(*query.attack)},
                        HexModel::Strength{static_cast<int>(*query.defence)}, 0);
    if (std::holds_alternative<HexModel::BelowMinimum>(column)) {
      return "below-minimum";
    }
    if (std::holds_alternative<HexModel::AboveMaximum>(column)) {
      return "above-maximum";
    }
    return resolver.code(std::get<HexModel::Odds>(column), *query.roll);
  }

  std::string
  caseName(const ::testing::TestParamInfo<std::string>& info)
  {
    std::string name = fs::path(info.param).stem().string();
    std::replace(name.begin(), name.end(), '-', '_');
    return name;
  }

  class Conformance : public ::testing::TestWithParam<std::string> {};

}  // namespace

TEST(ConformanceCorpus, HoldsCases)
{
  EXPECT_FALSE(caseFiles().empty()) << "no <case> document under " << casesDir().generic_string();
}

TEST_P(Conformance, AnswersAsExpected)
{
  const fs::path file(GetParam());
  const HexXml::CaseDoc c = HexXml::CaseDoc::parse(HexXml::XmlDocument::load(file), file);
  const HexXml::RulesDoc rules = readRules(c.rules);
  if (const std::optional<std::string> reason = unsupported(c, rules)) {
    GTEST_SKIP() << GetParam() << ": " << *reason;
  }
  if ("crt" == c.query.kind) {
    const std::string actual = crtAnswer(c.query, rules);
    EXPECT_EQ(*c.expect.code, actual)
        << GetParam() << ": expected code " << *c.expect.code << ", hexgames gives " << actual;
    return;
  }
  const std::vector<std::string> expected =
      sorted(c.expect.hexes ? *c.expect.hexes : *c.expect.units);
  const std::vector<std::string> actual = sorted(setAnswer(gameOf(file, c, rules), c.query));
  EXPECT_EQ(expected, actual) << GetParam() << ": expected " << joined(expected)
                              << ", hexgames gives " << joined(actual);
}

INSTANTIATE_TEST_SUITE_P(HexRulesCases, Conformance, ::testing::ValuesIn(caseFiles()), caseName);
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
