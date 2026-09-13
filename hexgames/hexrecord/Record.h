// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Game records (hexsave.xsd): loading a scenario or save into a Session, writing a canonical save,
// replaying a script, and comparing a replay with its golden.
// ----------------------------------------------
#pragma once
#include "hexengine/Command.h"
#include "hexengine/Session.h"
#include "hexrecord/SaveModel.h"
#include "hexrules/Package.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace HexRecord {

  enum class Kind : std::uint8_t { Scenario, Save, Script, Golden };

  struct ScriptedMove {
    int n = 0;
    int turn = 0;
    std::string phase;
    std::string side;
    HexEngine::Command command;
    // Present in saves and goldens: what the engine recorded when it applied the command.
    std::optional<std::string> outcome;
    std::vector<std::string> draws;   // "combat#5=3", "deck=card-17"
    // Added in M4: the events the command emitted, as hexsave <event> children, so that a golden
    // written from a replay carries the same log the engine printed.
    std::vector<SaveEvent> events;
  };

  struct Record {
    Kind kind = Kind::Scenario;
    std::string game;
    std::filesystem::path package;
    std::optional<std::string> scenario;
    std::uint64_t seed = 0;
    std::string title;
    HexModel::Position position;      // the units/control/regions/piles/streams sections
    std::vector<ScriptedMove> log;
  };

  // Reads any hexsave document; every cross-document reference is resolved against the definition
  // and a failure names file:line, the attribute and the id.
  Record readRecord(const std::filesystem::path&, const HexRules::GameDefinition&, const HexEngine::CommandGrammar&);

  // Writes the canonical form: fixed attribute order, units sorted by id, LF line ends, two-space
  // indent, `created` copied from the source when replaying a script. Two equal sessions write
  // byte-identical files.
  void writeRecord(const std::filesystem::path&, Kind, const HexEngine::Session&, const std::vector<ScriptedMove>& log,
                   const HexEngine::CommandGrammar&);

  struct Divergence {
    int moveNumber;
    std::string what;      // "outcome", "draw", "digest"
    std::string expected;
    std::string actual;
  };

  // Added in M4: applies a record's log to a session, returning what the engine recorded for each
  // command -- its outcome, its draws and its events. This is the log writeRecord writes, and the
  // log replay() compares against a golden's own.
  std::vector<ScriptedMove> playRecord(HexEngine::Session&, const Record&);

  // Applies a record's log to a session built from its scenario. In strict mode each recorded
  // outcome and draw is compared as it goes and the first divergence is returned.
  std::optional<Divergence> replay(HexEngine::Session&, const Record&, bool strictP);

  // Builds the session for a record: package -> definition -> scenario position -> seed.
  HexEngine::Session sessionFor(const Record&, std::shared_ptr<const HexRules::GameDefinition>, const HexEngine::Policies&);

  struct GoldenReport {
    bool matchP = false;
    std::optional<Divergence> first;
    std::string unifiedDiff;
    std::filesystem::path actualWritten;
  };

  // The system test: replay `script`, write the canonical save beside `golden` as *.actual.xml, and
  // compare byte for byte; on mismatch, name the first divergent move.
  GoldenReport compareWithGolden(const std::filesystem::path& script, const std::filesystem::path& golden,
                                 std::shared_ptr<const HexRules::GameDefinition>, const HexEngine::Policies&,
                                 const HexEngine::CommandGrammar&);

}  // namespace HexRecord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
