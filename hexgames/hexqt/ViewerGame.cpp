// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ViewerGame.h"

#include "PggGame.h"

#include "hexai/Fingerprint.h"
#include "hexengine/Event.h"
#include "hexrecord/Record.h"
#include "hexsearch/Search.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/XmlDocument.h"

#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace HexQt {

  namespace {

    constexpr std::size_t kCacheEvery = 10;  // a drag across the slider replays at most this many moves

    // The value after a "name" word in a line of words; nullopt when the word is absent.
    std::optional<std::string>
    wordAfter(const std::string& line, const std::string& name)
    {
      std::istringstream words(line);
      std::string word;
      while (words >> word) {
        if (name == word) {
          std::string value;
          return words >> value ? std::optional<std::string>(value) : std::nullopt;
        }
      }
      return std::nullopt;
    }

    // "name=value" in the run line.
    std::string
    settingOf(const std::string& line, const std::string& name, const std::filesystem::path& path)
    {
      std::istringstream words(line);
      std::string word;
      while (words >> word) {
        if (0 == word.rfind(name + "=", 0)) {
          return word.substr(name.size() + 1);
        }
      }
      throw std::invalid_argument(path.string() + ": the run line has no " + name + "=");
    }

  }  // namespace

  RunFile
  readRunFile(const std::filesystem::path& path)
  {
    std::ifstream in(path);
    if (!in) {
      throw std::invalid_argument(path.string() + ": cannot be read");
    }
    RunFile run;
    std::string line;
    while (std::getline(in, line)) {
      if (0 == line.rfind("run ", 0)) {
        run.game = settingOf(line, "game", path);
        run.setUp = settingOf(line, "setup", path);
        run.german = settingOf(line, "german", path);
        run.soviet = settingOf(line, "soviet", path);
      } else if (0 == line.rfind("game ", 0) && std::string::npos == line.find("FAULT")) {
        const std::optional<std::string> number = wordAfter(line, "game");
        const std::optional<std::string> seed = wordAfter(line, "seed");
        const std::optional<std::string> fingerprint = wordAfter(line, "fingerprint");
        if (!number || !seed || !fingerprint) {
          throw std::invalid_argument(path.string() + ": a game line has no number, seed or fingerprint: " + line);
        }
        run.lines.push_back(RunLine{std::stoi(*number), std::stoull(*seed), std::stoull(*fingerprint), line});
      }
    }
    if (run.game.empty()) {
      throw std::invalid_argument(path.string() + ": no 'run' line, so this is not a run file");
    }
    return run;
  }

  ViewerGame::~ViewerGame() = default;

  void
  ViewerGame::start(std::shared_ptr<const HexRules::GameDefinition> definition, const HexEngine::Policies& policies,
                    const HexEngine::GameNames& names, HexRecord::Record record)
  {
    definition_ = std::move(definition);
    names_ = &names;
    grammar_ = policies.grammar;
    log_ = record.log;
    const HexXml::PackageDoc package = HexXml::PackageDoc::parse(HexXml::XmlDocument::load(definition_->packagePath));
    if (package.sheets.empty()) {
      throw std::invalid_argument(definition_->packagePath.string() + ": the package names no sheet");
    }
    document_ = std::make_unique<HexMapEd::Document>(HexMapEd::Document::load(definition_->packagePath.parent_path() / package.sheets.front().path));
    frame_ = std::make_unique<HexView::MapFrame>(HexView::MapFrame::of(document_->sheet()));
    HexAi::Fingerprint running(names, *policies.grammar);
    replay_ = std::make_unique<HexView::ReplayController>(
        definition_, policies, record, kCacheEvery,
        [&running](const HexEngine::Session& session, const HexEngine::Command& command, const HexEngine::Applied& applied) {
          running.absorb(session, command, applied);
        });
    fingerprint_ = running.value();
    return;
  }

  std::unique_ptr<ViewerGame>
  ViewerGame::fromScript(const std::filesystem::path& path)
  {
    std::unique_ptr<ViewerGame> game(new ViewerGame);
    std::shared_ptr<const HexRules::GameDefinition> definition = HexGames::definitionOf(path);
    game->choice_ = std::make_unique<HexGames::PolicyChoice>(*definition, false);
    game->pgg_ = game->choice_->pgg();
    game->title_ = path.filename().string();
    HexRecord::Record record = HexRecord::readRecord(path, *definition, game->choice_->policies());
    game->start(std::move(definition), game->choice_->policies(), game->choice_->names(), std::move(record));
    return game;
  }

  std::unique_ptr<ViewerGame>
  ViewerGame::fromRun(const std::filesystem::path& root, const RunFile& run, const RunLine& line)
  {
    if ("pgg" != run.game) {
      throw std::invalid_argument("no agents play the game '" + run.game + "' yet, so a run of it cannot be played again");
    }
    Pgg::PggShared shared = Pgg::loadShared(root);
    Pgg::PggGameTask task;
    task.seed = line.seed;
    task.setUp = "drawn" == run.setUp ? Pgg::SetUp::Drawn : Pgg::SetUp::Fixed;
    task.german = HexAi::tacticNamed(run.german);
    task.soviet = HexAi::tacticNamed(run.soviet);
    const Pgg::PggGameResult result = Pgg::playGame(shared, task);

    std::unique_ptr<ViewerGame> game(new ViewerGame);
    game->ownSet_ = std::move(shared.set);
    game->pgg_ = game->ownSet_.get();
    game->expected_ = line.fingerprint;
    game->title_ = "game " + std::to_string(line.number) + ", " + run.setUp + " set-up, German " + run.german + ", Soviet " + run.soviet;
    HexRecord::Record record;
    record.kind = HexRecord::Kind::Script;
    record.game = run.game;
    record.seed = task.seed;
    record.position = Pgg::startOf(*game->ownSet_, shared.start, task);
    for (const HexAi::AppliedCommand& applied : result.play.commands) {
      HexRecord::ScriptedMove move;
      move.n = static_cast<int>(record.log.size()) + 1;
      move.turn = applied.turn;
      move.phase = game->ownSet_->names().phase(applied.phase);
      move.side = applied.side ? game->ownSet_->names().side(*applied.side) : std::string();
      move.command = applied.command;
      record.log.push_back(std::move(move));
    }
    game->start(std::move(shared.definition), game->ownSet_->policies(), game->ownSet_->names(), std::move(record));
    return game;
  }

  HexView::Scene
  ViewerGame::overlay(std::size_t move, const HexView::PlayLayerOptions& options)
  {
    HexView::Scene scene(frame_->width(), frame_->height());
    HexView::addUnits(scene, *frame_, *definition_->board, *definition_->roster, replay_->at(move), options);
    if (0 < move) {
      HexView::addStepMarks(scene, *frame_, *definition_->board, replay_->eventsOf(move));
    }
    return scene;
  }

  std::string
  ViewerGame::statusAt(std::size_t move)
  {
    const HexModel::Position& position = replay_->at(move);
    const HexModel::TurnClock& clock = position.clock();
    std::string out = "Turn " + std::to_string(clock.turn) + ", " + names_->phase(clock.phase);
    if (clock.actingSide) {
      out += ", " + names_->side(*clock.actingSide);
    }
    if (nullptr != pgg_) {
      const HexEngine::Ctx ctx{*definition_->board, *definition_->rules, *definition_->roster, position};
      HexSearch::SearchScratch scratch;
      out += ". Points: German " + std::to_string(pgg_->victory().germanPoints(ctx, scratch)) + ", Soviet " +
             std::to_string(pgg_->victory().sovietPoints(ctx));
    }
    return out;
  }

  std::string
  ViewerGame::commandOf(std::size_t move) const
  {
    if (0 == move) {
      return std::string();
    }
    const HexEngine::Command& command = log_[move - 1].command;
    std::string out = grammar_->verb(command);
    for (const auto& [name, value] : grammar_->arguments(command)) {
      out += " " + name + "=" + value;
    }
    return out;
  }

  std::vector<std::string>
  ViewerGame::eventsOf(std::size_t move) const
  {
    std::vector<std::string> out;
    if (0 < move) {
      for (const HexEngine::Event& event : replay_->eventsOf(move)) {
        out.push_back(HexEngine::TextEventEncoder::line(event, *names_));
      }
    }
    return out;
  }

  std::size_t
  ViewerGame::moveOfTurn(int turn) const
  {
    for (std::size_t i = 0; i < log_.size(); ++i) {
      if (turn <= log_[i].turn) {
        return i + 1;
      }
    }
    return log_.size();
  }

  int
  ViewerGame::turnOf(std::size_t move) const
  {
    return 0 == move ? (log_.empty() ? 1 : log_.front().turn) : log_[move - 1].turn;
  }

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
