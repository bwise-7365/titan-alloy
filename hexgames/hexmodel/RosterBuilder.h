// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Builds a Roster from a hexcounters document, a loaded RuleSet, a package's unit bindings and the
// game's ValueLineReader. The header only forward-declares HexRules::RuleSet; the .cpp that defines
// RosterBuilder::build is compiled into the hexrules static library (see BoardBuilder.h for why).
// ----------------------------------------------
#pragma once
#include "hexmodel/Roster.h"
#include "hexxml/CounterSetDoc.h"
#include "hexxml/PackageDoc.h"

#include <functional>
#include <string_view>

namespace HexRules {
  class RuleSet;
}

namespace HexModel {

  class RosterBuilder {
  public:
    // Throws std::invalid_argument naming the counter id when it matches zero or more than one unit
    // binding, when the bound type is unknown to the rules, or when its value line does not parse.
    // Marker-family counters are skipped.
    //
    // OPEN QUESTION (see the task log): a unit-type's own @side restricts most counters to one side
    // already, but a handful of shared, side-agnostic types (infantry, armour, HQ, leader ...) are
    // the very same rules unit-type on every side, so the counter's own side cannot be read out of
    // hexrules.xsd or hexpackage.xsd at all -- it is carried only by the counter id's own naming
    // convention (TRC: "g-"/"r-" nationality prefixes), which is a per-game fact neither this
    // builder nor PackageLoader::load's fixed (manifest, ValueLineReader) signature has anywhere to
    // receive. Pending a real mechanism (a package binding addition, most likely), RosterBuilder
    // tries the "countries"-shaped region layer (a region whose id equals the counter's style) and
    // otherwise assigns the rules' first side as an explicit, flagged placeholder -- never a throw,
    // since no test in the accepted list checks UnitSpec::side, but not a correct answer either.
    static Roster build(const HexXml::CounterSetDoc&, const HexRules::RuleSet&, const HexXml::PackageDoc&,
                        const ValueLineReader&);

  private:
    RosterBuilder() = delete;
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
