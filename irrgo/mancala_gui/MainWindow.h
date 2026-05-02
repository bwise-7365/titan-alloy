// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "MancalaWidget.h"
#include "Game.h"
#include "Searcher.h"
#include <QElapsedTimer>
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

class MainWindow : public QMainWindow {
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
    void updateControls();
    void logMove(int pitIndex, int player);
    void applyComputedMove(AbsGame::MoveId mv);

    void startSearchIndicator(int budgetSeconds = 0);
    void stopSearchIndicator();
    void cancelSearch();

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

    // Search progress
    QProgressBar*  searchProgress_ = nullptr;
    QTimer*        searchBarTimer_ = nullptr;
    QElapsedTimer  searchElapsed_;
    int            searchBudgetMs_ = 0;
    bool           isSearching_    = false;
    unsigned       searchGen_      = 0;
    int            playTurnsRemaining_ = 0;
    int            playTurnsTotal_     = 0;
    QProgressBar*  turnProgress_   = nullptr;

    // Game state
    std::unique_ptr<Mancala::Game> game_;
    int numPits_      = 6;
    int stonesPerPit_ = 4;
};
// Copyright Ben Paul Wise. All Rights Reserved.
