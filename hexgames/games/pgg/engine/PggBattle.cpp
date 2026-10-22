// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggBattle.h"

#include <array>
#include <stdexcept>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 5> kStages{"reveal", "defender", "attacker", "advance", "done"};

    void
    appendUnits(std::string& s, const std::vector<UnitId>& units)
    {
      for (UnitId unit : units) {
        s += std::to_string(unit.value) + ",";
      }
      s += ';';
      return;
    }

    void
    appendHexes(std::string& s, const std::vector<HexIndex>& hexes)
    {
      for (HexIndex hex : hexes) {
        s += std::to_string(hex.value) + ",";
      }
      s += ';';
      return;
    }

  }  // namespace

  std::string_view
  stageName(BattleStage stage)
  {
    return kStages[static_cast<std::size_t>(stage)];
  }

  BattleStage
  stageNamed(std::string_view text)
  {
    for (std::size_t i = 0; i < kStages.size(); ++i) {
      if (text == kStages[i]) {
        return static_cast<BattleStage>(i);
      }
    }
    throw std::invalid_argument("Pgg::stageNamed: '" + std::string(text) + "' is not a battle stage");
  }

  void
  PggBattle::appendDigest(std::string& s) const
  {
    appendUnits(s, attackers);
    appendUnits(s, defenders);
    s += std::to_string(target.value) + ";" + std::to_string(attackerSide.value) + ";" +
         std::to_string(defenderSide.value) + ";" + (overrunP ? "o;" : "-;") + code + ";" +
         std::string(stageName(stage)) + ";" + (startedP ? "s;" : "-;") + (choseP ? "c;" : "-;") +
         std::to_string(stepsLeft) + ";";
    appendUnits(s, retreatQueue);
    if (walk) {
      s += std::to_string(walk->unit.value) + ":" + std::to_string(walk->from.value) + ":" +
           std::to_string(walk->hexesLeft) + ":";
      appendHexes(s, walk->path);
    } else {
      s += "-;";
    }
    appendUnits(s, retreated);
    appendHexes(s, pathOfRetreat);
    s += std::string(defenderHitP ? "d;" : "-;") + (attackerHitP ? "a;" : "-;");
    s += (advancing ? std::to_string(advancing->value) : std::string("-")) + ";";
    appendUnits(s, advanced);
    return;
  }

  const PggBattle&
  battleOn(const Position& position)
  {
    const auto& owed = std::get<HexModel::Polymorphic<HexModel::GameObligation>>(position.top().owed);
    return owed.as<PggBattle>("the resolution stack's top");
  }

  PggBattle&
  battleOn(Position& position)
  {
    auto* owed = std::get_if<HexModel::Polymorphic<HexModel::GameObligation>>(&position.top().owed);
    if (nullptr == owed) {
      throw std::invalid_argument("Pgg::battleOn: the resolution stack's top is an engine obligation");
    }
    return owed->as<PggBattle>("the resolution stack's top");
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
