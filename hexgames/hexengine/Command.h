// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Commands: the parameterised intents a Player submits. The common verbs are typed; a game adds its
// own through GameCommand and its CommandGrammar, which also prints and parses them as text so that
// scripts, saves and goldens are plain files.
// ----------------------------------------------
#pragma once
#include "hexmodel/Ids.h"

#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace HexEngine {

  using namespace HexModel;

  struct MoveUnit { std::vector<UnitId> units; ModeId mode; std::vector<HexIndex> path; };
  struct DeclareAttack { std::vector<UnitId> attackers; HexIndex target; std::vector<ModifierId> modifiers; };
  struct ResolveNextAttack {};
  struct DecisionAnswer { std::string what; std::string answer; };   // matched against Position::pending()
  struct Place { UnitId unit; std::variant<HexIndex, SpaceId> where; };
  struct EndPhase {};
  struct GameCommand { std::string verb; std::vector<std::string> args; };

  using Command = std::variant<MoveUnit, DeclareAttack, ResolveNextAttack, DecisionAnswer, Place, EndPhase, GameCommand>;

  // Text form of commands: hexsave move/@cmd and its attributes.
  class CommandGrammar {
  public:
    virtual ~CommandGrammar() = default;
    virtual std::string verb(const Command&) const = 0;
    // Throws std::invalid_argument naming the verb or argument it could not read.
    virtual Command parse(const std::string& verb, const std::vector<std::pair<std::string, std::string>>& args) const = 0;
    // Added in M4: the inverse of parse, so that a command written into a save or a golden reads
    // back as itself. The names are hexsave move's own attribute names where one fits (units, mode,
    // path, target, modifiers) and an <arg> name otherwise (what, answer, where).
    virtual std::vector<std::pair<std::string, std::string>> arguments(const Command&) const = 0;
    // Added in M6b review: every verb parse() reads, so that a Session can refuse rules steps whose
    // @commands name a verb the game cannot issue.
    virtual std::vector<std::string> verbs() const = 0;
  };

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
