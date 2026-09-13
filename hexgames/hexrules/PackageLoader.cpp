// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexrules/Package.h"

#include "hexmodel/BoardBuilder.h"
#include "hexmodel/RosterBuilder.h"
#include "hexrules/RuleSetBuilder.h"
#include "hexxml/CounterSetDoc.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/SaveDoc.h"
#include "hexxml/SheetDoc.h"
#include "hexxml/XmlDocument.h"

#include <algorithm>
#include <optional>
#include <set>
#include <stdexcept>

namespace HexRules {

  namespace {

    HexXml::PackageDoc
    loadPackage(const std::filesystem::path& manifest)
    {
      return HexXml::PackageDoc::parse(HexXml::XmlDocument::load(manifest));
    }

    RuleSet
    loadRules(const std::filesystem::path& base, const HexXml::PackageDoc& package)
    {
      const HexXml::RulesDoc doc = HexXml::RulesDoc::parse(HexXml::XmlDocument::load(base / package.rules.path));
      return RuleSetBuilder::build(doc);
    }

    HexXml::SheetDoc
    loadSheet(const std::filesystem::path& base, const HexXml::PackageDoc& package)
    {
      if (package.sheets.empty()) {
        throw std::invalid_argument("PackageLoader: package '" + package.id + "' names no sheet document");
      }
      return HexXml::SheetDoc::parse(HexXml::XmlDocument::load(base / package.sheets.front().path));
    }

    HexXml::CounterSetDoc
    loadCounters(const std::filesystem::path& base, const HexXml::PackageDoc& package)
    {
      if (package.counters.empty()) {
        throw std::invalid_argument("PackageLoader: package '" + package.id + "' names no counters document");
      }
      return HexXml::CounterSetDoc::parse(HexXml::XmlDocument::load(base / package.counters.front().path));
    }

  }  // namespace

  GameDefinition
  PackageLoader::load(const std::filesystem::path& manifest, const ValueLineReader& reader)
  {
    const HexXml::PackageDoc package = loadPackage(manifest);
    const std::filesystem::path base = manifest.parent_path();

    auto rules = std::make_shared<RuleSet>(loadRules(base, package));
    const HexXml::SheetDoc sheetDoc = loadSheet(base, package);
    auto board = std::make_shared<Board>(HexModel::BoardBuilder::build(sheetDoc, *rules, package));
    const HexXml::CounterSetDoc counterDoc = loadCounters(base, package);
    auto roster = std::make_shared<Roster>(HexModel::RosterBuilder::build(counterDoc, *rules, package, reader));

    GameDefinition def;
    def.rules = rules;
    def.board = board;
    def.roster = roster;
    def.packagePath = manifest;
    return def;
  }

  std::vector<PackageProblem>
  PackageLoader::check(const std::filesystem::path& manifest, const ValueLineReader& reader)
  {
    std::vector<PackageProblem> problems;
    const std::string manifestStr = manifest.string();
    const auto addProblem = [&](const std::string& message) {
      problems.push_back(PackageProblem{manifestStr, 0, message});
    };

    HexXml::PackageDoc package;
    try {
      package = loadPackage(manifest);
    } catch (const std::exception& e) {
      addProblem(e.what());
      return problems;
    }
    const std::filesystem::path base = manifest.parent_path();

    std::optional<RuleSet> rules;
    try {
      rules = loadRules(base, package);
    } catch (const std::exception& e) {
      addProblem(e.what());
    }

    std::optional<HexXml::SheetDoc> sheetDoc;
    try {
      sheetDoc = loadSheet(base, package);
    } catch (const std::exception& e) {
      addProblem(e.what());
    }

    std::optional<Board> board;
    if (rules && sheetDoc) {
      try {
        board = HexModel::BoardBuilder::build(*sheetDoc, *rules, package);
      } catch (const std::exception& e) {
        addProblem(e.what());
      }
    }

    std::optional<HexXml::CounterSetDoc> counterDoc;
    try {
      counterDoc = loadCounters(base, package);
    } catch (const std::exception& e) {
      addProblem(e.what());
    }
    if (rules && counterDoc) {
      try {
        (void)HexModel::RosterBuilder::build(*counterDoc, *rules, package, reader);
      } catch (const std::exception& e) {
        addProblem(e.what());
      }
    }

    if (sheetDoc) {
      for (const HexXml::PackageSpaceBindingDoc& sb : package.space) {
        if (!sb.panel) {
          continue;
        }
        const bool found =
            std::any_of(sheetDoc->panels.begin(), sheetDoc->panels.end(),
                        [&](const HexXml::SheetPanelDoc& p) { return p.id == *sb.panel; });
        if (!found) {
          addProblem("space '" + sb.rules + "' is bound to unknown panel '" + *sb.panel + "'");
        }
      }
    }

    // Checked against the sheet's own listed hex ids directly, not the built Board: a sheet problem
    // that keeps BoardBuilder from finishing (an unbound terrain, say) should not also hide an
    // unrelated scenario problem, since check() is meant to collect every independent one.
    if (sheetDoc) {
      std::set<std::string> knownHexIds;
      for (const HexXml::SheetHexesDoc& bulk : sheetDoc->hexesBulk) {
        knownHexIds.insert(bulk.ids.begin(), bulk.ids.end());
      }
      for (const HexXml::SheetHexDoc& h : sheetDoc->hexes) {
        knownHexIds.insert(h.id);
      }
      for (const HexXml::PackageScenarioDoc& sc : package.scenarios) {
        try {
          const HexXml::SaveDoc save = HexXml::SaveDoc::parse(HexXml::XmlDocument::load(base / sc.path));
          for (const HexXml::SaveUnitDoc& u : save.units) {
            if (u.hex && knownHexIds.end() == knownHexIds.find(*u.hex)) {
              addProblem("scenario '" + sc.id + "' places unit '" + u.id + "' on unknown hex '" + *u.hex + "'");
            }
          }
        } catch (const std::exception& e) {
          addProblem(e.what());
        }
      }
    }

    return problems;
  }

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
