// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Main window for the instance viewer: a FlowPlanView plus a control panel to
// pick the laydown, seed, node-class counts, and the nearest-neighbour count.
// ----------------------------------------------
#ifndef VINCP_NETWORK_MAINWINDOW_HPP
#define VINCP_NETWORK_MAINWINDOW_HPP

#include "costhistogram.hpp"
#include "flowplan.hpp"
#include "flowplanview.hpp"
#include "nodelistwidget.hpp"
#include "plan.hpp"
#include "swap.hpp"

#include <QFutureWatcher>
#include <QMainWindow>
#include <QString>

class QComboBox;
class QSpinBox;
class QLabel;
class QRadioButton;
class QCheckBox;
class QPushButton;

namespace VINCP::Network {

  class MainWindow : public QMainWindow
  {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);

  private slots:
    // Rebuild the instance from the current control values and refresh the view.
    void regenerate();
    // Push the nearest-neighbour spin value into the view (no regeneration).
    void applyNearestK();
    // Apply the current plan-mode radio to the CURRENT instance: overlay the
    // greedy plan or show placement only (no regeneration).
    void applyPlanMode();

    // Swap (2-exchange) actions on the working plan.
    void onNodeSwap(int node);           // v1: right-clicked node's best swap
    void onNodeSwapToOptimum(int node);  // intermediate: that node to its optimum
    void onBestSwap();                   // v2: global best swap
    void onSwapToOptimum();              // v3: iterate to global local optimum
    void onResetSwaps();                 // discard swaps, recompute fresh greedy

    // The background optimal-plan solve finished (success or failure).
    void onOptimalFinished();

  private:
    // Assemble an InstanceProfile from the control values.
    InstanceProfile currentProfile() const;

    // Set the status line from the current instance plus the given mode note.
    void refreshStatus(const QString& modeNote);

    // Rebuild the multi-field status from the working plan (greedy or gravity).
    void refreshPlanStatus();

    // Apply a swap move (if improving) to the working plan, refresh the plan
    // overlay / lists / status, and show the result popup anchored at a node.
    void applyAndShowSwap(const SwapMove& move, int anchorNode);

    // The popup lines describing an improving swap (edges, tons, saving).
    QStringList buildSwapLines(const SwapMove& move) const;

    // Enable / disable the swap buttons together.
    void setSwapControlsEnabled(bool onP);

    // Fill both node lists from a plan and enable them / clear and grey them.
    void showFlowLists(const Plan& plan);
    void hideFlowLists();

    FlowPlanView* view_ = nullptr;

    QComboBox* laydownBox_ = nullptr;
    QSpinBox* seedBox_ = nullptr;
    QSpinBox* supplyOnlyBox_ = nullptr;
    QSpinBox* bothBox_ = nullptr;
    QSpinBox* demandOnlyBox_ = nullptr;
    QSpinBox* transitBox_ = nullptr;
    QSpinBox* nearestBox_ = nullptr;
    QRadioButton* noneRadio_ = nullptr;
    QRadioButton* closestRadio_ = nullptr;
    QRadioButton* greedyRadio_ = nullptr;
    QRadioButton* gravityRadio_ = nullptr;
    QRadioButton* optimalRadio_ = nullptr;
    QCheckBox* labelsCheck_ = nullptr;
    CostHistogram* histogram_ = nullptr;
    QSpinBox* histBinsBox_ = nullptr;
    NodeListWidget* throughputList_ = nullptr;
    NodeListWidget* countList_ = nullptr;
    QPushButton* swapBestButton_ = nullptr;
    QPushButton* swapOptButton_ = nullptr;
    QPushButton* swapResetButton_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    // The instance currently on screen, so a plan-mode switch can recompute
    // without regenerating. lastSeed_ is the seed that produced it.
    Instance current_;
    std::uint64_t lastSeed_ = 0;
    bool haveInstanceP_ = false;

    // The working plan: computed once per (instance, kind) and -- for greedy --
    // MUTATED by swaps, persisting across None/Closest toggles until Regenerate,
    // Reset, or a switch to the other plan kind. workingKind_: 0 none, 1 greedy,
    // 2 gravity, 3 optimal.
    Plan workingPlan_;
    VectorXd greedyTargets_;      // phase-1 rationed targets (greedy objective)
    double baseTonMiles_ = 0.0;   // fresh plan's ton-miles (for cumulative save)
    int swapCount_ = 0;           // swaps applied since the last fresh plan
    int workingKind_ = 0;

    // --- optimal plan (solveFlowPlan, computed on a worker thread) ---
    // The solve runs off the UI thread (a multi-second synchronous solve would
    // freeze the viewer -- the recorded gravity-swap lesson). What the worker
    // returns: the outcome, the calibrated budget it used, and (on failure)
    // the error text. Cached per instance; solveToken_ stamps each launch so a
    // result arriving after a Regenerate is discarded as stale.
    struct OptimalOutcome {
      bool okP = false;
      QString error;
      FlowPlanResult result;
      double budgetUsed = 0.0;
      int token = 0;
    };
    void startOptimalSolve();

    QFutureWatcher<OptimalOutcome>* optimalWatcher_ = nullptr;
    FlowPlanResult optimalResult_;   // valid iff optimalValidP_
    double optimalBudget_ = 0.0;     // tonMileLimit the solve was run with
    bool optimalValidP_ = false;     // cached result matches current_
    bool optimalBusyP_ = false;      // a solve is in flight
    int solveToken_ = 0;             // bumped on Regenerate; stamps launches
  };

} // namespace VINCP::Network

#endif // VINCP_NETWORK_MAINWINDOW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
