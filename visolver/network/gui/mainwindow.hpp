// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Main window for the instance viewer: the PlannerGui skeleton driven by the
// single-commodity flow-plan model, with greedy / gravity / optimal plans and
// the interactive swap (2-exchange) toolkit.
// ----------------------------------------------
#ifndef VINCP_NETWORK_MAINWINDOW_HPP
#define VINCP_NETWORK_MAINWINDOW_HPP

#include "flowplan.hpp"
#include "plannergui.hpp"
#include "swap.hpp"

#include <QFutureWatcher>
#include <QString>

class QPushButton;
class QRadioButton;

namespace VINCP::Network {

  class MainWindow : public PlannerGui
  {
    Q_OBJECT

  public:
    explicit MainWindow(QWidget* parent = nullptr);

  protected:
    // PlannerGui hooks.
    void rebuildInstance(std::uint64_t seed) override;
    Index nodeCount() const override;
    void refreshMap() override;
    // Apply the current plan-mode radio to the CURRENT instance: overlay the
    // greedy / gravity / optimal plan or show placement only.
    void applyPlanMode() override;
    // Set the status line from the current instance plus the given mode note.
    void refreshStatus(const QString& modeNote) override;

  private slots:
    // Swap (2-exchange) actions on the working plan.
    void onNodeSwap(int node);           // v1: right-clicked node's best swap
    void onNodeSwapToOptimum(int node);  // intermediate: that node to its optimum
    void onBestSwap();                   // v2: global best swap
    void onSwapToOptimum();              // v3: iterate to global local optimum
    void onResetSwaps();                 // discard swaps, recompute fresh greedy
    void onPurify();                     // purifyPlan on a worker thread (F4)

    // The background optimal-plan solve finished (success or failure).
    void onOptimalFinished();
    // The background purification finished (adopt unless stale).
    void onPurifyFinished();

  private:
    // Rebuild the multi-field status from the working plan (greedy or gravity).
    void refreshPlanStatus();

    // Apply a swap move (if improving) to the working plan, refresh the plan
    // overlay / lists / status, and show the result popup anchored at a node.
    void applyAndShowSwap(const SwapMove& move, int anchorNode);

    // The popup lines describing an improving swap (edges, tons, saving).
    QStringList buildSwapLines(const SwapMove& move) const;

    // Enable / disable the swap buttons together (purify included).
    void setSwapControlsEnabled(bool onP);
    // Re-apply the per-mode swap/purify button states: greedy gets everything,
    // the optimal overlay gets Reset + Purify only, other modes get nothing.
    void refreshSwapControls();

    QRadioButton* greedyRadio_ = nullptr;
    QRadioButton* gravityRadio_ = nullptr;
    QRadioButton* optimalRadio_ = nullptr;
    QPushButton* swapBestButton_ = nullptr;
    QPushButton* swapOptButton_ = nullptr;
    QPushButton* swapResetButton_ = nullptr;
    QPushButton* purifyButton_ = nullptr;

    // The instance currently on screen, so a plan-mode switch can recompute
    // without regenerating.
    Instance current_;

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
    // the error text. Cached per instance; the base solveToken_ stamps each
    // launch so a result arriving after a Regenerate is discarded as stale.
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

    // --- purification (purifyPlan on a worker thread; plan.md F4) ---
    // The worker purifies a COPY of the working plan (the spread optimal plan
    // has thousands of positive arcs, and the pivot loop is O(arcs^2) per
    // pivot -- the recorded gravity-swap freeze lesson applies). The result
    // is adopted only if the instance (token) and plan kind are unchanged.
    struct PurifyOutcome {
      Plan plan;
      PurifySummary summary;
      int token = 0;
      int kind = 0;
    };
    QFutureWatcher<PurifyOutcome>* purifyWatcher_ = nullptr;
    bool purifyBusyP_ = false;
  };

} // namespace VINCP::Network

#endif // VINCP_NETWORK_MAINWINDOW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
