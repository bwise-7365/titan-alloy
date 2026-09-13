// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Session-dependent glue: converts between the document-level SaveModel (read, written and
// compared entirely above) and a Record built around a live HexEngine::Session. Milestone M4 supplies
// the missing piece throughout: a Board and Roster to resolve hex and counter ids into a Position,
// and a working Session to run one. Until then these are thin conversions of what a SaveModel already
// carries, each gap marked "// M4:".
// ----------------------------------------------
#include "hexrecord/GoldenCompare.h"
#include "hexrecord/Record.h"
#include "hexrecord/SaveModel.h"

#include <stdexcept>

namespace HexRecord {

  namespace {

    std::string
    kindName(Kind kind)
    {
      switch (kind) {
        case Kind::Scenario:
          return "scenario";
        case Kind::Save:
          return "save";
        case Kind::Script:
          return "script";
        case Kind::Golden:
          return "golden";
      }
      throw std::invalid_argument("kindName: unknown HexRecord::Kind");
    }

    Kind
    kindFromName(const std::string& name, const std::filesystem::path& file)
    {
      if ("scenario" == name) {
        return Kind::Scenario;
      }
      if ("save" == name) {
        return Kind::Save;
      }
      if ("script" == name) {
        return Kind::Script;
      }
      if ("golden" == name) {
        return Kind::Golden;
      }
      throw std::invalid_argument(file.string() + ": save: unknown kind '" + name + "'");
    }

    // The name-value pairs a CommandGrammar sees: the move's own fixed attributes, in schema order,
    // then its <arg> children. Token lists are re-joined with a single space, as they read from XML.
    std::vector<std::pair<std::string, std::string>>
    argsOf(const SaveMove& move)
    {
      std::vector<std::pair<std::string, std::string>> args;
      auto add = [&args](const char* name, const std::optional<std::string>& value) {
        if (value.has_value()) {
          args.emplace_back(name, *value);
        }
        return;
      };
      auto addTokens = [&args](const char* name, const std::vector<std::string>& tokens) {
        if (!tokens.empty()) {
          std::string joined;
          for (std::size_t i = 0; i < tokens.size(); ++i) {
            if (0 != i) {
              joined += ' ';
            }
            joined += tokens[i];
          }
          args.emplace_back(name, joined);
        }
        return;
      };
      addTokens("units", move.units);
      add("from", move.from);
      add("to", move.to);
      addTokens("path", move.path);
      add("target", move.target);
      add("mode", move.mode);
      addTokens("modifiers", move.modifiers);
      add("card", move.card);
      add("value", move.value);
      add("choice", move.choice);
      for (const SaveArg& arg : move.args) {
        args.emplace_back(arg.name, arg.value);
      }
      return args;
    }

  }  // namespace

  Record
  readRecord(const std::filesystem::path& path, const HexRules::GameDefinition& definition,
             const HexEngine::CommandGrammar& grammar)
  {
    static_cast<void>(definition);  // M4: resolves hex and counter ids into record.position below.
    const SaveModel doc = SaveModel::read(path);

    Record record;
    record.kind = kindFromName(doc.kind, path);
    record.game = doc.game;
    record.package = doc.package;
    record.scenario = doc.scenario;
    record.seed = doc.seed;
    record.title = doc.title.value_or(std::string());

    // M4: record.position stays default-constructed (no units, no control, no piles) until a
    // PositionBuilder can place doc.units by resolved HexIndex/UnitId, set doc.controlHexes/Links by
    // resolved NetworkId, and load doc.regions/piles/streams -- all of which need this GameDefinition's
    // Board and Roster.

    for (const SaveMove& move : doc.log) {
      ScriptedMove scripted;
      scripted.n = move.n;
      scripted.turn = move.turn;
      scripted.phase = move.phase;
      scripted.side = move.side;
      scripted.command = grammar.parse(move.cmd, argsOf(move));
      if (move.result.has_value()) {
        scripted.outcome = move.result->outcome;
      }
      for (const SaveDraw& draw : move.draws) {
        scripted.draws.push_back(renderDraw(draw));
      }
      record.log.push_back(std::move(scripted));
    }
    return record;
  }

  void
  writeRecord(const std::filesystem::path& path, Kind kind, const HexEngine::Session& session,
              const std::vector<ScriptedMove>& log, const HexEngine::CommandGrammar& grammar)
  {
    static_cast<void>(session);  // M4: position()/events() -> units, control, piles, streams, and each
                                 // move's recorded draw/event children; all need Board + Roster to print
                                 // dense ids back out as the tokens a hexsave document holds.
    SaveModel doc;
    doc.kind = kindName(kind);
    for (const ScriptedMove& move : log) {
      SaveMove m;
      m.n = move.n;
      m.turn = move.turn;
      m.phase = move.phase;
      m.side = move.side;
      m.cmd = grammar.verb(move.command);
      if (move.outcome.has_value()) {
        SaveResult result;
        result.outcome = *move.outcome;
        m.result = result;
      }
      doc.log.push_back(std::move(m));
    }
    writeCanonical(doc, path);
    return;
  }

  std::optional<Divergence>
  replay(HexEngine::Session& session, const Record& record, bool strictP)
  {
    static_cast<void>(strictP);  // M4: only strict mode is meaningful once outcomes can be compared.
    for (const ScriptedMove& move : record.log) {
      session.apply(move.command);
      // M4: compare the Applied events against move.outcome and move.draws (needs an event-to-text
      // rendering over this game's Board and Roster -- HexEngine::TextEventEncoder plus GameNames --
      // which do not exist before M4) and return the first Divergence found.
    }
    return std::nullopt;
  }

  HexEngine::Session
  sessionFor(const Record& record, std::shared_ptr<const HexRules::GameDefinition> definition,
             const HexEngine::Policies& policies)
  {
    return HexEngine::Session(std::move(definition), policies, record.position, record.seed);
  }

  GoldenReport
  compareWithGolden(const std::filesystem::path& script, const std::filesystem::path& golden,
                     std::shared_ptr<const HexRules::GameDefinition> definition, const HexEngine::Policies& policies,
                     const HexEngine::CommandGrammar& grammar)
  {
    const Record record = readRecord(script, *definition, grammar);
    HexEngine::Session session = sessionFor(record, definition, policies);
    replay(session, record, /* strictP = */ true);
    // M4: writeRecord(actualPathFor(golden), Kind::Golden, session, record.log, grammar) needs the
    // Board + Roster conversions noted there; once it can write a real canonical actual file, read
    // both it and `golden` with SaveModel::read and hand them to buildGoldenReport (GoldenCompare.h),
    // which already does the byte compare, first-divergence search and diff.
    GoldenReport report;
    report.actualWritten = actualPathFor(golden);
    return report;
  }

}  // namespace HexRecord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
