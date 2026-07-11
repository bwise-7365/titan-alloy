// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Main window for the FLEET viewer: the PlannerGui skeleton driven by the
// multi-asset / multi-vehicle fleet model (fleetinstance/fleetgreedy/
// fleetsolve). The map, histogram, and node lists show ONE asset at a time
// via the "Asset Displayed" spinner; node popups show the full per-asset
// C / D vectors. Plans: greedy (synchronous) and optimal (solveFleetPlan on
// a worker thread), both improvable by swaps / purification.
// ----------------------------------------------
#ifndef VINCP_NETWORK_FLEETMAINWINDOW_HPP
#define VINCP_NETWORK_FLEETMAINWINDOW_HPP

#include "fleetgreedy.hpp"
#include "fleetsolve.hpp"
#include "fleetswap.hpp"
#include "plannergui.hpp"

#include <QFutureWatcher>
#include <QString>
#include <QStringList>

class QLabel;
class QPushButton;
class QRadioButton;
class QSlider;
class QSpinBox;

namespace VINCP::Network {

  class FleetMainWindow : public PlannerGui
  {
    Q_OBJECT

  public:
    explicit FleetMainWindow(QWidget* parent = nullptr);

  protected:
    // PlannerGui hooks.
    void rebuildInstance(std::uint64_t seed) override;
    Index nodeCount() const override;
    // Push the displayed asset's instance slice into the view + histogram.
    // (The view keeps its pan/zoom: the geometry is unchanged across assets.)
    void refreshMap() override;
    // Apply the current plan-mode radio (none/closest/greedy/optimal).
    void applyPlanMode() override;
    void refreshStatus(const QString& modeNote) override;

  private slots:
    // The fleet-multiple slider moved: update the readout; rebuild the
    // instance unless a drag is in progress (then the release rebuilds).
    void onFleetMultipleMoved();
    // The "Asset Displayed" spinner moved: re-slice the map for that asset.
    void applyDisplayedAsset();
    // Drive every asset class in turn to its 2-exchange local optimum and
    // reallocate the vehicles, on a worker thread (greedy or optimal plan;
    // the faster, weaker sibling of Purify).
    void onSwapToOptimum();
    // Purify the working plan (greedy or optimal) on a worker thread.
    void onPurify();
    // Background completion of the swap.
    void onSwapFinished();
    // Discard swaps/purification: recompute greedy or re-copy the cached
    // optimal solve.
    void onResetPlan();
    // Background completions.
    void onOptimalFinished();
    void onPurifyFinished();

  private:
    // Assemble a FleetProfile from the control values: geometry from the node
    // class spinners, types as catalog prefixes of the two type spinners.
    FleetProfile currentProfile() const;

    // The fleet-multiple slider's value as the factor it names (ticks are
    // hundredths: 318 -> 3.18).
    double fleetMultiple() const;

    // 0-based column of the 1-based "Asset Displayed" spinner.
    Index displayedAsset() const;

    // Display the working plan: slice overlay, lists, buttons, status.
    void showWorkingPlan();

    void refreshPlanStatus();     // live numbers from the working plan
    void refreshPlanControls();   // per-kind button enabling

    // Popup lines: node label, then C and D as one-entry-per-asset lists.
    QStringList nodeInfoLines(int node) const;

    QSpinBox* vehicleTypesBox_ = nullptr;      // catalog prefix, 1..10
    QSpinBox* assetTypesBox_ = nullptr;        // catalog prefix, 1..10
    QSlider* fleetMultipleSlider_ = nullptr;   // x1.00..x25.00 in 0.01 ticks
    QLabel* fleetMultipleLabel_ = nullptr;     // "x3.18" readout beside it
    QRadioButton* greedyRadio_ = nullptr;
    QRadioButton* optimalRadio_ = nullptr;
    QPushButton* swapOptButton_ = nullptr;
    QPushButton* purifyButton_ = nullptr;
    QPushButton* resetButton_ = nullptr;
    QSpinBox* assetBox_ = nullptr;          // "Asset Displayed", 1-based

    // The instance currently on screen.
    FleetInstance current_;

    // The working plan and its kind: 0 none, 1 greedy, 2 optimal. The
    // working copy persists across mode/asset toggles and is MUTATED by
    // swaps and purification; Reset re-derives it from the caches below.
    FleetPlan workingPlan_;
    int planKind_ = 0;

    // Greedy cache (per instance): targets/hint/unserved for the status.
    FleetGreedyResult greedy_;
    bool greedyValidP_ = false;

    // --- optimal plan (solveFleetPlan on a worker thread) ---
    // Budgets are DATA, so no calibration pre-pass; cached per instance; the
    // base solveToken_ stamps launches so stale results are discarded.
    struct OptimalOutcome {
      bool okP = false;
      QString error;
      FleetSolveResult result;
      int token = 0;
    };
    void startOptimalSolve();
    QFutureWatcher<OptimalOutcome>* optimalWatcher_ = nullptr;
    FleetSolveResult optimalResult_;   // valid iff optimalValidP_
    bool optimalValidP_ = false;
    bool optimalBusyP_ = false;

    // --- purification (purifyFleetPlan on a worker thread) ---
    struct PurifyOutcome {
      bool okP = false;
      QString error;
      FleetPlan plan;
      FleetPurifySummary summary;
      int token = 0;
      int kind = 0;
    };
    QFutureWatcher<PurifyOutcome>* purifyWatcher_ = nullptr;
    bool purifyBusyP_ = false;

    // --- swap to optimum (swapFleetToLocalOptimum on a worker thread) ---
    // Same discipline as purification: the worker owns a copy, results are
    // stamped by (token, kind), and swap/purify exclude each other because
    // both replace the working plan.
    struct SwapOutcome {
      bool okP = false;
      QString error;
      FleetPlan plan;
      FleetSwapSummary summary;
      int token = 0;
      int kind = 0;
    };
    QFutureWatcher<SwapOutcome>* swapWatcher_ = nullptr;
    bool swapBusyP_ = false;
  };

} // namespace VINCP::Network

#endif // VINCP_NETWORK_FLEETMAINWINDOW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
