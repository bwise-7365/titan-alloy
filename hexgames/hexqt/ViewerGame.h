// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// One game for pgg_trc_viewer to step through, Qt-free. It comes from a hexsave script of any game with a
// module (the reader and the policy choice hexgames_cli uses), or from a line of a run file, which is played
// again by the agents the file names. Either way it becomes a HexRecord::Record and a HexView::ReplayController,
// so the window has one thing to show. For TRC, scripts work now; run files need its agents.
// ----------------------------------------------
#pragma once
#include "GameOfRecord.h"

#include "hexmaped/Document.h"
#include "hexview/MapFrame.h"
#include "hexview/PlayLayers.h"
#include "hexview/Replay.h"
#include "hexview/Scene.h"

#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace HexQt {

  // A run file (doc/pgg-runs): its "run" line and one line for each game that finished.
  struct RunLine {
    int number = 0;
    std::uint64_t seed = 0;
    std::uint64_t fingerprint = 0;
    std::string text;  // the line as written
  };
  struct RunFile {
    std::string game;
    std::string setUp;
    std::string german;
    std::string soviet;
    std::vector<RunLine> lines;
  };
  // Throws std::invalid_argument naming the file and what is wrong with it.
  RunFile readRunFile(const std::filesystem::path&);

  class ViewerGame {
  public:
    static std::unique_ptr<ViewerGame> fromScript(const std::filesystem::path&);
    // root is hexgames' own directory. Throws for a game with no agents yet.
    static std::unique_ptr<ViewerGame> fromRun(const std::filesystem::path& root, const RunFile&, const RunLine&);
    ~ViewerGame();

    const std::string& title() const { return title_; }
    std::size_t moveCount() const { return replay_->moveCount(); }
    std::uint64_t fingerprint() const { return fingerprint_; }
    std::optional<std::uint64_t> expectedFingerprint() const { return expected_; }
    bool fingerprintMismatchP() const { return expected_ && *expected_ != fingerprint_; }
    const HexMapEd::Document& document() const { return *document_; }

    // The drawn state after `move` moves (0: the start), as a scene to lay over the map.
    HexView::Scene overlay(std::size_t move, const HexView::PlayLayerOptions&);
    std::string statusAt(std::size_t move);                  // turn, phase and side, and the score where there is one
    std::string commandOf(std::size_t move) const;           // the command as the hexsave grammar writes it; "" for 0
    std::vector<std::string> eventsOf(std::size_t move) const;
    std::size_t moveOfTurn(int turn) const;                  // the first move of the turn, or the last move there is
    int turnOf(std::size_t move) const;

  private:
    ViewerGame() = default;
    void start(std::shared_ptr<const HexRules::GameDefinition>, const HexEngine::Policies&, const HexEngine::GameNames&,
               HexRecord::Record);

    std::shared_ptr<const HexRules::GameDefinition> definition_;
    std::unique_ptr<HexGames::PolicyChoice> choice_;  // a script's
    std::unique_ptr<Pgg::PggPolicySet> ownSet_;       // a run's
    const Pgg::PggPolicySet* pgg_ = nullptr;
    const HexEngine::GameNames* names_ = nullptr;
    const HexEngine::CommandGrammar* grammar_ = nullptr;
    std::unique_ptr<HexMapEd::Document> document_;
    std::unique_ptr<HexView::MapFrame> frame_;
    std::unique_ptr<HexView::ReplayController> replay_;
    std::vector<HexRecord::ScriptedMove> log_;
    std::string title_;
    std::uint64_t fingerprint_ = 0;
    std::optional<std::uint64_t> expected_;
  };

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
