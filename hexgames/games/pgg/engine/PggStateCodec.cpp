// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PggState <-> hexsave side flags. Every flag is parsed and checked here, once; a flag that is
// unknown, on the wrong side, repeated or unreadable throws naming the flag and its value. Lists are
// space-separated tokens: counter ids, hex ids, "counter:halves", "counter:hex", "hex:turn" and
// "area:rifles:armour". A zero count and an empty list are written as no flag.
// ----------------------------------------------
#include "PggState.h"

#include <stdexcept>

namespace Pgg {

  namespace {

    using HexModel::SideFlag;

    [[noreturn]] void
    refuse(const SideFlag& flag, const std::string& why)
    {
      throw std::invalid_argument("PggStateCodec: flag '" + flag.name + "' (value '" + flag.value + "') " + why);
    }

    std::vector<std::string>
    tokensOf(const std::string& text)
    {
      std::vector<std::string> out;
      std::string current;
      for (char c : text + " ") {
        if (' ' == c) {
          if (!current.empty()) {
            out.push_back(current);
          }
          current.clear();
        } else {
          current += c;
        }
      }
      return out;
    }

    std::vector<std::string>
    partsOf(const SideFlag& flag, const std::string& token, std::size_t count)
    {
      std::vector<std::string> out;
      std::size_t start = 0;
      for (std::size_t colon = token.find(':'); std::string::npos != colon; colon = token.find(':', start)) {
        out.push_back(token.substr(start, colon - start));
        start = colon + 1;
      }
      out.push_back(token.substr(start));
      if (count != out.size()) {
        refuse(flag, "has token '" + token + "' with " + std::to_string(out.size()) + " parts, not " +
                         std::to_string(count));
      }
      return out;
    }

    int
    numberOf(const SideFlag& flag, const std::string& text)
    {
      try {
        std::size_t used = 0;
        const int value = std::stoi(text, &used);
        if (used == text.size()) {
          return value;
        }
      } catch (const std::exception&) {
      }
      refuse(flag, "has '" + text + "', which is not a whole number");
    }

    template <class Fn>
    auto
    named(const SideFlag& flag, const std::string& token, const Fn& lookup)
    {
      try {
        return lookup(token);
      } catch (const std::invalid_argument&) {
        refuse(flag, "names '" + token + "', which the game does not know");
      }
    }

    std::string
    joined(const std::vector<std::string>& items)
    {
      std::string out;
      for (const std::string& item : items) {
        out += (out.empty() ? "" : " ") + item;
      }
      return out;
    }

  }  // namespace

  PggStateCodec::PggStateCodec(const PggFacts& facts, const HexEngine::GameNames& names) : facts_(facts), names_(names)
  {
  }

