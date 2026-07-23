// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "MancalaWidget.h"
#include "Game.h"
#include "GameMainWindow.h"
#include <QMainWindow>
#include <memory>

class QAction;
class QActionGroup;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTextEdit;
class QProgressBar;
class QTimer;

class MainWindow : public guicommon::GameMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onNewGame();
    void onMoveRequested(int pitIndex);
    void onPlayNegamaxGo();
    void onPlayMctsGo();
    void onPlayComputerGo();  // enter human-vs-computer mode with the chosen side + time
    void onSuggestNegamaxGo();
    void onSuggestMctsGo();

private:
    void buildMenuBar();
    void updateControls() override;
    void logMove(int pitIndex, int player);

    // guicommon::GameMainWindow hooks.
    AbsGame::Game* currentGame() override;
    void applyComputedMove(AbsGame::MoveId mv) override;

    void updateWindowSize();

    // Central UI
    MancalaWidget* boardWidget_;
    QLabel*        statusLabel_;
    QPushButton*   stopBtn_;
    QTextEdit*     moveLog_;
    QTextEdit*     suggestedLog_;
    QPushButton*   clearSuggestBtn_;

    // Play menu controls. NegaMax runs iterative deepening bounded by a wall clock, so it
    // is budgeted by time like MCTS and its control is a time combo, not a depth spinbox.
    QAction*  manualAction_      = nullptr;
    QComboBox* playNegamaxSecCombo_ = nullptr;
    QComboBox* playMctsSecCombo_ = nullptr;
    QSpinBox*  playTurnsSpin_    = nullptr;
    QSpinBox*  playMctsTurnsSpin_ = nullptr;
    // Human-vs-computer controls: NegaMax think time and which side (P0/P1) the human plays.
    QComboBox* playComputerSecCombo_  = nullptr;
    QComboBox* playComputerSideCombo_ = nullptr;

    // Suggest menu controls
    QComboBox* suggestNegamaxSecCombo_ = nullptr;
    QComboBox* suggestMctsSecCombo_  = nullptr;

    // Game state
    std::unique_ptr<Mancala::Game> game_;
    int numPits_      = 6;
    int stonesPerPit_ = 4;
};
// Copyright Ben Paul Wise. All Rights Reserved.
