// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcCommandGrammar.h"

#include <stdexcept>

namespace Trc {

  namespace {

    // Each TRC verb and the names of its two arguments, in order.
    struct GameVerb {
      const char* verb;
      const char* first;
      const char* second;
    };
    constexpr std::array<GameVerb, 3> kVerbs{{{"sea-move", "units", "to"},
                                              {"paradrop", "units", "to"},
                                              {"av-attack", "units", "target"}}};

    const GameVerb*
    findVerb(const std::string& verb)
    {
      for (const GameVerb& known : kVerbs) {
        if (verb == known.verb) {
          return &known;
        }
      }
      return nullptr;
    }

    const std::string&
    required(const std::vector<std::pair<std::string, std::string>>& args, const char* name, const std::string& verb)
    {
      for (const auto& [key, value] : args) {
        if (name == key) {
          return value;
        }
      }
      throw std::invalid_argument("TrcCommandGrammar: '" + verb + "' needs a '" + name + "' argument");
    }

  }  // namespace

  TrcCommandGrammar::TrcCommandGrammar(const TrcFacts& facts, const HexEngine::GameNames& names)
    : facts_(facts), common_(names)
  {
  }

  std::string
  TrcCommandGrammar::verb(const HexEngine::Command& command) const
  {
    if (const HexEngine::MoveUnit* move = std::get_if<HexEngine::MoveUnit>(&command)) {
      if (facts_.railMode() == move->mode) {
        return "rail-move";
      }
    }
    if (const HexEngine::GameCommand* game = std::get_if<HexEngine::GameCommand>(&command)) {
      if (nullptr != findVerb(game->verb)) {
        return game->verb;
      }
      throw std::invalid_argument("TrcCommandGrammar: TRC has no command '" + game->verb + "'");
    }
    return common_.verb(command);
  }

  HexEngine::Command
  TrcCommandGrammar::parse(const std::string& verb, const std::vector<std::pair<std::string, std::string>>& args) const
  {
    if ("rail-move" == verb) {
      HexEngine::MoveUnit move = std::get<HexEngine::MoveUnit>(common_.parse("move", args));
      move.mode = facts_.railMode();
      return move;
    }
    if (const GameVerb* known = findVerb(verb)) {
      return HexEngine::GameCommand{verb, {required(args, known->first, verb), required(args, known->second, verb)}};
    }
    if (0 == verb.rfind("game:", 0)) {
      throw std::invalid_argument("TrcCommandGrammar: TRC has no command '" + verb + "'");
    }
    return common_.parse(verb, args);
  }

  std::vector<std::string>
  TrcCommandGrammar::verbs() const
  {
    std::vector<std::string> out = common_.verbs();
    out.push_back("rail-move");
    for (const GameVerb& known : kVerbs) {
      out.push_back(known.verb);
    }
    return out;
  }

  std::vector<std::pair<std::string, std::string>>
  TrcCommandGrammar::arguments(const HexEngine::Command& command) const
  {
    if (const HexEngine::GameCommand* game = std::get_if<HexEngine::GameCommand>(&command)) {
      const GameVerb* known = findVerb(game->verb);
      if (nullptr == known || 2 != game->args.size()) {
        throw std::invalid_argument("TrcCommandGrammar: cannot write command '" + game->verb + "'");
      }
      return {{known->first, game->args[0]}, {known->second, game->args[1]}};
    }
    return common_.arguments(command);
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
