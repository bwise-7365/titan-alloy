// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include "BoardWidget.h"
#include "Game.h"            // Latrunculi::Game, Move
#include "GameMainWindow.h"

#include <QColor>
#include <QString>
#include <memory>
#include <random>
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
    void updateControls() override;
    AbsGame::Game* currentGame() override;
    void applyComputedMove(AbsGame::MoveId mv) override;
    bool extraSearchBlock() const override;  // true while the seed animation runs
    int  playbackPlyCount() const override;  // length of the move timeline (M)
    void rebuildToPly(int ply) override;     // reconstruct the game at ply (replay)

    void newGame(int rows, int columns, int perSide);
    void stopSeed();                     // halt any running seed animation
    bool seeding() const;                // true while the seed timer is active
    void updateSwatches();               // recolor the side A/B tally squares
    void refreshBoard();                 // board update + controls + tallies
    void afterMoveApplied();             // common tail once a move is applied to game_
    void rebuildMoveList();              // repopulate the clickable move list + bar
    QString notate(int square) const;    // chess-like, matching the board labels
    QString moveDescription(const Latrunculi::Move& m) const;
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
    QColor    colorA_{0x1F, 0x20, 0x14};
    QColor    colorB_{0x85, 0x25, 0x32};
    QColor    background_{0xF5, 0xE8, 0xC7};

    // Play / Suggest menu controls.
    QAction*   manualAction_        = nullptr;
    QSpinBox*  playDepthSpin_       = nullptr;
    QSpinBox*  playTurnsSpin_       = nullptr;
    QComboBox* playMctsSecCombo_    = nullptr;
    QSpinBox*  playMctsTurnsSpin_   = nullptr;
    QSpinBox*  suggestDepthSpin_    = nullptr;
    QComboBox* suggestMctsSecCombo_ = nullptr;

    std::unique_ptr<Latrunculi::Game> game_;
    QString currentFilePath_;

    // Replay timeline: the full move list and the board params needed to rebuild
    // any prefix of it (game_ is always the position at the playback cursor).
    std::vector<Latrunculi::Move> timeline_;
    int tlRows_ = 0, tlCols_ = 0, tlPerSide_ = 0;

    // Placement-seeding animation (analogous to IrrGo's stone setup): places a
    // percentage of each side's discs at random no-capture squares, one at a time.
    QTimer*         seedTimer_  = nullptr;
    int             seedPlaced_ = 0;   // discs placed so far this run
    int             seedTarget_ = 0;   // total discs to place this run (both sides)
    std::mt19937_64 seedRng_;
};
// Copyright Ben Paul Wise. All Rights Reserved.
