// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// DefaultCommandGrammar: the common verbs as text, so that a script, a save and a golden are plain
// files. Argument names are hexsave move's own attribute names where one fits.
// ----------------------------------------------
#include "hexengine/Defaults.h"

#include <algorithm>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace HexEngine {

  namespace {

    std::vector<std::string>
    splitTokens(const std::string& text)
    {
      std::vector<std::string> tokens;
      std::string current;
      for (char c : text) {
        if (' ' == c || '\t' == c || '\n' == c) {
          if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
          }
        } else {
          current += c;
        }
      }
      if (!current.empty()) {
        tokens.push_back(current);
      }
      return tokens;
    }

    std::string
    joinTokens(const std::vector<std::string>& tokens)
    {
      std::string out;
      for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (0 != i) {
          out += ' ';
        }
        out += tokens[i];
      }
      return out;
    }

    const std::string&
    required(const std::vector<std::pair<std::string, std::string>>& args, const std::string& name,
              const std::string& verb)
    {
      for (const auto& [key, value] : args) {
        if (key == name) {
          return value;
        }
      }
      throw std::invalid_argument("DefaultCommandGrammar: the '" + verb + "' command needs a '" + name +
                                   "' argument");
    }

    std::string
    optional(const std::vector<std::pair<std::string, std::string>>& args, const std::string& name)
    {
      for (const auto& [key, value] : args) {
        if (key == name) {
          return value;
        }
      }
      return std::string();
    }

  }  // namespace

  DefaultCommandGrammar::DefaultCommandGrammar(const GameNames& names) : names_(names)
  {
  }

  std::string
  DefaultCommandGrammar::verb(const Command& command) const
  {
    std::string out;
    std::visit(
        [&](auto&& c) {
          using T = std::decay_t<decltype(c)>;
          if constexpr (std::is_same_v<T, MoveUnit>) {
            out = "move";
          } else if constexpr (std::is_same_v<T, DeclareAttack>) {
            out = "attack";
          } else if constexpr (std::is_same_v<T, ResolveNextAttack>) {
            out = "resolve";
          } else if constexpr (std::is_same_v<T, DecisionAnswer>) {
            out = "answer";
          } else if constexpr (std::is_same_v<T, Place>) {
            out = "place";
          } else if constexpr (std::is_same_v<T, EndPhase>) {
            out = "end-phase";
          } else if constexpr (std::is_same_v<T, GameCommand>) {
            out = "game:" + c.verb;
          }
        },
        command);
    return out;
  }

  std::vector<std::pair<std::string, std::string>>
  DefaultCommandGrammar::arguments(const Command& command) const
  {
    std::vector<std::pair<std::string, std::string>> args;
    std::visit(
        [&](auto&& c) {
          using T = std::decay_t<decltype(c)>;
          if constexpr (std::is_same_v<T, MoveUnit>) {
            std::vector<std::string> units;
            for (UnitId unit : c.units) {
              units.push_back(names_.counter(unit));
            }
            args.emplace_back("units", joinTokens(units));
            args.emplace_back("mode", names_.mode(c.mode));
            std::vector<std::string> path;
            for (HexIndex hex : c.path) {
              path.push_back(names_.hex(hex));
            }
            args.emplace_back("path", joinTokens(path));
          } else if constexpr (std::is_same_v<T, DeclareAttack>) {
            std::vector<std::string> units;
            for (UnitId unit : c.attackers) {
              units.push_back(names_.counter(unit));
            }
            args.emplace_back("units", joinTokens(units));
            args.emplace_back("target", names_.hex(c.target));
            if (!c.modifiers.empty()) {
              std::vector<std::string> modifiers;
              for (ModifierId id : c.modifiers) {
                modifiers.push_back(names_.modifier(id));
              }
              args.emplace_back("modifiers", joinTokens(modifiers));
            }
          } else if constexpr (std::is_same_v<T, DecisionAnswer>) {
            args.emplace_back("what", c.what);
            args.emplace_back("answer", c.answer);
          } else if constexpr (std::is_same_v<T, Place>) {
            args.emplace_back("units", names_.counter(c.unit));
            if (std::holds_alternative<HexIndex>(c.where)) {
              args.emplace_back("where", names_.hex(std::get<HexIndex>(c.where)));
            } else {
              args.emplace_back("where", names_.space(std::get<SpaceId>(c.where)));
            }
          } else if constexpr (std::is_same_v<T, GameCommand>) {
            for (std::size_t i = 0; i < c.args.size(); ++i) {
              args.emplace_back("arg" + std::to_string(i + 1), c.args[i]);
            }
          }
        },
        command);
    return args;
  }

  std::vector<std::string>
  DefaultCommandGrammar::verbs() const
  {
    return {"move", "attack", "resolve", "answer", "place", "end-phase"};
  }

  Command
  DefaultCommandGrammar::parse(const std::string& verb,
                                const std::vector<std::pair<std::string, std::string>>& args) const
  {
    if ("move" == verb) {
      MoveUnit move;
      for (const std::string& token : splitTokens(required(args, "units", verb))) {
        move.units.push_back(names_.unitOf(token));
      }
      const std::string mode = optional(args, "mode");
      move.mode = mode.empty() ? ModeId{0} : names_.modeOf(mode);
      for (const std::string& token : splitTokens(required(args, "path", verb))) {
        move.path.push_back(names_.hexOf(token));
      }
      return move;
    }
    if ("attack" == verb) {
      DeclareAttack attack;
      for (const std::string& token : splitTokens(required(args, "units", verb))) {
        attack.attackers.push_back(names_.unitOf(token));
      }
      attack.target = names_.hexOf(required(args, "target", verb));
      for (const std::string& token : splitTokens(optional(args, "modifiers"))) {
        attack.modifiers.push_back(names_.modifierOf(token));
      }
      return attack;
    }
    if ("resolve" == verb) {
      return ResolveNextAttack{};
    }
    if ("answer" == verb) {
      return DecisionAnswer{required(args, "what", verb), required(args, "answer", verb)};
    }
    if ("place" == verb) {
      Place place;
      place.unit = names_.unitOf(required(args, "units", verb));
      const std::string where = required(args, "where", verb);
      if (const std::optional<HexIndex> hex = names_.board().find(HexCoord::HexId{where})) {
        place.where = *hex;
      } else {
        place.where = names_.spaceOf(where);
      }
      return place;
    }
    if ("end-phase" == verb) {
      return EndPhase{};
    }
    if (0 == verb.rfind("game:", 0)) {
      GameCommand game;
      game.verb = verb.substr(5);
      for (const auto& [name, value] : args) {
        game.args.push_back(value);
      }
      return game;
    }
    throw std::invalid_argument("DefaultCommandGrammar: unknown command verb '" + verb + "'");
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
