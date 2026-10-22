// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <cstddef>
#include <QString>

class QActionGroup;
class QComboBox;
class QMenu;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QWidget;

// Reusable Qt menu/status-bar widgets shared by the game GUIs (irrgo, mancala,
// latrunculi). These were duplicated verbatim across the windows; they now live
// here and take the ranges/options each game wants via a small config struct, so
// no per-game behaviour changes.
namespace guicommon {

// Keep a widget's layout slot reserved while it is hidden (avoids reflow jitter).
void retainSizeWhenHidden(QWidget* w);

// A thin, text-less 0-100 status progress bar that holds its slot while hidden.
QProgressBar* makeStatusBar(QWidget* parent, int heightPx = 14);

// One entry of a time-budget menu (combo data is the seconds value).
struct TimeOption {
    int secs;
    const char* label;
};
// Historical name, kept so existing call sites read unchanged.
using MctsOption = TimeOption;

// Config for any time-budgeted submenu (MCTS or NegaMax -- both are wall-clock budgets).
struct TimeMenuConfig {
    const TimeOption* options = nullptr;
    std::size_t optionCount = 0;
    bool withTurns = true;
    int turnsMin = 1, turnsMax = 999, turnsDefault = 10;
};
using MctsMenuConfig = TimeMenuConfig;

// Build a checkable "MCTS" submenu (Time [+ Turns] + Go!) under `parent`, added to the
// exclusive `group`; `owner` parents the created QObjects. secOut receives the time combo
// (item data = seconds); turnsOut receives the Turns spinbox, or nullptr if !withTurns.
// Returns the Go! button so the caller can connect it to a slot.
QPushButton* buildMctsMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                           const TimeMenuConfig& cfg,
                           QComboBox*& secOut, QSpinBox*& turnsOut);

// The same widget set under a "NegaMax" heading. NegaMax runs iterative deepening in
// every game now, so a wall clock is what bounds it and a fixed depth no longer describes
// what the engine does -- depth survives only as a ceiling the clock rarely lets it reach.
// The depth-budgeted builder this replaced was removed once its last caller migrated.
QPushButton* buildNegaMaxTimeMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                                  const TimeMenuConfig& cfg,
                                  QComboBox*& secOut, QSpinBox*& turnsOut);

// Config for the "Computer" (human-vs-computer) submenu: a NegaMax think-time budget plus
// a choice of which side the human plays. The two side labels name the players the way the
// game does (A/B, Black/White, P0/P1); the combo data is the player index (0 or 1).
struct ComputerMenuConfig {
    const TimeOption* options = nullptr;
    std::size_t optionCount = 0;
    const char* side0Label = "First";   // player index 0
    const char* side1Label = "Second";  // player index 1
    int defaultHumanSide = 0;           // side the human plays by default (0 or 1)
};

// Build a checkable "Computer" submenu (Think + You play + Go!) under `parent`, added to the
// exclusive `group`; `owner` parents the created QObjects. secOut receives the think-time
// combo (item data = seconds); sideOut receives the side combo (item data = the player index
// the human plays). Returns the Go! button so the caller can connect it to a slot.
QPushButton* buildComputerMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                               const ComputerMenuConfig& cfg,
                               QComboBox*& secOut, QComboBox*& sideOut);

// Shows the "About <appName>" dialog carrying the project's copyright notice. Shared
// across the game GUIs so that notice lives in one place instead of three.
void showAboutDialog(QWidget* owner, const QString& appName);

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
