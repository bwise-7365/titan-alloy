// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TrcState <-> hexsave side flags. Every flag is parsed and checked here, once; a flag that is
// unknown, on the wrong side, repeated or unreadable throws naming the flag, its side and its value.
// Encoding writes each side's flags sorted by name, lists as sorted space-separated tokens.
// ----------------------------------------------
#include "TrcState.h"

#include <map>
#include <stdexcept>

namespace Trc {

  namespace {

    using HexModel::SideFlag;

    [[noreturn]] void
    refuse(const SideFlag& flag, const std::string& why)
    {
      throw std::invalid_argument("TrcStateCodec: flag '" + flag.name + "' (value '" + flag.value + "') " + why);
    }

    int
    countOf(const SideFlag& flag)
    {
      try {
        std::size_t used = 0;
        const int value = std::stoi(flag.value, &used);
        if (used == flag.value.size()) {
          return value;
        }
      } catch (const std::exception&) {
      }
      refuse(flag, "is not a whole number");
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

    std::string
    joined(const std::set<std::string>& items)
    {
      std::string out;
      for (const std::string& item : items) {
        out += (out.empty() ? "" : " ") + item;
      }
      return out;
    }

    LeaderLost
    leaderLostOf(const SideFlag& flag)
    {
      if ("pending" == flag.value) {
        return LeaderLost::Pending;
      }
      if ("active" == flag.value) {
        return LeaderLost::Active;
      }
      if ("spent" == flag.value) {
        return LeaderLost::Spent;
      }
      refuse(flag, "is not pending, active or spent");
    }

    std::string
    leaderLostName(LeaderLost state)
    {
      switch (state) {
        case LeaderLost::Pending:
          return "pending";
        case LeaderLost::Active:
          return "active";
        case LeaderLost::Spent:
          return "spent";
      }
      throw std::invalid_argument("TrcStateCodec: leader-lost state outside the three");
    }

    SeaArea
    seaAreaOf(const SideFlag& flag, const std::string& token)
    {
      for (SeaArea area : {SeaArea::Baltic, SeaArea::BlackSea, SeaArea::Caspian}) {
        if (seaAreaName(area) == token) {
          return area;
        }
      }
      refuse(flag, "names '" + token + "', which is not a sea area");
    }

    void
    putCount(std::map<std::string, std::string>& out, const char* name, const std::optional<int>& count)
    {
      if (count) {
        out[name] = std::to_string(*count);
      }
      return;
    }

    void
    putList(std::map<std::string, std::string>& out, const char* name, const std::set<std::string>& items)
    {
      if (!items.empty()) {
        out[name] = joined(items);
      }
      return;
    }

  }  // namespace

  TrcStateCodec::TrcStateCodec(const HexRules::RuleSet& rules) : rules_(rules), axis_(rules.side("axis"))
  {
  }

  void
  TrcStateCodec::decodeAxisFlag(TrcState& state, const SideFlag& flag) const
  {
    if ("weather" == flag.name) {
      try {
        state.weather = weatherNamed(flag.value);
      } catch (const std::invalid_argument&) {
        refuse(flag, "is not a weather state");
      }
    } else if ("weather-drm" == flag.name) {
      state.weatherDrm = countOf(flag);
    } else if ("surrendered" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        try {
          state.surrendered.insert(nationNamed(token));
        } catch (const std::invalid_argument&) {
          refuse(flag, "names '" + token + "', which is not a nation");
        }
      }
    } else if ("helsinki-russian" == flag.name) {
      if ("true" != flag.value) {
        refuse(flag, "is written only as 'true'");
      }
      state.helsinkiRussianP = true;
    } else if ("sudden-death" == flag.name) {
      const std::vector<std::string> parts = tokensOf(flag.value);
      if (2 != parts.size()) {
        refuse(flag, "is not '<condition> <side>'");
      }
      state.suddenDeath = SuddenDeathMet{parts[0], rules_.side(parts[1])};
    } else if ("garrison-warsaw" == flag.name) {
      if ("due" != flag.value && "done" != flag.value) {
        refuse(flag, "is not due or done");
      }
      state.garrisonWarsaw = "due" == flag.value ? WarsawGarrison::Due : WarsawGarrison::Done;
    }
    return;
  }

