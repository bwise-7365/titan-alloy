// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A running hash of everything one game did, to show that two runs of it were the same game. After each
// command it folds in the command's text, the position's digest, every random stream's draw count and
// the text of each event the command produced. Position::digest alone covers neither the streams nor
// the events.
// ----------------------------------------------
#pragma once
#include "hexengine/Command.h"
#include "hexengine/GameNames.h"
#include "hexengine/Session.h"

#include <cstdint>
#include <string_view>

namespace HexAi {

  class Fingerprint {
  public:
    Fingerprint(const HexEngine::GameNames&, const HexEngine::CommandGrammar&);
    void absorb(const HexEngine::Session&, const HexEngine::Command&, const HexEngine::Applied&);
    std::uint64_t value() const { return value_; }

  private:
    void fold(std::string_view);

    const HexEngine::GameNames& names_;
    const HexEngine::CommandGrammar& grammar_;
    std::uint64_t value_ = 14695981039346656037ull;  // FNV-1a's offset basis, as Position::digest
  };

}  // namespace HexAi
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
