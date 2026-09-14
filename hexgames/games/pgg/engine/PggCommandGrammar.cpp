// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggCommandGrammar.h"

#include <array>
#include <stdexcept>

namespace Pgg {

  namespace {

    // Each PGG verb and the names of its arguments, in order.
    struct GameVerb {
      const char* verb;
      std::vector<const char*> names;
    };
    const std::vector<GameVerb>&
    gameVerbs()
    {
      static const std::vector<GameVerb> verbs{{"overrun", {"units", "target"}},
                                               {"interdict", {"target"}},
                                               {"cut-rail", {"target"}},
                                               {"lift-cut", {"target"}},
                                               {"reinforce", {"type", "where", "source"}}};
      return verbs;
    }

    const GameVerb*
    findVerb(const std::string& verb)
    {
      for (const GameVerb& known : gameVerbs()) {
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
      throw std::invalid_argument("PggCommandGrammar: '" + verb + "' needs a '" + name + "' argument");
    }

  }  // namespace

  PggCommandGrammar::PggCommandGrammar(const PggFacts& facts, const HexEngine::GameNames& names)
    : facts_(facts), common_(names)
  {
  }

  std::string
  PggCommandGrammar::verb(const HexEngine::Command& command) const
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
      throw std::invalid_argument("PggCommandGrammar: PGG has no command '" + game->verb + "'");
    }
    return common_.verb(command);
  }

  HexEngine::Command
  PggCommandGrammar::parse(const std::string& verb, const std::vector<std::pair<std::string, std::string>>& args) const
  {
    if ("rail-move" == verb) {
      HexEngine::MoveUnit move = std::get<HexEngine::MoveUnit>(common_.parse("move", args));
      move.mode = facts_.railMode();
      return move;
    }
    if (const GameVerb* known = findVerb(verb)) {
      HexEngine::GameCommand game{verb, {}};
      for (const char* name : known->names) {
        game.args.push_back(required(args, name, verb));
      }
      return game;
    }
    if (0 == verb.rfind("game:", 0)) {
      throw std::invalid_argument("PggCommandGrammar: PGG has no command '" + verb + "'");
    }
    return common_.parse(verb, args);
  }

  std::vector<std::string>
  PggCommandGrammar::verbs() const
  {
    std::vector<std::string> out = common_.verbs();
    out.push_back("rail-move");
    for (const GameVerb& known : gameVerbs()) {
      out.push_back(known.verb);
    }
    return out;
  }

  std::vector<std::pair<std::string, std::string>>
  PggCommandGrammar::arguments(const HexEngine::Command& command) const
  {
    if (const HexEngine::GameCommand* game = std::get_if<HexEngine::GameCommand>(&command)) {
      const GameVerb* known = findVerb(game->verb);
      if (nullptr == known || known->names.size() != game->args.size()) {
        throw std::invalid_argument("PggCommandGrammar: cannot write command '" + game->verb + "'");
      }
      std::vector<std::pair<std::string, std::string>> out;
      for (std::size_t i = 0; i < game->args.size(); ++i) {
        out.emplace_back(known->names[i], game->args[i]);
      }
      return out;
    }
    return common_.arguments(command);
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
