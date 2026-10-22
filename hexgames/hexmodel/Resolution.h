// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The resolution stack (M6b, replacing M6's combat-only CombatPlan): what the rules still owe
// before play goes on, one obligation per entry, the top settled first. An entry that needs a
// player's answer carries the decision it asked, and Position::pending() is the top entry's
// decision. The engine's own kinds are a battle's losses and retreats; a game adds its own by
// deriving from GameObligation (Dai Senso card effects, Tarawa's reveal-and-consult-again).
// ----------------------------------------------
#pragma once
#include "hexmodel/Ids.h"
#include "hexmodel/Polymorphic.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace HexModel {

  // A choice the rules require before adjudication can continue; the engine accepts only the
  // matching answer next. A combat decision names the side that answers it, which is not always
  // the side whose turn it is (TRC 13.3: the defender picks his own loss, the attacker routes).
  struct NoDecision {};
  struct ChooseLoss { SideId side; std::vector<UnitId> candidates; int count; };
  struct ChooseRetreat { SideId side; UnitId unit; std::vector<HexIndex> candidates; bool mayStopP = false; };
  struct ChooseCard { RandomizerId deck; std::vector<std::string> candidates; };
  // Changed in M7b: a game's choice may name the side that answers it (PGG 9.65: the defender
  // chooses between a step loss and a retreat in the attacker's phase); nullopt: the acting side.
  struct GameChoice { std::string verb; std::vector<std::string> options; std::optional<SideId> side; };
  using PendingDecision = std::variant<NoDecision, ChooseLoss, ChooseRetreat, ChooseCard, GameChoice>;

  // The battle an engine obligation belongs to: who routes its retreats (nullopt: each unit's
  // owner) and every unit that took part, attackers first.
  struct Battle {
    std::optional<SideId> router;
    std::vector<UnitId> involved;
  };

  struct OwedLoss { Battle battle; SideId side; int count; };                // steps a side still has to lose
  struct OwedRetreat { Battle battle; SideId side; int fewest; int most; };  // every unit of a side retreats
  struct UnitRetreat {                                                       // one unit's walk, part-way through
    Battle battle;
    UnitId unit;
    HexIndex from;
    int fewest;
    int most;
    std::vector<HexIndex> path;
  };

  // A game's own obligation. clone() copies it for a fork; kind() names it in messages;
  // appendDigest() writes its canonical text into Position::digest().
  class GameObligation {
  public:
    virtual ~GameObligation() = default;
    virtual std::unique_ptr<GameObligation> clone() const = 0;
    virtual std::string_view kind() const = 0;
    virtual void appendDigest(std::string&) const = 0;
  };

  // Added in M6b review: a game obligation in hexsave, <owe kind="game" name=".."><arg name value/>*.
  // The name is the obligation's kind(); the arguments are the game's own.
  struct ObligationArg {
    std::string name;
    std::string value;
  };
  class ObligationCodec {
  public:
    virtual ~ObligationCodec() = default;
    // Throws std::invalid_argument naming the obligation, or the argument it cannot read.
    virtual Polymorphic<GameObligation> decode(const std::string& name, const std::vector<ObligationArg>&) const = 0;
    virtual std::vector<ObligationArg> encode(const GameObligation&) const = 0;
  };

  using Obligation = std::variant<OwedLoss, OwedRetreat, UnitRetreat, Polymorphic<GameObligation>>;

  struct Resolution {
    Obligation owed;
    PendingDecision asked;  // NoDecision until the entry needs an answer
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
