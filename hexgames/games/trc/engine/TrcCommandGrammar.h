// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC's command words on top of the engine's: rail-move (a move in the rail-move mode), sea-move
// units/to, paradrop units/to and av-attack units/target. Reinforcements, the Off-Map Units Box,
// replacements and partisans all use the engine's own `place`; rail conversion happens by rule in
// the end phases and has no command.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"

#include "hexengine/Defaults.h"

namespace Trc {

  class TrcCommandGrammar : public HexEngine::CommandGrammar {
  public:
    TrcCommandGrammar(const TrcFacts&, const HexEngine::GameNames&);
    std::string verb(const HexEngine::Command&) const override;
    HexEngine::Command parse(const std::string& verb,
                             const std::vector<std::pair<std::string, std::string>>& args) const override;
    std::vector<std::pair<std::string, std::string>> arguments(const HexEngine::Command&) const override;

  private:
    const TrcFacts& facts_;
    HexEngine::DefaultCommandGrammar common_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
