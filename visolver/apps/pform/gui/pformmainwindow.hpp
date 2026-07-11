// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Main window for pform_gui: the Qt adaptation of the old ProbPForm window --
// Parties/Issues/seed controls, the editable party grid, and the result panes
// (log, deterministic parliament, coalition table). Solving and coalition
// post-processing go through pformapp exactly as pform_cli.
// ----------------------------------------------
#ifndef VIMCP_APPS_PFORMMAINWINDOW_HPP
#define VIMCP_APPS_PFORMMAINWINDOW_HPP

#include "pformpartytable.hpp"
#include "pformtext.hpp"

#include <QFutureWatcher>
#include <QMainWindow>

class QAction;
class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QLineEdit;
class QPlainTextEdit;
class QProgressBar;
class QPushButton;
class QSpinBox;
class QTableWidget;

namespace VIMCP::App {

  // The pceDistrib-era controls of the exemplar (voting-rule radios, issue
  // forecast, parliament-displayed radios, SQ/goal strips) are deliberately
  // absent: PformResult::deterministic replaced them.
  class PformMainWindow : public QMainWindow
  {
    Q_OBJECT

  public:
    explicit PformMainWindow(QWidget* parent = nullptr);

  protected:

  private slots:
    // Resolve the seed (0 = surprise-me), generate a fresh random instance at
    // the spinner sizes, and populate the grid.
    void onReset();
    // Parties/Issues spinner moved: refresh the K = M^D display and the Reset
    // enable (the grid itself changes only on Reset / Open).
    void onCountsChanged();
    // A user edit in the grid: the shown results no longer belong to it.
    void onCellEdited();
    // File > Open GMS: read an instance through readPformGms (the pform_cli
    // file path exactly) and install it with its labels and q.
    void onOpenGms();
    // Read + validate the grid, then run PForm::solve and pformCoalitions on
    // a worker thread (the pform_cli pipeline exactly).
    void onSolve();
    // Background completion: discard if stale, else render everything.
    void onSolveFinished();
    // A coalition row was selected: highlight its pattern in the grid.
    void onCoalitionSelected();

  private:
    // Everything a solve produces, computed on the worker and stamped with
    // the launch token (the fleet windows' outcome pattern): stale results
    // are discarded, errors are reported without touching the display.
    struct SolveOutcome {
      bool okP = false;
      QString error;
      VIResult vi;
      PformResult result;
      std::vector<PformCoalition> coalitions;
      PformInstance instance;   // the inputs the worker actually solved
      PformParams params;
      int token = 0;
    };

    // "K = M^D = ..." or the red over-cap message; gates the Reset button.
    void refreshKDisplay();
    // Install an instance: rebuild + fill the grid, set the provenance text
    // ("random seed 42" / the file name) in the window title, invalidate
    // results.
    void populateFromInstance(const PformInstance& in, const QString& source);
    // Clear utilities, highlights, the coalition table, and the deterministic
    // label (the log pane persists, console-style).
    void clearResults();
    // Append shared-renderer text to the log pane verbatim (no extra breaks).
    void appendLog(const std::string& text);
    // Grey the generation/solve controls and run the busy bar while a solve
    // is in flight (the grid stays editable: an edit bumps the token, so the
    // running solve's result is discarded on arrival).
    void setBusy(bool busyP);

    QSpinBox* partiesBox = nullptr;
    QSpinBox* issuesBox = nullptr;
    QLineEdit* seedEdit = nullptr;
    QDoubleSpinBox* qBox = nullptr;
    QComboBox* engineCombo = nullptr;
    QLabel* kLabel = nullptr;
    QProgressBar* busyBar = nullptr;
    QPushButton* resetButton = nullptr;
    QPushButton* solveButton = nullptr;
    QAction* openAction = nullptr;
    PformPartyTable* partyTable = nullptr;
    QPlainTextEdit* logPane = nullptr;
    QLabel* deterministicLabel = nullptr;
    QTableWidget* coalitionTable = nullptr;

    // The instance on display (data lives in the grid; this carries labels
    // and provenance for rendering).
    PformInstance current;
    // Bumped by Reset/Open/edits; stamps background solves.
    int solveToken = 0;
    QFutureWatcher<SolveOutcome>* solveWatcher = nullptr;
    bool solveBusyP = false;
    // The last solve's coalition structure and deterministic matching, kept
    // for the selection-driven highlights.
    std::vector<PformCoalition> coalitions;
    std::vector<Index> deterministicMatching;
  };

} // namespace VIMCP::App

#endif // VIMCP_APPS_PFORMMAINWINDOW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
