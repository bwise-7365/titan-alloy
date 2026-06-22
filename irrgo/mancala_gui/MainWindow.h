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

    // Play menu controls
    QAction*  manualAction_      = nullptr;
    QSpinBox* playDepthSpin_     = nullptr;
    QComboBox* playMctsSecCombo_ = nullptr;
    QSpinBox*  playTurnsSpin_    = nullptr;
    QSpinBox*  playMctsTurnsSpin_ = nullptr;

    // Suggest menu controls
    QSpinBox*  suggestDepthSpin_     = nullptr;
    QComboBox* suggestMctsSecCombo_  = nullptr;

    // Game state
    std::unique_ptr<Mancala::Game> game_;
    int numPits_      = 6;
    int stonesPerPit_ = 4;
};
// Copyright Ben Paul Wise. All Rights Reserved.
