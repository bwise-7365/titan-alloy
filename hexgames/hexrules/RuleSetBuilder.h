// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Builds a RuleSet from a parsed RulesDoc: mints every dense id (sides, unit types, hex and hexside
// terrain, networks, region layers and their regions, spaces, phases, randomizers, resolvers) and
// resolves every IDREFS attribute against them. Throws std::invalid_argument naming the offending id
// on any dangling reference, or on an asymmetric @hostile-to (naming both sides).
// ----------------------------------------------
#pragma once
#include "hexrules/RuleSet.h"
#include "hexxml/RulesDoc.h"

namespace HexRules {

  // The name -> dense id tables built as each section of RuleSetBuilder mints its ids, threaded
  // through the later sections that reference earlier ones by name; defined in the .cpp. A
  // namespace-scope type (not nested in RuleSetBuilder) so the free helper functions in the .cpp
  // can use it too; only RuleSetBuilder's own static members touch RuleSet's private vectors.
  struct RuleSetBuildContext;

  class RuleSetBuilder {
  public:
    static RuleSet build(const HexXml::RulesDoc&);

  private:
    RuleSetBuilder() = delete;

    using BuildContext = RuleSetBuildContext;

    static void buildSides(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildUnitTypes(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildTerrain(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildNetworks(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildLayers(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildSpaces(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildPhases(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildStacking(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildZocs(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildRandomizers(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildMovement(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildSupply(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildCombat(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildRetreat(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildWeather(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildVictory(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
    static void buildProse(RuleSet&, const HexXml::RulesDoc&, BuildContext&);
  };

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
