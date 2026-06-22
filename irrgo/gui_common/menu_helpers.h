// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <cstddef>

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

// One entry of an MCTS time menu (combo data is the seconds value).
struct MctsOption {
    int secs;
    const char* label;
};

struct NegaMaxMenuConfig {
    int depthMin = 1, depthMax = 6, depthDefault = 2;
    bool withTurns = true;
    int turnsMin = 1, turnsMax = 50, turnsDefault = 2;
};

struct MctsMenuConfig {
    const MctsOption* options = nullptr;
    std::size_t optionCount = 0;
    bool withTurns = true;
    int turnsMin = 1, turnsMax = 999, turnsDefault = 10;
};

// Build a checkable "NegaMax" submenu (Depth [+ Turns] + Go!) under `parent`,
// added to the exclusive `group`; `owner` parents the created QObjects.
// depthOut/turnsOut receive the controls (turnsOut == nullptr if !withTurns).
// Returns the Go! button so the caller can connect it to a slot.
QPushButton* buildNegaMaxMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                              const NegaMaxMenuConfig& cfg,
                              QSpinBox*& depthOut, QSpinBox*& turnsOut);

// Build a checkable "MCTS" submenu (Time [+ Turns] + Go!). secOut receives the
// time combo (item data = seconds); turnsOut as above. Returns the Go! button.
QPushButton* buildMctsMenu(QWidget* owner, QMenu* parent, QActionGroup* group,
                           const MctsMenuConfig& cfg,
                           QComboBox*& secOut, QSpinBox*& turnsOut);

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
