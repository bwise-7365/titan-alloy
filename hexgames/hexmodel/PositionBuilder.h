// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Builds a Position from a hexsave document's units/control/regions/tracks, resolving every id
// against the Board, the Roster and the RuleSet. The header only forward-declares HexRules::RuleSet;
// the .cpp that defines PositionBuilder::build is compiled into the hexrules static library (see
// BoardBuilder.h for why).
// ----------------------------------------------
#pragma once
#include "hexmodel/Board.h"
#include "hexmodel/Position.h"
#include "hexmodel/Roster.h"
#include "hexxml/SaveDoc.h"

namespace HexRules {
  class RuleSet;
}

namespace HexModel {

  class PositionBuilder {
  public:
    // Throws std::invalid_argument naming the offending id (a unit's counter, a hex, a space, a
    // side, a network, a layer or a region) on any reference the save document makes that the
    // board, roster or rules do not recognise. The flags on every <side> go to the game's state codec,
    // whose state the position holds (M6b); the codec throws naming a flag it cannot read. The
    // <resolution> entries are pushed bottom first with the decision each asked (M6b review); a
    // game entry goes to the ObligationCodec, and an entry missing an attribute its kind needs is
    // refused, naming the entry and the attribute.
    static Position build(const HexXml::SaveDoc&, const Board&, const Roster&, const HexRules::RuleSet&,
                          const GameStateCodec&, const ObligationCodec&);

  private:
    PositionBuilder() = delete;
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
