// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include "BoardWidget.h"
#include "DisplayConstants.h"  // latgui display constants
#include "Game.h"            // Latrunculi::Game, Move
#include "GameMainWindow.h"
#include "PlacementPolicy.h"  // Latrunculi::PlacementPolicy (opening variety)

#include <QColor>
#include <QString>
#include <memory>
#include <random>
#include <unordered_set>
#include <vector>

class QAction;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTextEdit;
class QTimer;

// The Latrunculi main window. Derives guicommon::GameMainWindow to reuse the
// shared NegaMax/MCTS Play orchestration, the threaded SearchController, and the
// menu builders; it adds the Latrunculi-specific board widget, banner, tallies,
// menus and save/load.
class MainWindow : public guicommon::GameMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

    // Set the bundled banner font family (resolved in main_gui after loading the
    // application font). Empty leaves the default UI font with a diagnostic.
    void setBannerFont(const QString& family);

private slots:
    void onNewGame();
    void onMoveRequested(AbsGame::MoveId mv);
    void onPlayNegamaxGo();
    void onPlayMctsGo();
    void onPlayComputerGo();  // enter human-vs-computer mode with the chosen side + time
    void onSuggestNegamaxGo();
    void onSuggestMctsGo();
    void onPickColorA();
    void onPickColorB();
    void onPickBackground();
    void onSeedSelected(QAction* action);  // seed the placement phase to a percentage
    void onSeedTick();                      // place one seeded disc (animation)
    void onSave();
    void onLoad();

private:
    void buildMenuBar();
    // Renders a bundled Qt-resource markdown file (":/doc/...") in a read-only popup
    // dialog titled `title`.
    void showMarkdownResource(const QString& title, const QString& resourcePath);
    void updateControls() override;
    AbsGame::Game* currentGame() override;
    void applyComputedMove(AbsGame::MoveId mv) override;
    bool extraSearchBlock() const override;  // true while the seed animation runs
    // Auto-play opening variety: plays some placements at random instead of searching
    // them, per Latrunculi::PlacementPolicy. Movement plies are always searched.
    bool autoPlayMoveOverride(AbsGame::MoveId& mv) override;
    int  playbackPlyCount() const override;  // length of the move timeline (M)
    void rebuildToPly(int ply) override;     // reconstruct the game at ply (replay)

    void newGame(int rows, int columns, int perSide, Latrunculi::MoveStyle style,
                 double komi);
    // The movement rule and komi currently chosen in the Board menu.
    Latrunculi::MoveStyle selectedMoveStyle() const;
    double selectedKomi() const;
    void stopSeed();                     // halt any running seed animation
    bool seeding() const;                // true while the seed timer is active
    void updateSwatches();               // recolor the side A/B tally squares
    void refreshBoard();                 // board update + controls + tallies
    void afterMoveApplied();             // common tail once a move is applied to game_
    void rebuildMoveList();              // repopulate the clickable move list + bar
    QString notate(int square) const;    // chess-like, matching the board labels
    QString moveDescription(const Latrunculi::Move& m) const;
    QString gameOverSummary() const;     // "Game over. A won by +2.5" (game_ must be over)
    QString describeMoveId(AbsGame::MoveId mv) const;  // for the suggestion text

    // Save / load (MainWindow_save.cpp).
    void saveToFile(const QString& path);
    bool loadFromFile(const QString& path);

    // Banner + board.
    BoardWidget* boardWidget_ = nullptr;
    QLabel*      bannerLabel_ = nullptr;

    // Right panel.
    QLabel*      statusLabel_     = nullptr;
    QLabel*      swatchA_         = nullptr;  // color square for side A's tally
    QLabel*      swatchB_         = nullptr;  // color square for side B's tally
    QLabel*      tallyA_          = nullptr;  // side A disc counts
    QLabel*      tallyB_          = nullptr;  // side B disc counts
    QPushButton* stopBtn_         = nullptr;
    QTextEdit*   suggestedLog_    = nullptr;
    QPushButton* clearSuggestBtn_ = nullptr;
    guicommon::MoveListWidget* moveList_ = nullptr;  // clickable per-ply move log
    guicommon::PlaybackBar*    playback_ = nullptr;  // replay transport bar

    // Board menu controls.
    QSpinBox* rowsSpin_    = nullptr;
    QSpinBox* colsSpin_    = nullptr;
    QSpinBox* perSideSpin_ = nullptr;
    // Movement rule set and komi; both take effect on the next new game, like the size
    // spinboxes, and both are part of the board definition rather than a display option.
    QComboBox* movementCombo_ = nullptr;
    QComboBox* komiCombo_ = nullptr;
    QColor    colorA_ = latgui::kDefaultSideA;
    QColor    colorB_ = latgui::kDefaultSideB;
    QColor    background_ = latgui::kDefaultBackground;

    // Play / Suggest menu controls.
    QAction*   manualAction_        = nullptr;
    // NegaMax is time-budgeted (iterative deepening), so these are time combos, not
    // depth spinboxes -- the same choices the MCTS menu offers.
    QComboBox* playNegamaxSecCombo_    = nullptr;
    QComboBox* suggestNegamaxSecCombo_ = nullptr;
    QSpinBox*  playTurnsSpin_       = nullptr;
    QComboBox* playMctsSecCombo_    = nullptr;
    QSpinBox*  playMctsTurnsSpin_   = nullptr;
    QComboBox* suggestMctsSecCombo_ = nullptr;
    // Human-vs-computer controls: NegaMax think time and which side the human plays.
    QComboBox* playComputerSecCombo_  = nullptr;
    QComboBox* playComputerSideCombo_ = nullptr;

    std::unique_ptr<Latrunculi::Game> game_;
    QString currentFilePath_;

    // Replay timeline: the full move list and the board params needed to rebuild
    // any prefix of it (game_ is always the position at the playback cursor).
    std::vector<Latrunculi::Move> timeline_;
    int tlRows_ = 0, tlCols_ = 0, tlPerSide_ = 0;
    // The rule set the timeline was played under. Replaying a slide game under the step
    // rules (or the reverse) would reject legal moves, so this travels with the timeline
    // rather than being re-read from the Board menu.
    Latrunculi::MoveStyle tlStyle_ = Latrunculi::kDefaultMoveStyle;
    double tlKomi_ = Latrunculi::kDefaultKomi;

    // Placement-seeding animation (analogous to IrrGo's stone setup): places a
    // percentage of each side's discs at random no-capture squares, one at a time.
    QTimer*         seedTimer_  = nullptr;
    int             seedPlaced_ = 0;   // discs placed so far this run
    int             seedTarget_ = 0;   // total discs to place this run (both sides)
    std::mt19937_64 seedRng_;
    // Random/searched alternation for auto-played placements. Re-seeded by newGame so
    // each fresh board gets a different opening.
    Latrunculi::PlacementPolicy placementPolicy_{AbsGame::makeSeed(0)};
    // 1-based plies (Move::turn) the policy placed at random, so the move list can tag
    // them. Display provenance only: not part of the game state and not saved.
    std::unordered_set<int> randomPlies_;
};
// Copyright Ben Paul Wise. All Rights Reserved.
