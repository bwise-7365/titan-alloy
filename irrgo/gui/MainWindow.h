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

private:
    void buildMenuBar();
    void updateControls();
    void logLastMove();
    void stopStoneSetup();

    // Central UI
    BoardWidget*  boardWidget_;
    QLabel*       currentPlayerLabel_;
    QPushButton*  blackPassBtn_;
    QPushButton*  whitePassBtn_;
    QTextEdit*    moveLog_;

    // Board menu embedded controls
    QComboBox*    sizeCombo_;
    QCheckBox*    irregularCheck_;
    QComboBox*    bgCombo_;

    // Random menu
    QLineEdit*    randomSeedEdit_;

    // Stones menu
    QActionGroup* stonesGroup_;

    // Game state
    std::unique_ptr<Graph>  graph_;
    std::unique_ptr<Game>   game_;

    // Setup animation
    QTimer*          stoneTimer_;
    std::vector<int> pendingSetup_;
    int              setupIdx_    = 0;
    int              setupPlaced_ = 0;
    int              setupTarget_ = 0;
    std::mt19937_64  setupRng_;
};