  void
  PggStateCodec::decodeGerman(PggState& state, const SideFlag& flag) const
  {
    const auto hexOf = [&](const std::string& token) { return names_.hexOf(token); };
    if ("air-interdiction" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        state.airInterdiction.push_back(named(flag, token, hexOf));
      }
    } else if ("rail-cuts" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        state.railCuts.insert(named(flag, token, hexOf));
      }
    } else if ("rail-repaired" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        const std::vector<std::string> parts = partsOf(flag, token, 2);
        state.railRepaired[named(flag, parts[0], hexOf)] = numberOf(flag, parts[1]);
      }
    } else if ("passed" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        state.passedByGermans.insert(named(flag, token, hexOf));
      }
    } else if ("smolensk-taken" == flag.name) {
      state.smolenskTaken = numberOf(flag, flag.value);
    } else if ("german-held" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        state.germanHeld.insert(named(flag, token, hexOf));
      }
    } else if ("eliminated" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        state.germanEliminated.insert(named(flag, token, [&](const std::string& t) { return names_.unitOf(t); }));
      }
    } else if ("outcome" == flag.name) {
      const std::vector<VictoryLevel>& levels = facts_.victoryLevels();
      for (std::size_t i = 0; i < levels.size(); ++i) {
        if (flag.value == levels[i].code) {
          state.outcome = i;
        }
      }
      if (!state.outcome) {
        refuse(flag, "is not a code of the victory-levels list");
      }
    } else {
      refuse(flag, "is not a PGG flag of side 'german'");
    }
    return;
  }

  void
  PggStateCodec::decodeSoviet(PggState& state, const SideFlag& flag) const
  {
    if ("interdiction" == flag.name) {
      state.sovietInterdiction = named(flag, flag.value, [&](const std::string& t) { return names_.hexOf(t); });
    } else if ("interdiction-turns" == flag.name) {
      state.sovietInterdictionTurns = numberOf(flag, flag.value);
    } else if ("swf-used" == flag.name) {
      state.swfUsed = numberOf(flag, flag.value);
    } else if ("swf-this-turn" == flag.name) {
      state.swfThisTurn = numberOf(flag, flag.value);
    } else if ("rail-units" == flag.name) {
      state.railUnits = numberOf(flag, flag.value);
    } else if ("recapture-vp" == flag.name) {
      state.recaptureVp = numberOf(flag, flag.value);
    } else if ("owed" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        const std::vector<std::string> parts = partsOf(flag, token, 3);
        state.owed[named(flag, parts[0], [](const std::string& t) { return areaNamed(t); })] =
            Owed{numberOf(flag, parts[1]), numberOf(flag, parts[2])};
      }
    } else if (flag.name.starts_with("army-")) {
      const Army army = named(flag, flag.name.substr(5), [&](const std::string& t) { return armyNumbered(numberOf(flag, t)); });
      for (const std::string& token : tokensOf(flag.value)) {
        state.armies[named(flag, token, [&](const std::string& t) { return names_.unitOf(t); })] = army;
      }
    } else if ("frozen" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        state.frozen.insert(named(flag, token, [&](const std::string& t) { return armyNumbered(numberOf(flag, t)); }));
      }
    } else {
      refuse(flag, "is not a PGG flag of side 'soviet'");
    }
    return;
  }

  void
  PggStateCodec::decodeSide(PggSideState& side, SideId owner, const SideFlag& flag) const
  {
    const auto unitOf = [&](const std::string& token) {
      const UnitId unit = named(flag, token, [&](const std::string& t) { return names_.unitOf(t); });
      if (owner != facts_.definition().roster->unit(unit).side) {
        refuse(flag, "names counter '" + token + "' of the other side");
      }
      return unit;
    };
    const auto units = [&](std::set<UnitId>& into) {
      for (const std::string& token : tokensOf(flag.value)) {
        into.insert(unitOf(token));
      }
      return;
    };
    if ("disrupted" == flag.name) {
      units(side.disrupted);
    } else if ("unsupplied" == flag.name) {
      units(side.unsupplied);
    } else if ("beyond-radius" == flag.name) {
      units(side.beyondRadius);
    } else if ("entered" == flag.name) {
      units(side.entered);
    } else if ("halted" == flag.name) {
      units(side.halted);
    } else if ("continuing" == flag.name) {
      units(side.continuing);
    } else if ("retreated-onto" == flag.name) {
      units(side.retreatedOnto);
    } else if ("spent" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        const std::vector<std::string> parts = partsOf(flag, token, 2);
        side.spent[unitOf(parts[0])] = numberOf(flag, parts[1]);
      }
    } else if ("zoc-entry" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        const std::vector<std::string> parts = partsOf(flag, token, 2);
        side.zocEntry[unitOf(parts[0])] = named(flag, parts[1], [&](const std::string& t) { return names_.hexOf(t); });
      }
    } else {
      refuse(flag, "is not a per-side PGG flag");
    }
    return;
  }

  HexModel::Polymorphic<HexModel::GameState>
  PggStateCodec::decode(const HexModel::SideFlags& flags) const
  {
    const std::set<std::string> perSide{"disrupted", "unsupplied", "beyond-radius", "entered", "halted",
                                        "continuing", "retreated-onto", "spent", "zoc-entry"};
    if (2 != flags.size()) {
      throw std::invalid_argument("PggStateCodec: flags for " + std::to_string(flags.size()) + " sides, PGG has two");
    }
    PggState state(flags.size());
    for (std::size_t s = 0; s < flags.size(); ++s) {
      const SideId side{static_cast<std::uint32_t>(s)};
      std::set<std::string> seen;
      for (const SideFlag& flag : flags[s]) {
        if (!seen.insert(flag.name).second) {
          refuse(flag, "is given twice on one side");
        }
        if (perSide.contains(flag.name)) {
          decodeSide(state.side(side), side, flag);
        } else if (facts_.german() == side) {
          decodeGerman(state, flag);
        } else {
          decodeSoviet(state, flag);
        }
      }
    }
    return HexModel::makePolymorphic<HexModel::GameState, PggState>(std::move(state));
  }

  HexModel::SideFlags
  PggStateCodec::encode(const HexModel::GameState& held) const
  {
    const PggState* state = dynamic_cast<const PggState*>(&held);
    if (nullptr == state) {
      throw std::invalid_argument("PggStateCodec: the position holds no PGG state");
    }
    const auto counters = [&](const std::set<UnitId>& units) {
      std::vector<std::string> out;
      for (UnitId unit : units) {
        out.push_back(names_.counter(unit));
      }
      return joined(out);
    };
    const auto hexes = [&](const auto& list) {
      std::vector<std::string> out;
      for (HexIndex hex : list) {
        out.push_back(names_.hex(hex));
      }
      return joined(out);
    };
    HexModel::SideFlags out(state->sideCount());
    for (std::size_t s = 0; s < state->sideCount(); ++s) {
      const SideId side{static_cast<std::uint32_t>(s)};
      const PggSideState& one = state->side(side);
      std::map<std::string, std::string> put;
      put["disrupted"] = counters(one.disrupted);
      put["unsupplied"] = counters(one.unsupplied);
      put["beyond-radius"] = counters(one.beyondRadius);
      put["entered"] = counters(one.entered);
      put["halted"] = counters(one.halted);
      put["continuing"] = counters(one.continuing);
      put["retreated-onto"] = counters(one.retreatedOnto);
      std::vector<std::string> spent;
      for (const auto& [unit, halves] : one.spent) {
        spent.push_back(names_.counter(unit) + ":" + std::to_string(halves));
      }
      put["spent"] = joined(spent);
      std::vector<std::string> entries;
      for (const auto& [unit, hex] : one.zocEntry) {
        entries.push_back(names_.counter(unit) + ":" + names_.hex(hex));
      }
      put["zoc-entry"] = joined(entries);
      if (facts_.german() == side) {
        put["air-interdiction"] = hexes(state->airInterdiction);
        put["rail-cuts"] = hexes(state->railCuts);
        std::vector<std::string> repaired;
        for (const auto& [hex, turn] : state->railRepaired) {
          repaired.push_back(names_.hex(hex) + ":" + std::to_string(turn));
        }
        put["rail-repaired"] = joined(repaired);
        put["passed"] = hexes(state->passedByGermans);
        put["smolensk-taken"] = state->smolenskTaken ? std::to_string(*state->smolenskTaken) : "";
        put["german-held"] = hexes(state->germanHeld);
        put["eliminated"] = counters(state->germanEliminated);
        put["outcome"] = state->outcome ? facts_.victoryLevels().at(*state->outcome).code : "";
      } else {
        put["interdiction"] = state->sovietInterdiction ? names_.hex(*state->sovietInterdiction) : "";
        const auto count = [](int n) { return 0 == n ? std::string() : std::to_string(n); };
        put["interdiction-turns"] = count(state->sovietInterdictionTurns);
        put["swf-used"] = count(state->swfUsed);
        put["swf-this-turn"] = count(state->swfThisTurn);
        put["rail-units"] = count(state->railUnits);
        put["recapture-vp"] = count(state->recaptureVp);
        std::vector<std::string> owed;
        for (const auto& [area, owe] : state->owed) {
          if (Owed{} != owe) {
            owed.push_back(std::string(areaName(area)) + ":" + std::to_string(owe.rifles) + ":" + std::to_string(owe.armour));
          }
        }
        put["owed"] = joined(owed);
        std::map<Army, std::set<UnitId>> byArmy;
        for (const auto& [unit, army] : state->armies) {
          byArmy[army].insert(unit);
        }
        for (const auto& [army, units] : byArmy) {
          put["army-" + std::to_string(armyNumber(army))] = counters(units);
        }
        std::vector<std::string> frozen;
        for (Army army : state->frozen) {
          frozen.push_back(std::to_string(armyNumber(army)));
        }
        put["frozen"] = joined(frozen);
      }
      for (const auto& [name, value] : put) {
        if (!value.empty()) {
          out[s].push_back(SideFlag{name, value});
        }
      }
    }
    return out;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
