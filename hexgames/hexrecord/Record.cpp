// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Session-dependent glue: converts between the document-level SaveModel (read, written and
// compared entirely above) and a Record built around a live HexEngine::Session. Every id that
// crosses between the two is resolved through the game definition's Board, Roster and RuleSet.
// ----------------------------------------------
#include "hexrecord/GoldenCompare.h"
#include "hexrecord/Record.h"
#include "hexrecord/RecordResolution.h"
#include "hexrecord/SaveModel.h"

#include "hexengine/Defaults.h"
#include "hexengine/Event.h"
#include "hexengine/GameNames.h"
#include "hexmodel/PositionBuilder.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/SaveDoc.h"
#include "hexxml/XmlDocument.h"

#include <algorithm>
#include <array>
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

    std::vector<std::string>
    splitTokens(const std::string& text)
    {
      std::vector<std::string> tokens;
      std::string current;
      for (char c : text) {
        if (' ' == c || '\t' == c || '\n' == c) {
          if (!current.empty()) {
            tokens.push_back(current);
            current.clear();
          }
        } else {
          current += c;
        }
      }
      if (!current.empty()) {
        tokens.push_back(current);
      }
      return tokens;
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

    // The inverse: a command's arguments laid back into the move's own attributes where hexsave has
    // one, and into <arg> children where it has not.
    void
    layArgs(SaveMove& move, const std::vector<std::pair<std::string, std::string>>& args)
    {
      for (const auto& [name, value] : args) {
        if ("units" == name) {
          move.units = splitTokens(value);
        } else if ("path" == name) {
          move.path = splitTokens(value);
        } else if ("modifiers" == name) {
          move.modifiers = splitTokens(value);
        } else if ("from" == name) {
          move.from = value;
        } else if ("to" == name) {
          move.to = value;
        } else if ("target" == name) {
          move.target = value;
        } else if ("mode" == name) {
          move.mode = value;
        } else if ("card" == name) {
          move.card = value;
        } else if ("value" == name) {
          move.value = value;
        } else if ("choice" == name) {
          move.choice = value;
        } else {
          move.args.push_back(SaveArg{name, value});
        }
      }
      return;
    }

    // The document model as hexmodel's own PositionBuilder wants it. The two structures mirror the
    // same schema, so this is a field-for-field copy of the sections a position is built from.
    HexXml::SaveDoc
    asSaveDoc(const SaveModel& model)
    {
      HexXml::SaveDoc doc;
      doc.format = "hexsave-1.0";
      doc.kind = model.kind;
      doc.game = model.game;
      doc.package = model.package;
      doc.scenario = model.scenario;
      doc.seed = model.seed;
      doc.cursor.turn = model.cursor.turn;
      doc.cursor.phase = model.cursor.phase;
      doc.cursor.side = model.cursor.side;
      doc.cursor.moves = static_cast<int>(model.cursor.moves);
      doc.cursor.over = model.cursor.overP;
      doc.cursor.winner = model.cursor.winner;
      for (const SaveSide& side : model.sides) {
        HexXml::SaveSideDoc out;
        out.id = side.id;
        for (const SaveRegister& reg : side.registers) {
          out.registers.push_back(HexXml::SaveRegisterDoc{reg.track, reg.value});
        }
        for (const SaveFlag& flag : side.flags) {
          out.flags.push_back(HexXml::SaveFlagDoc{flag.name, flag.value});
        }
        doc.sides.push_back(std::move(out));
      }
      for (const SaveUnit& unit : model.units) {
        HexXml::SaveUnitDoc out;
        out.id = unit.id;
        out.counter = unit.counter;
        out.type = unit.type;
        out.owner = unit.owner;
        out.hex = unit.hex;
        out.space = unit.space;
        out.face = unit.face;
        out.steps = unit.steps;
        out.status = unit.status;
        out.attached = unit.attached;
        out.revealed = unit.revealedP;
        out.moved = unit.movedP;
        out.delay = unit.delay;
        out.text = unit.text;
        doc.units.push_back(std::move(out));
      }
      for (const SaveControlHex& hex : model.controlHexes) {
        doc.controlHexes.push_back(HexXml::SaveControlHexDoc{hex.id, hex.side});
      }
      for (const SaveControlLink& link : model.controlLinks) {
        doc.controlLinks.push_back(HexXml::SaveControlLinkDoc{link.network, link.hexes, link.side});
      }
      for (const SaveRegion& region : model.regions) {
        doc.regions.push_back(HexXml::SaveRegionDoc{region.layer, region.id, region.status,
                                                     region.alignment, region.posture, region.owner});
      }
      for (const SaveOwe& owe : model.resolution) {
        doc.resolution.push_back(Detail::asOweDoc(owe));
      }
      return doc;
    }

    void
    requireCodecs(const HexEngine::Policies& policies, const char* who)
    {
      if (nullptr == policies.grammar || nullptr == policies.state || nullptr == policies.obligationCodec) {
        throw std::invalid_argument(std::string(who) +
                                    ": the policy set needs a CommandGrammar, a GameStateCodec and an ObligationCodec");
      }
      return;
    }

    // A script carries commands but no position: its @scenario names one in the package manifest.
    SaveModel
    positionSource(const SaveModel& model, const HexRules::GameDefinition& definition,
                    const std::filesystem::path& file)
    {
      if (!model.units.empty() || !model.scenario.has_value()) {
        return model;
      }
      const HexXml::PackageDoc package =
          HexXml::PackageDoc::parse(HexXml::XmlDocument::load(definition.packagePath));
      for (const HexXml::PackageScenarioDoc& scenario : package.scenarios) {
        if (scenario.id == *model.scenario) {
          return SaveModel::read(definition.packagePath.parent_path() / scenario.path);
        }
      }
      throw std::invalid_argument(file.string() + ": save/@scenario names '" + *model.scenario +
                                   "', which package '" + package.id + "' does not declare");
    }

    std::string
    counterOf(const std::string& unitRef)
    {
      const std::size_t hash = unitRef.find('#');
      return std::string::npos == hash ? unitRef : unitRef.substr(0, hash);
    }

    SaveEvent
    asSaveEvent(const HexEngine::Event& event, const HexEngine::GameNames& names)
    {
      const HexEngine::EventFields fields = HexEngine::TextEventEncoder::fields(event, names);
      SaveEvent out;
      out.kind = fields.kind;
      out.unit = fields.unit;
      out.hex = fields.hex;
      out.side = fields.side;
      out.value = fields.value;
      out.text = fields.text;
      return out;
    }

    using DrawCounts = std::array<std::uint64_t, HexEngine::kStreamCount>;

    DrawCounts
    drawsOf(const HexEngine::Session& session)
    {
      DrawCounts counts{};
      for (int i = 0; i < HexEngine::kStreamCount; ++i) {
        counts[static_cast<std::size_t>(i)] =
            session.streams().draws(static_cast<HexEngine::StreamTag>(i));
      }
      return counts;
    }

    // What the engine recorded while applying one command: the combat outcome if there was one, the
    // die draws in order, and every event.
    void
    recordApplied(ScriptedMove& move, const HexEngine::Session& session, const HexEngine::Applied& applied,
                   const DrawCounts& before, const HexEngine::GameNames& names)
    {
      DrawCounts counted = before;
      const std::vector<HexEngine::Event>& events = session.events().events();
      for (std::size_t i = 0; i < applied.eventCount; ++i) {
        const HexEngine::Event& event = events[applied.firstEvent + i];
        if (const HexEngine::CombatResolved* combat = std::get_if<HexEngine::CombatResolved>(&event)) {
          move.outcome = combat->outcome;
        }
        if (const HexEngine::DieRolled* die = std::get_if<HexEngine::DieRolled>(&event)) {
          const std::size_t slot = static_cast<std::size_t>(die->stream);
          SaveDraw draw;
          draw.stream = std::string(HexEngine::streamName(die->stream));
          draw.n = static_cast<int>(++counted[slot]);
          draw.value = std::to_string(die->value);
          move.draws.push_back(renderDraw(draw));
        }
        move.events.push_back(asSaveEvent(event, names));
      }
      return;
    }

  }  // namespace

  Record
  readRecord(const std::filesystem::path& path, const HexRules::GameDefinition& definition,
             const HexEngine::Policies& policies)
  {
    requireCodecs(policies, "readRecord");
    const HexEngine::CommandGrammar& grammar = *policies.grammar;
    const SaveModel doc = SaveModel::read(path);

    Record record;
    record.kind = kindFromName(doc.kind, path);
    record.game = doc.game;
    record.package = doc.package;
    record.scenario = doc.scenario;
    record.seed = doc.seed;
    record.title = doc.title.value_or(std::string());

    const SaveModel source = positionSource(doc, definition, path);
    record.position = HexModel::PositionBuilder::build(asSaveDoc(source), *definition.board,
                                                        *definition.roster, *definition.rules, *policies.state,
                                                        *policies.obligationCodec);

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
      scripted.events = move.events;
      record.log.push_back(std::move(scripted));
    }
    return record;
  }

  void
  writeRecord(const std::filesystem::path& path, Kind kind, const HexEngine::Session& session,
              const std::vector<ScriptedMove>& log, const HexEngine::CommandGrammar& grammar)
  {
    const HexRules::GameDefinition& definition = session.definition();
    const HexEngine::GameNames names(*definition.board, *definition.roster, *definition.rules);
    const HexModel::Position& position = session.position();

    SaveModel doc;
    doc.kind = kindName(kind);
    doc.game = definition.rules->gameId();
    // Canonical on both sides, so that the same pair of files gives the same relative path whatever
    // directory the writer was run from.
    // absolute() first: a bare file name has an empty parent, and relative() to "" is "" (M6 fix).
    doc.package = std::filesystem::relative(std::filesystem::weakly_canonical(definition.packagePath),
                                            std::filesystem::weakly_canonical(std::filesystem::absolute(path).parent_path()))
                       .generic_string();
    doc.seed = session.streams().seed();

    const HexModel::TurnClock& clock = position.clock();
    doc.cursor.turn = clock.turn;
    doc.cursor.phase = names.phase(clock.phase);
    doc.cursor.side = clock.actingSide ? std::optional<std::string>(names.side(*clock.actingSide))
                                        : std::nullopt;
    doc.cursor.moves = log.size();
    doc.cursor.overP = session.prompt().overP;

    // Track registers, each written under the side the rules give its space, or under the first
    // side when the space belongs to neither: hexsave keeps a register inside a <side> element, and
    // a position that is read back has to find every track again.
    {
      std::vector<SaveSide> sides;
      for (const HexRules::Side& side : definition.rules->sides()) {
        sides.push_back(SaveSide{side.id, {}, {}});
      }
      for (std::size_t s = 0; s < definition.board->spaceCount(); ++s) {
        const HexModel::SpaceId space{static_cast<std::uint32_t>(s)};
        const std::optional<HexModel::TrackId> track = definition.board->trackOf(space);
        if (!track) {
          continue;
        }
        const HexModel::SideMask owners = definition.board->space(space).sides;
        std::size_t owner = 0;
        for (std::size_t i = 0; i < owners.size(); ++i) {
          if (owners.test(i)) {
            owner = i;
            break;
          }
        }
        if (sides.size() <= owner) {
          continue;
        }
        sides[owner].registers.push_back(
            SaveRegister{names.space(space), std::to_string(position.track(*track))});
      }
      requireCodecs(session.policies(), "writeRecord");
      if (position.heldGameState().holdsP()) {
        const HexModel::SideFlags flags = session.policies().state->encode(position.heldGameState().base());
        if (flags.size() != sides.size()) {
          throw std::invalid_argument("writeRecord: the game state codec wrote " + std::to_string(flags.size()) +
                                      " sides of flags for " + std::to_string(sides.size()) + " rules sides");
        }
        for (std::size_t i = 0; i < sides.size(); ++i) {
          for (const HexModel::SideFlag& flag : flags[i]) {
            sides[i].flags.push_back(SaveFlag{flag.name, flag.value});
          }
        }
      }
      for (SaveSide& side : sides) {
        if (!side.registers.empty() || !side.flags.empty()) {
          doc.sides.push_back(std::move(side));
        }
      }
    }

    for (const HexModel::UnitSpec& spec : definition.roster->units()) {
      const HexModel::UnitState& state = position.unit(spec.id);
      if (!state.where) {
        continue;
      }
      SaveUnit unit;
      unit.id = spec.counter.text;
      unit.counter = counterOf(spec.counter.text);
      unit.type = definition.rules->unitTypes()[spec.type.value].id;
      unit.owner = names.side(spec.side);
      if (std::holds_alternative<HexModel::HexIndex>(*state.where)) {
        unit.hex = names.hex(std::get<HexModel::HexIndex>(*state.where));
      } else {
        unit.space = names.space(std::get<HexModel::SpaceId>(*state.where));
      }
      unit.face = HexModel::Face::Back == state.face ? "back" : "front";
      if (state.steps.current() != state.steps.maximum()) {
        unit.steps = state.steps.current();
      }
      unit.status = state.markers;
      if (!state.flags.revealedP) {
        unit.revealedP = false;
      }
      unit.movedP = state.flags.movedP;
      unit.delay = state.delay;
      doc.units.push_back(std::move(unit));
    }

    for (std::size_t h = 0; h < definition.board->hexCount(); ++h) {
      const HexModel::HexIndex hex{static_cast<std::uint32_t>(h)};
      if (const std::optional<HexModel::SideId> owner = position.control(hex)) {
        doc.controlHexes.push_back(SaveControlHex{names.hex(hex), names.side(*owner)});
      }
    }

    for (std::size_t n = 0; n < definition.board->networkCount(); ++n) {
      const HexModel::NetworkId network{static_cast<std::uint32_t>(n)};
      const HexModel::LinkNetwork& links = definition.board->network(network);
      for (std::size_t l = 0; l < links.linkCount(); ++l) {
        const std::optional<HexModel::SideId> owner = position.linkOwner(network, l);
        if (!owner) {
          continue;
        }
        doc.controlLinks.push_back(SaveControlLink{definition.rules->networks()[n].id,
                                                    {names.hex(links.links()[l].a), names.hex(links.links()[l].b)},
                                                    names.side(*owner)});
      }
    }

    doc.resolution = Detail::resolutionOf(position, names, *session.policies().obligationCodec);

    for (int i = 0; i < HexEngine::kStreamCount; ++i) {
      const HexEngine::StreamTag tag = static_cast<HexEngine::StreamTag>(i);
      const std::uint64_t draws = session.streams().draws(tag);
      if (0 != draws) {
        doc.streams.push_back(SaveStream{std::string(HexEngine::streamName(tag)), draws, std::nullopt});
      }
    }

    for (const ScriptedMove& move : log) {
      SaveMove m;
      m.n = move.n;
      m.turn = move.turn;
      m.phase = move.phase;
      m.side = move.side;
      m.cmd = grammar.verb(move.command);
      layArgs(m, grammar.arguments(move.command));
      if (move.outcome.has_value()) {
        SaveResult result;
        result.outcome = *move.outcome;
        m.result = result;
      }
      for (const std::string& drawn : move.draws) {
        // A rendered draw reads back as "<stream>#<n>=<value>"; the golden keeps the parts.
        const std::size_t hash = drawn.find('#');
        const std::size_t equals = drawn.find('=');
        SaveDraw draw;
        if (std::string::npos != hash && std::string::npos != equals) {
          draw.stream = drawn.substr(0, hash);
          draw.n = std::stoi(drawn.substr(hash + 1, equals - hash - 1));
          draw.value = drawn.substr(equals + 1);
        } else {
          draw.value = drawn;
        }
        m.draws.push_back(draw);
      }
      m.events = move.events;
      doc.log.push_back(std::move(m));
    }

    writeCanonical(doc, path);
    return;
  }

  std::vector<ScriptedMove>
  playRecord(HexEngine::Session& session, const Record& record)
  {
    const HexRules::GameDefinition& definition = session.definition();
    const HexEngine::GameNames names(*definition.board, *definition.roster, *definition.rules);

    std::vector<ScriptedMove> played;
    int number = 0;
    for (const ScriptedMove& move : record.log) {
      ScriptedMove out;
      out.n = ++number;
      out.turn = session.prompt().turn;
      out.phase = names.phase(session.prompt().phase);
      out.side = session.prompt().side ? names.side(*session.prompt().side) : std::string();
      out.command = move.command;

      const DrawCounts before = drawsOf(session);
      const HexEngine::Applied applied = session.apply(move.command);
      recordApplied(out, session, applied, before, names);
      played.push_back(std::move(out));
    }
    return played;
  }

  std::optional<Divergence>
  replay(HexEngine::Session& session, const Record& record, bool strictP)
  {
    const std::vector<ScriptedMove> played = playRecord(session, record);
    if (!strictP) {
      return std::nullopt;
    }
    for (std::size_t i = 0; i < record.log.size() && i < played.size(); ++i) {
      const ScriptedMove& expected = record.log[i];
      const ScriptedMove& actual = played[i];
      if (expected.outcome.has_value() && expected.outcome != actual.outcome) {
        return Divergence{expected.n, "outcome", *expected.outcome,
                           actual.outcome.value_or(std::string())};
      }
      for (std::size_t d = 0; d < expected.draws.size(); ++d) {
        const std::string got = d < actual.draws.size() ? actual.draws[d] : std::string();
        if (expected.draws[d] != got) {
          return Divergence{expected.n, "draw", expected.draws[d], got};
        }
      }
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
    const Record record = readRecord(script, *definition, policies);
    HexEngine::Session session = sessionFor(record, definition, policies);
    const std::vector<ScriptedMove> played = playRecord(session, record);

    const std::filesystem::path actualPath = actualPathFor(golden);
    writeRecord(actualPath, Kind::Golden, session, played, grammar);

    const SaveModel actual = SaveModel::read(actualPath);
    const SaveModel expected = SaveModel::read(golden);
    return buildGoldenReport(expected, actual, golden);
  }

}  // namespace HexRecord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
