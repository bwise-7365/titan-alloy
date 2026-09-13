// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Events: everything that happens in a game, as values. The log is the golden record's body and the
// observer interface is how views (Qt today, a browser later) follow play without touching state.
// ----------------------------------------------
#pragma once
#include "hexengine/PrngStreams.h"
#include "hexmodel/Ids.h"

#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace HexEngine {

  using namespace HexModel;

  struct PhaseEntered { int turn; PhaseId phase; std::optional<SideId> side; };
  struct UnitMoved { UnitId unit; std::vector<HexIndex> path; };
  struct UnitPlaced { UnitId unit; std::variant<HexIndex, SpaceId> where; };
  struct UnitReduced { UnitId unit; int stepsLeft; };
  struct UnitEliminated { UnitId unit; std::optional<SpaceId> to; };
  struct UnitRevealed { UnitId unit; };
  struct CombatDeclared { std::vector<UnitId> attackers; HexIndex target; };
  struct CombatResolved { HexIndex target; std::string odds; std::string outcome; };
  struct Retreated { UnitId unit; std::vector<HexIndex> path; };
  struct ControlChanged { HexIndex hex; std::optional<SideId> side; };
  struct SupplyChecked { UnitId unit; bool suppliedP; };
  struct DieRolled { StreamTag stream; int value; };
  struct CardDrawn { RandomizerId deck; std::string card; };
  struct DecisionRequested { std::string what; };
  struct DecisionAnswered { std::string what; std::string answer; };
  struct VictoryDeclared { std::optional<SideId> winner; std::string condition; };
  struct GameEvent { std::string kind; std::string text; };  // game-specific, in words

  using Event = std::variant<PhaseEntered, UnitMoved, UnitPlaced, UnitReduced, UnitEliminated, UnitRevealed,
                             CombatDeclared, CombatResolved, Retreated, ControlChanged, SupplyChecked,
                             DieRolled, CardDrawn, DecisionRequested, DecisionAnswered, VictoryDeclared, GameEvent>;

  class EventLog {
  public:
    void append(Event);
    const std::vector<Event>& events() const { return events_; }
    std::size_t size() const { return events_.size(); }
  private:
    std::vector<Event> events_;
  };

  class EventSink {
  public:
    virtual ~EventSink() = default;
    virtual void onEvent(const Event&) = 0;
  };

  // Added in M4: the attributes hexsave's <event> element carries, which are also exactly the
  // fields the one-line text form prints. Keeping them as a value lets hexrecord write a golden
  // without re-parsing the line.
  struct EventFields {
    std::string kind;
    std::optional<std::string> unit;
    std::optional<std::string> hex;
    std::optional<std::string> side;
    std::optional<std::string> value;
    std::string text;
  };

  // One line per event, the golden record's text form; needs the Board and Roster for names.
  class TextEventEncoder {
  public:
    static std::string line(const Event&, const class GameNames&);
    static EventFields fields(const Event&, const class GameNames&);
  };

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
