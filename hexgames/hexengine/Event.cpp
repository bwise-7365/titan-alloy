// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// One stable line per event. The line is the flat rendering of the same fields hexsave's <event>
// element carries, so a golden's events and the text log can never drift apart.
// ----------------------------------------------
#include "hexengine/Event.h"

#include "hexengine/GameNames.h"

#include <type_traits>

namespace HexEngine {

  namespace {

    std::string
    joinHexes(const std::vector<HexIndex>& hexes, const GameNames& names)
    {
      std::string out;
      for (std::size_t i = 0; i < hexes.size(); ++i) {
        if (0 != i) {
          out += ' ';
        }
        out += names.hex(hexes[i]);
      }
      return out;
    }

    std::string
    joinUnits(const std::vector<UnitId>& units, const GameNames& names)
    {
      std::string out;
      for (std::size_t i = 0; i < units.size(); ++i) {
        if (0 != i) {
          out += ' ';
        }
        out += names.counter(units[i]);
      }
      return out;
    }

    std::string
    sideOrNone(const std::optional<SideId>& side, const GameNames& names)
    {
      return side ? names.side(*side) : std::string("none");
    }

  }  // namespace

  void
  EventLog::append(Event event)
  {
    events_.push_back(std::move(event));
    return;
  }

  EventFields
  TextEventEncoder::fields(const Event& event, const GameNames& names)
  {
    EventFields out;
    std::visit(
        [&](auto&& e) {
          using T = std::decay_t<decltype(e)>;
          if constexpr (std::is_same_v<T, PhaseEntered>) {
            out.kind = "phase";
            out.side = sideOrNone(e.side, names);
            out.value = names.phase(e.phase);
            out.text = "turn " + std::to_string(e.turn);
          } else if constexpr (std::is_same_v<T, UnitMoved>) {
            out.kind = "moved";
            out.unit = names.counter(e.unit);
            if (!e.path.empty()) {
              out.hex = names.hex(e.path.back());
            }
            out.value = joinHexes(e.path, names);
          } else if constexpr (std::is_same_v<T, UnitPlaced>) {
            out.kind = "placed";
            out.unit = names.counter(e.unit);
            if (std::holds_alternative<HexIndex>(e.where)) {
              out.hex = names.hex(std::get<HexIndex>(e.where));
            } else {
              out.value = names.space(std::get<SpaceId>(e.where));
            }
          } else if constexpr (std::is_same_v<T, UnitReduced>) {
            out.kind = "reduced";
            out.unit = names.counter(e.unit);
            out.value = std::to_string(e.stepsLeft);
          } else if constexpr (std::is_same_v<T, UnitEliminated>) {
            out.kind = "eliminated";
            out.unit = names.counter(e.unit);
            if (e.to) {
              out.value = names.space(*e.to);
            }
          } else if constexpr (std::is_same_v<T, UnitRevealed>) {
            out.kind = "revealed";
            out.unit = names.counter(e.unit);
          } else if constexpr (std::is_same_v<T, CombatDeclared>) {
            out.kind = "combat-declared";
            out.hex = names.hex(e.target);
            out.value = joinUnits(e.attackers, names);
          } else if constexpr (std::is_same_v<T, CombatResolved>) {
            out.kind = "combat-resolved";
            out.hex = names.hex(e.target);
            out.value = e.odds;
            out.text = e.outcome;
          } else if constexpr (std::is_same_v<T, Retreated>) {
            out.kind = "retreated";
            out.unit = names.counter(e.unit);
            if (!e.path.empty()) {
              out.hex = names.hex(e.path.back());
            }
            out.value = joinHexes(e.path, names);
          } else if constexpr (std::is_same_v<T, ControlChanged>) {
            out.kind = "control";
            out.hex = names.hex(e.hex);
            out.side = sideOrNone(e.side, names);
          } else if constexpr (std::is_same_v<T, SupplyChecked>) {
            out.kind = "supply";
            out.unit = names.counter(e.unit);
            out.value = e.suppliedP ? "supplied" : "isolated";
          } else if constexpr (std::is_same_v<T, DieRolled>) {
            out.kind = "die";
            out.value = std::to_string(e.value);
            out.text = std::string(streamName(e.stream));
          } else if constexpr (std::is_same_v<T, CardDrawn>) {
            out.kind = "card";
            out.value = e.card;
            out.text = names.randomizer(e.deck);
          } else if constexpr (std::is_same_v<T, DecisionRequested>) {
            out.kind = "decision-requested";
            out.value = e.what;
          } else if constexpr (std::is_same_v<T, DecisionAnswered>) {
            out.kind = "decision-answered";
            out.value = e.what;
            out.text = e.answer;
          } else if constexpr (std::is_same_v<T, VictoryDeclared>) {
            out.kind = "victory";
            out.side = sideOrNone(e.winner, names);
            out.text = e.condition;
          } else if constexpr (std::is_same_v<T, GameEvent>) {
            out.kind = e.kind;
            out.text = e.text;
          }
        },
        event);
    return out;
  }

  std::string
  TextEventEncoder::line(const Event& event, const GameNames& names)
  {
    const EventFields f = fields(event, names);
    std::string out = f.kind;
    if (f.unit) {
      out += " unit=" + *f.unit;
    }
    if (f.hex) {
      out += " hex=" + *f.hex;
    }
    if (f.side) {
      out += " side=" + *f.side;
    }
    if (f.value) {
      out += " value=" + *f.value;
    }
    if (!f.text.empty()) {
      out += " " + f.text;
    }
    return out;
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
