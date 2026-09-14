// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PggBattle <-> hexsave <owe kind="game" name="pgg-battle"><arg name value/>*. Lists are
// space-separated counter or hex ids; an empty list, a false flag and an absent walk or advance are
// written as no argument. Decoding throws naming the argument it cannot read.
// ----------------------------------------------
#include "PggBattle.h"

#include <map>
#include <stdexcept>

namespace Pgg {

  namespace {

    using HexModel::ObligationArg;

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
    joined(const std::vector<std::string>& items)
    {
      std::string out;
      for (const std::string& item : items) {
        out += (out.empty() ? "" : " ") + item;
      }
      return out;
    }

    class Reader {
    public:
      explicit Reader(const std::vector<ObligationArg>& args)
      {
        for (const ObligationArg& arg : args) {
          if (!values_.emplace(arg.name, arg.value).second) {
            throw std::invalid_argument("PggBattleCodec: argument '" + arg.name + "' is given twice");
          }
        }
      }
      const std::string&
      required(const std::string& name) const
      {
        const auto found = values_.find(name);
        if (values_.end() == found) {
          throw std::invalid_argument("PggBattleCodec: pgg-battle needs argument '" + name + "'");
        }
        return found->second;
      }
      std::string
      optional(const std::string& name) const
      {
        const auto found = values_.find(name);
        return values_.end() == found ? std::string() : found->second;
      }
      int
      number(const std::string& name) const
      {
        const std::string text = optional(name);
        if (text.empty()) {
          return 0;
        }
        try {
          std::size_t used = 0;
          const int value = std::stoi(text, &used);
          if (used == text.size()) {
            return value;
          }
        } catch (const std::exception&) {
        }
        throw std::invalid_argument("PggBattleCodec: argument '" + name + "' ('" + text + "') is not a whole number");
      }
      bool
      flag(const std::string& name) const
      {
        const std::string text = optional(name);
        if (!text.empty() && "true" != text) {
          throw std::invalid_argument("PggBattleCodec: argument '" + name + "' is written only as 'true'");
        }
        return "true" == text;
      }

    private:
      std::map<std::string, std::string> values_;
    };

  }  // namespace

  PggBattleCodec::PggBattleCodec(const HexEngine::GameNames& names) : names_(names)
  {
  }

  HexModel::Polymorphic<HexModel::GameObligation>
  PggBattleCodec::decode(const std::string& name, const std::vector<ObligationArg>& args) const
  {
    if ("pgg-battle" != name) {
      throw std::invalid_argument("PggBattleCodec: PGG has no game obligation '" + name + "'");
    }
    const Reader read(args);
    const auto units = [&](const std::string& arg) {
      std::vector<UnitId> out;
      for (const std::string& token : tokensOf(read.optional(arg))) {
        out.push_back(names_.unitOf(token));
      }
      return out;
    };
    const auto hexes = [&](const std::string& arg) {
      std::vector<HexIndex> out;
      for (const std::string& token : tokensOf(read.optional(arg))) {
        out.push_back(names_.hexOf(token));
      }
      return out;
    };
    PggBattle battle;
    battle.attackers = units("attackers");
    battle.defenders = units("defenders");
    battle.target = names_.hexOf(read.required("target"));
    battle.attackerSide = names_.sideOf(read.required("attacker"));
    battle.defenderSide = names_.sideOf(read.required("defender"));
    battle.overrunP = read.flag("overrun");
    battle.code = read.required("code");
    battle.stage = stageNamed(read.required("stage"));
    battle.startedP = read.flag("started");
    battle.choseP = read.flag("chose");
    battle.stepsLeft = read.number("steps-left");
    battle.retreatQueue = units("retreat-queue");
    if (!read.optional("walk-unit").empty()) {
      battle.walk = RetreatWalk{names_.unitOf(read.required("walk-unit")), names_.hexOf(read.required("walk-from")),
                                read.number("walk-left"), hexes("walk-path")};
    }
    battle.retreated = units("retreated");
    battle.pathOfRetreat = hexes("path-of-retreat");
    battle.defenderHitP = read.flag("defender-hit");
    battle.attackerHitP = read.flag("attacker-hit");
    if (!read.optional("advancing").empty()) {
      battle.advancing = names_.unitOf(read.required("advancing"));
    }
    battle.advanced = units("advanced");
    return HexModel::makePolymorphic<HexModel::GameObligation, PggBattle>(std::move(battle));
  }

  std::vector<ObligationArg>
  PggBattleCodec::encode(const HexModel::GameObligation& held) const
  {
    const PggBattle* battle = dynamic_cast<const PggBattle*>(&held);
    if (nullptr == battle) {
      throw std::invalid_argument("PggBattleCodec: game obligation '" + std::string(held.kind()) + "' is not PGG's");
    }
    const auto units = [&](const std::vector<UnitId>& list) {
      std::vector<std::string> out;
      for (UnitId unit : list) {
        out.push_back(names_.counter(unit));
      }
      return joined(out);
    };
    const auto hexes = [&](const std::vector<HexIndex>& list) {
      std::vector<std::string> out;
      for (HexIndex hex : list) {
        out.push_back(names_.hex(hex));
      }
      return joined(out);
    };
    std::vector<ObligationArg> out;
    const auto put = [&out](const char* name, const std::string& value) {
      if (!value.empty()) {
        out.push_back(ObligationArg{name, value});
      }
      return;
    };
    put("attackers", units(battle->attackers));
    put("defenders", units(battle->defenders));
    put("target", names_.hex(battle->target));
    put("attacker", names_.side(battle->attackerSide));
    put("defender", names_.side(battle->defenderSide));
    put("overrun", battle->overrunP ? "true" : "");
    put("code", battle->code);
    put("stage", std::string(stageName(battle->stage)));
    put("started", battle->startedP ? "true" : "");
    put("chose", battle->choseP ? "true" : "");
    put("steps-left", 0 == battle->stepsLeft ? "" : std::to_string(battle->stepsLeft));
    put("retreat-queue", units(battle->retreatQueue));
    if (battle->walk) {
      put("walk-unit", names_.counter(battle->walk->unit));
      put("walk-from", names_.hex(battle->walk->from));
      put("walk-left", std::to_string(battle->walk->hexesLeft));
      put("walk-path", hexes(battle->walk->path));
    }
    put("retreated", units(battle->retreated));
    put("path-of-retreat", hexes(battle->pathOfRetreat));
    put("defender-hit", battle->defenderHitP ? "true" : "");
    put("attacker-hit", battle->attackerHitP ? "true" : "");
    put("advancing", battle->advancing ? names_.counter(*battle->advancing) : "");
    put("advanced", units(battle->advanced));
    return out;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
