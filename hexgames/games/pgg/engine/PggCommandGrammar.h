// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PGG's command words on top of the engine's: rail-move (a move in the rail-move mode), overrun
// units/target, interdict target, cut-rail target, lift-cut target, and reinforce type/where/source.
// German reinforcements and Soviet Leaders use the engine's own `place`.
// ----------------------------------------------
#pragma once
#include "PggFacts.h"

#include "hexengine/Defaults.h"

namespace Pgg {

  class PggCommandGrammar : public HexEngine::CommandGrammar {
  public:
    PggCommandGrammar(const PggFacts&, const HexEngine::GameNames&);
    std::string verb(const HexEngine::Command&) const override;
    HexEngine::Command parse(const std::string& verb,
                             const std::vector<std::pair<std::string, std::string>>& args) const override;
    std::vector<std::pair<std::string, std::string>> arguments(const HexEngine::Command&) const override;
    std::vector<std::string> verbs() const override;

  private:
    const PggFacts& facts_;
    HexEngine::DefaultCommandGrammar common_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
