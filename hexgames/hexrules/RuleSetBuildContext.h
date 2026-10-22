// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The full name -> dense id tables RuleSetBuilder fills in as it mints each section, shared between
// RuleSetBuilder.cpp and RuleSetBuilderCombat.cpp. Not part of the public facade.
// ----------------------------------------------
#pragma once
#include "hexrules/RuleSetBuilder.h"

#include <map>
#include <string>
#include <vector>

namespace HexRules {

  struct RuleSetBuildContext {
    std::map<std::string, SideId> sideByName;
    std::map<std::string, UnitTypeId> unitTypeByName;
    std::map<std::string, TerrainId> terrainByName;
    std::map<std::string, EdgeTerrainId> edgeTerrainByName;
    std::map<std::string, NetworkId> networkByName;
    std::map<std::string, LayerId> layerByName;
    std::vector<std::map<std::string, RegionId>> regionByNamePerLayer;
    std::map<std::string, PhaseId> phaseByName;
    std::map<std::string, RandomizerId> randomizerByName;
  };

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