  void
  TrcStateCodec::decodeSideFlag(TrcSideState& side, const SideFlag& flag) const
  {
    if ("rail-moves" == flag.name) {
      side.railMoves = countOf(flag);
    } else if ("rail-touched" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        side.railTouched.insert(HexCoord::HexId{token});
      }
    } else if ("air-used" == flag.name) {
      side.airUsed = countOf(flag);
    } else if ("leader-lost" == flag.name) {
      side.leaderLost = leaderLostOf(flag);
    } else if ("south-entry" == flag.name) {
      side.southEntry = countOf(flag);
    } else if ("sea-used" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        side.seaUsed.insert(seaAreaOf(flag, token));
      }
    } else if ("replaced" == flag.name) {
      for (const std::string& token : tokensOf(flag.value)) {
        side.replaced.insert(token);
      }
    } else if ("replacement-points" == flag.name) {
      side.replacementPoints = countOf(flag);
    } else if ("replaced-armour" == flag.name) {
      side.replacedArmour = countOf(flag);
    } else if ("replaced-guards" == flag.name) {
      side.replacedGuards = countOf(flag);
    } else {
      refuse(flag, "is not a TRC flag");
    }
    return;
  }

  HexModel::Polymorphic<HexModel::GameState>
  TrcStateCodec::decode(const HexModel::SideFlags& flags) const
  {
    const std::set<std::string> axisOnly{"weather", "weather-drm", "surrendered", "helsinki-russian", "sudden-death",
                                         "garrison-warsaw"};
    if (flags.size() != rules_.sides().size()) {
      throw std::invalid_argument("TrcStateCodec: flags for " + std::to_string(flags.size()) + " sides, the rules have " +
                                  std::to_string(rules_.sides().size()));
    }
    TrcState state(flags.size());
    for (std::size_t s = 0; s < flags.size(); ++s) {
      const SideId side{static_cast<std::uint32_t>(s)};
      std::set<std::string> seen;
      for (const SideFlag& flag : flags[s]) {
        if (!seen.insert(flag.name).second) {
          refuse(flag, "is given twice on side '" + rules_.sides()[s].id + "'");
        }
        if (axisOnly.contains(flag.name)) {
          if (axis_ != side) {
            refuse(flag, "belongs to side 'axis', not '" + rules_.sides()[s].id + "'");
          }
          decodeAxisFlag(state, flag);
        } else {
          decodeSideFlag(state.side(side), flag);
        }
      }
    }
    return HexModel::makePolymorphic<HexModel::GameState, TrcState>(std::move(state));
  }

  HexModel::SideFlags
  TrcStateCodec::encode(const HexModel::GameState& held) const
  {
    const TrcState* state = dynamic_cast<const TrcState*>(&held);
    if (nullptr == state) {
      throw std::invalid_argument("TrcStateCodec: the position holds no TRC state");
    }
    HexModel::SideFlags out(state->sideCount());
    for (std::size_t s = 0; s < state->sideCount(); ++s) {
      const SideId side{static_cast<std::uint32_t>(s)};
      const TrcSideState& one = state->side(side);
      std::map<std::string, std::string> named;
      putCount(named, "rail-moves", one.railMoves);
      std::set<std::string> touched;
      for (const HexCoord::HexId& id : one.railTouched) {
        touched.insert(id.text);
      }
      putList(named, "rail-touched", touched);
      putCount(named, "air-used", one.airUsed);
      if (one.leaderLost) {
        named["leader-lost"] = leaderLostName(*one.leaderLost);
      }
      putCount(named, "south-entry", one.southEntry);
      std::set<std::string> seas;
      for (SeaArea area : one.seaUsed) {
        seas.insert(std::string(seaAreaName(area)));
      }
      putList(named, "sea-used", seas);
      putList(named, "replaced", one.replaced);
      putCount(named, "replacement-points", one.replacementPoints);
      putCount(named, "replaced-armour", one.replacedArmour);
      putCount(named, "replaced-guards", one.replacedGuards);
      if (axis_ == side) {
        if (state->weather) {
          named["weather"] = std::string(weatherName(*state->weather));
        }
        putCount(named, "weather-drm", state->weatherDrm);
        std::set<std::string> nations;
        for (Nation nation : state->surrendered) {
          nations.insert(std::string(nationName(nation)));
        }
        putList(named, "surrendered", nations);
        if (state->helsinkiRussianP) {
          named["helsinki-russian"] = "true";
        }
        if (state->suddenDeath) {
          named["sudden-death"] = state->suddenDeath->condition + " " + rules_.sides()[state->suddenDeath->winner.value].id;
        }
        if (state->garrisonWarsaw) {
          named["garrison-warsaw"] = WarsawGarrison::Due == *state->garrisonWarsaw ? "due" : "done";
        }
      }
      for (const auto& [name, value] : named) {
        out[s].push_back(SideFlag{name, value});
      }
    }
    return out;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
