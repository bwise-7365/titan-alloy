// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include "BoardWidget.h"
#include "Game.h"            // Latrunculi::Game, Move
#include "GameMainWindow.h"

#include <QColor>
#include <QString>
#include <memory>

class QAction;
class QComboBox;
class QLabel;
class QPushButton;
class QSpinBox;
class QTextEdit;

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
    void onSave();
    void onLoad();

private:
    void buildMenuBar();
    void updateControls() override;
    AbsGame::Game* currentGame() override;
    void applyComputedMove(AbsGame::MoveId mv) override;

    void newGame(int rows, int columns, int perSide);
    void refreshBoard();                 // board update + controls + tallies
    void logMove(const Latrunculi::Move& m);
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
    QLabel*      tallyLabel_      = nullptr;
    QPushButton* stopBtn_         = nullptr;
    QTextEdit*   suggestedLog_    = nullptr;
    QPushButton* clearSuggestBtn_ = nullptr;
    QTextEdit*   moveLog_         = nullptr;

    // Board menu controls.
    QSpinBox* rowsSpin_    = nullptr;
    QSpinBox* colsSpin_    = nullptr;
    QSpinBox* perSideSpin_ = nullptr;
    QColor    colorA_{0xFA, 0xE5, 0xBE};  // pale beige (side_a)
    QColor    colorB_{0x85, 0x25, 0x32};  // brick-red  (side_b)
    QColor    background_{0x8C, 0x8E, 0x7E};  // SvgStyle default background

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
};
// Copyright Ben Paul Wise. All Rights Reserved.
