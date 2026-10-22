// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The codecs of a game with no module: no side flags and no game obligations in any document.
// ----------------------------------------------
#include "hexengine/Defaults.h"

#include <stdexcept>
#include <string>

namespace HexEngine {

  NoGameStateCodec::NoGameStateCodec(const HexRules::RuleSet& rules) : rules_(rules)
  {
  }

  HexModel::Polymorphic<HexModel::GameState>
  NoGameStateCodec::decode(const HexModel::SideFlags& flags) const
  {
    std::string named;
    for (std::size_t s = 0; s < flags.size(); ++s) {
      for (const HexModel::SideFlag& flag : flags[s]) {
        const std::string side = s < rules_.sides().size() ? rules_.sides()[s].id : std::to_string(s);
        named += (named.empty() ? "" : ", ") + ("flag '" + flag.name + "' on side '" + side + "'");
      }
    }
    if (!named.empty()) {
      throw std::invalid_argument("NoGameStateCodec: no game module supplies a GameStateCodec, so a document may "
                                  "carry no side flags; found " + named);
    }
    return HexModel::Polymorphic<HexModel::GameState>();
  }

  HexModel::SideFlags
  NoGameStateCodec::encode(const HexModel::GameState&) const
  {
    throw std::invalid_argument("NoGameStateCodec: the position holds a game state, and no game module supplies "
                                "a codec to write it");
  }

  HexModel::Polymorphic<HexModel::GameObligation>
  NoObligationCodec::decode(const std::string& name, const std::vector<HexModel::ObligationArg>&) const
  {
    throw std::invalid_argument("NoObligationCodec: no game module supplies an ObligationCodec, so game obligation '" +
                                name + "' cannot be read");
  }

  std::vector<HexModel::ObligationArg>
  NoObligationCodec::encode(const HexModel::GameObligation& owed) const
  {
    throw std::invalid_argument("NoObligationCodec: no game module supplies an ObligationCodec, so game obligation '" +
                                std::string(owed.kind()) + "' cannot be written");
  }

}  // namespace HexEngine
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
