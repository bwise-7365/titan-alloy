// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "Fingerprint.h"

#include "hexengine/Event.h"

#include <string>

namespace HexAi {

  Fingerprint::Fingerprint(const HexEngine::GameNames& names, const HexEngine::CommandGrammar& grammar)
    : names_(names), grammar_(grammar)
  {
  }

  void
  Fingerprint::fold(std::string_view text)
  {
    for (const char c : text) {
      value_ ^= static_cast<unsigned char>(c);
      value_ *= 1099511628211ull;
    }
    value_ ^= 0xff;  // ends the text, so that "ab" + "c" and "a" + "bc" differ
    value_ *= 1099511628211ull;
    return;
  }

  void
  Fingerprint::absorb(const HexEngine::Session& session, const HexEngine::Command& command, const HexEngine::Applied& applied)
  {
    fold(grammar_.verb(command));
    for (const auto& [name, value] : grammar_.arguments(command)) {
      fold(name);
      fold(value);
    }
    fold(std::to_string(session.position().digest()));
    for (int tag = 0; tag < HexEngine::kStreamCount; ++tag) {
      fold(std::to_string(session.streams().draws(static_cast<HexEngine::StreamTag>(tag))));
    }
    const std::vector<HexEngine::Event>& events = session.events().events();
    for (std::size_t i = applied.firstEvent; i < applied.firstEvent + applied.eventCount; ++i) {
      fold(HexEngine::TextEventEncoder::line(events[i], names_));
    }
    return;
  }

}  // namespace HexAi
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
