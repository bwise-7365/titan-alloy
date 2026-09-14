// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// One battle's result on the resolution stack (a PGG game obligation): the attack or overrun, its
// CRT code and how far the result has been worked. Stages, in order: Reveal (12.2, 12.3), Defender
// (9.66: the defender's half first), Attacker, Advance (9.8; for an overrun, the free entry into a
// vacated hex, 6.51), Done. Within a side's stage the side answers "result" (a step from one of its
// units, or "retreat") and then "loss" for each further step; a retreat walks each unit in turn, the
// opponent routing it (9.72). PggObligations settles and answers it; PggBattleCodec writes it.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"

#include "hexengine/GameNames.h"
#include "hexmodel/Resolution.h"

#include <optional>
#include <string>
#include <vector>

namespace Pgg {

  enum class BattleStage : std::uint8_t { Reveal, Defender, Attacker, Advance, Done };
  std::string_view stageName(BattleStage);
  BattleStage stageNamed(std::string_view);  // throws naming the text

  struct RetreatWalk {
    UnitId unit;
    HexIndex from;
    int hexesLeft = 0;
    std::vector<HexIndex> path;
  };

  class PggBattle : public HexModel::Cloneable<PggBattle, HexModel::GameObligation> {
  public:
    std::string_view kind() const override { return "pgg-battle"; }
    void appendDigest(std::string&) const override;

    std::vector<UnitId> attackers;  // the attacking units and the Leaders adding their points (10.36)
    std::vector<UnitId> defenders;  // every unit in the target hex as the battle began
    HexIndex target;
    SideId attackerSide;
    SideId defenderSide;
    bool overrunP = false;
    std::string code;  // the CRT code, "void" (no attack strength) or "De" (no defence strength)
    BattleStage stage = BattleStage::Reveal;

    bool startedP = false;  // the current side's result has begun
    bool choseP = false;    // the current side has chosen a step loss or a retreat
    int stepsLeft = 0;
    std::vector<UnitId> retreatQueue;
    std::optional<RetreatWalk> walk;
    std::vector<UnitId> retreated;        // units that walked or could not
    std::vector<HexIndex> pathOfRetreat;  // the first defender's walk (9.81)
    bool defenderHitP = false;            // the defender lost a step or retreated
    bool attackerHitP = false;
    std::optional<UnitId> advancing;      // the unit in the vacated hex that may go a hex further (9.86)
    std::vector<UnitId> advanced;
  };

  // The battle on top of the position's resolution stack; throws when the top is anything else.
  const PggBattle& battleOn(const Position&);
  PggBattle& battleOn(Position&);

  class PggBattleCodec : public HexModel::ObligationCodec {
  public:
    explicit PggBattleCodec(const HexEngine::GameNames&);
    HexModel::Polymorphic<HexModel::GameObligation> decode(const std::string& name,
                                                           const std::vector<HexModel::ObligationArg>&) const override;
    std::vector<HexModel::ObligationArg> encode(const HexModel::GameObligation&) const override;

  private:
    const HexEngine::GameNames& names_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
