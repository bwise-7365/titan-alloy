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
    // UnitSpec::side comes from the package's <side rules="..." styles="..."/> bindings: the
    // counter's front-face style (the printed ground colour, which is the side on every sheet) names
    // exactly one rules side. A style bound to no side, or to two, and a counter bound to a side its
    // own unit-type's @side mask excludes, are each a throw naming the counter and the style.
    // PackageLoader::check collects the same problems for every counter instead of stopping at the
    // first (M4, task 04).
    static Roster build(const HexXml::CounterSetDoc&, const HexRules::RuleSet&, const HexXml::PackageDoc&,
                        const ValueLineReader&);

  private:
    RosterBuilder() = delete;
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
