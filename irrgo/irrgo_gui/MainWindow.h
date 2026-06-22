// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "BoardWidget.h"
#include "Game.h"
#include "Graph.h"
#include "GameMainWindow.h"
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
class QProgressBar;
class QTimer;

class MainWindow : public guicommon::GameMainWindow {
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
    void onSuggestMctsGo();
    void onPlayNegamaxGo();
    void onPlayMctsGo();
    void onSave();
    void onSaveAs();
    void onLoad();

private:
    void buildMenuBar();
    void updateControls() override;
    void logLastMove();
    void stopStoneSetup();
    void clearSuggestion();

    // guicommon::GameMainWindow hooks.
    AbsGame::Game* currentGame() override;
    void applyComputedMove(AbsGame::MoveId mv) override;
    bool extraSearchBlock() const override;   // true while the stone setup animates

    void saveToFile(const QString& path);

    // Central UI
    BoardWidget*  boardWidget_;
    QLabel*       currentPlayerLabel_;
    QLabel*       hoverCoordLabel_     = nullptr;
    QPushButton*  stopBtn_;
    QPushButton*  blackPassBtn_;
    QPushButton*  whitePassBtn_;
    QTextEdit*    suggestedLog_;
    QPushButton*  clearSuggestBtn_;
    QSpinBox*     dvrRadiusSpin_      = nullptr;
    QCheckBox*    blackDvrCheck_      = nullptr;
    QCheckBox*    whiteDvrCheck_      = nullptr;
    QPushButton*  labelsBtn_           = nullptr;
    QPushButton*  neighborhoodBtn_    = nullptr;
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
    QAction*  manualAction_      = nullptr;
    QSpinBox* playDepthSpin_     = nullptr;
    QSpinBox* playTurnsSpin_     = nullptr;
    QComboBox* playMctsSecCombo_ = nullptr;
    QSpinBox*  playMctsTurnsSpin_ = nullptr;

    // Suggest menu
    QSpinBox*  suggestDepthSpin_    = nullptr;
    QSpinBox*  suggestTurnsSpin_    = nullptr;
    QComboBox* suggestMctsSecCombo_ = nullptr;

    // Game state
    std::unique_ptr<IrrGo::Graph>  graph_;
    std::unique_ptr<IrrGo::Game>   game_;
    QString currentFilePath_;

    // Setup animation
    QTimer*          stoneTimer_;
    std::vector<int> pendingSetup_;
    int              setupIdx_    = 0;
    int              setupPlaced_ = 0;
    int              setupTarget_ = 0;
    std::mt19937_64  setupRng_;
};
// Copyright Ben Paul Wise. All Rights Reserved.
