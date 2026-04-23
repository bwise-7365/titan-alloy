// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "BoardWidget.h"
#include "Game.h"
#include "Graph.h"
#include <QMainWindow>
#include <memory>
#include <random>
#include <vector>

class QAction;
class QActionGroup;
class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QTextEdit;
class QTimer;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void generateBoard();
    void onBgColorChanged(int index);
    void onStonesSelected(QAction* action);
    void onSetupTick();
    void onMoveRequested(int nodeId);
    void onBlackPass();
    void onWhitePass();
    void onSuggestGo();

private:
    void buildMenuBar();
    void updateControls();
    void logLastMove();
    void stopStoneSetup();
    void clearSuggestion();

    // Builds a NegaMax submenu action and attaches it to parent / group.
    // depthOut / turnsOut are set to the created spinboxes.
    // connectGo: if true the Go! button is wired to onSuggestGo().
    void buildNegaMaxMenu(QMenu* parent, QActionGroup* group,
                          QSpinBox*& depthOut, QSpinBox*& turnsOut,
                          bool connectGo, bool withTurns = true);

    // Central UI
    BoardWidget*  boardWidget_;
    QLabel*       currentPlayerLabel_;
    QPushButton*  blackPassBtn_;
    QPushButton*  whitePassBtn_;
    QTextEdit*    suggestedLog_;
    QPushButton*  clearSuggestBtn_;
    QTextEdit*    moveLog_;

    // Board menu embedded controls
    QComboBox*    sizeCombo_;
    QCheckBox*    irregularCheck_;
    QComboBox*    maxEdgesCombo_;
    QComboBox*    bgCombo_;

    // Random menu
    QLineEdit*    randomSeedEdit_;

    // Stones menu
    QActionGroup* stonesGroup_;

    // Play menu
    QAction*  manualAction_     = nullptr;
    QSpinBox* playDepthSpin_    = nullptr;
    QSpinBox* playTurnsSpin_    = nullptr;

    // Suggest menu
    QSpinBox* suggestDepthSpin_ = nullptr;
    QSpinBox* suggestTurnsSpin_ = nullptr;

    // Game state
    std::unique_ptr<IrrGo::Graph>  graph_;
    std::unique_ptr<IrrGo::Game>   game_;

    // Setup animation
    QTimer*          stoneTimer_;
    std::vector<int> pendingSetup_;
    int              setupIdx_    = 0;
    int              setupPlaced_ = 0;
    int              setupTarget_ = 0;
    std::mt19937_64  setupRng_;
};
// Copyright Ben Paul Wise. All Rights Reserved.
