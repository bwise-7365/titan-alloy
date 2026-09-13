// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The 5x5 synthetic board the hexsearch tests walk, built through the real BoardBuilder from the
// tiny toy-rules / toy-sheet / toy-package documents beside this header. Building a Board by hand
// is not possible outside BoardBuilder (it is Board's only friend), so the toy documents are the
// shortest honest route to one.
// ----------------------------------------------
#pragma once
#include "hexmodel/BoardBuilder.h"
#include "hexrules/RuleSetBuilder.h"
#include "hexsearch/Search.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/SheetDoc.h"
#include "hexxml/XmlDocument.h"

#include <filesystem>
#include <string>

namespace ToyBoard {

  inline std::filesystem::path
  testDir()
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexsearch" / "test";
  }

  inline HexModel::Board
  build()
  {
    const HexXml::RulesDoc rules = HexXml::RulesDoc::parse(HexXml::XmlDocument::load(testDir() / "toy-rules.xml"));
    const HexRules::RuleSet ruleSet = HexRules::RuleSetBuilder::build(rules);
    const HexXml::SheetDoc sheet = HexXml::SheetDoc::parse(HexXml::XmlDocument::load(testDir() / "toy-sheet.xml"));
    const HexXml::PackageDoc package =
        HexXml::PackageDoc::parse(HexXml::XmlDocument::load(testDir() / "toy-package.xml"));
    return HexModel::BoardBuilder::build(sheet, ruleSet, package);
  }

  inline HexModel::HexIndex
  at(const HexModel::Board& board, const std::string& id)
  {
    return board.indexOf(HexCoord::HexId{id});
  }

  inline HexSearch::NodeIndex
  node(const HexModel::Board& board, const std::string& id)
  {
    return at(board, id).value;
  }

  // True when the hexside carries the toy sheet's "wall" line, whichever side of it one stands on.
  inline bool
  walledP(const HexModel::Board& board, HexModel::HexIndex hex, HexModel::Direction direction)
  {
    return !board.edge(hex, direction).empty();
  }

  // An arc predicate over the toy board: every hexside is crossable except a walled one.
  inline HexSearch::HexAdjacencyGraph
  openGraph(const HexModel::Board& board)
  {
    return HexSearch::HexAdjacencyGraph(board, [](HexModel::HexIndex, HexModel::Direction) { return true; });
  }

  inline HexSearch::HexAdjacencyGraph
  walledGraph(const HexModel::Board& board)
  {
    return HexSearch::HexAdjacencyGraph(
        board, [&board](HexModel::HexIndex hex, HexModel::Direction d) { return !walledP(board, hex, d); });
  }

}  // namespace ToyBoard
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
