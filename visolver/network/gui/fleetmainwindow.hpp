// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Main window for the FLEET viewer: the network viewer's layout driven by the
// multi-asset / multi-vehicle fleet model (fleetinstance/fleetgreedy/
// fleetsolve). The map, histogram, and node lists show ONE asset at a time
// via the "Asset Displayed" spinner; node popups show the full per-asset
// C / D vectors. Plans: greedy (synchronous) and optimal (solveFleetPlan on
// a worker thread), both improvable by swaps / purification.
// ----------------------------------------------
#ifndef VINCP_NETWORK_FLEETMAINWINDOW_HPP
#define VINCP_NETWORK_FLEETMAINWINDOW_HPP

#include "costhistogram.hpp"
#include "fleetgreedy.hpp"
#include "fleetsolve.hpp"
#include "fleetswap.hpp"
#include "flowplanview.hpp"
#include "nodelistwidget.hpp"

#include <QFutureWatcher>
#include <QMainWindow>
#include <QString>
#include <QStringList>

class QComboBox;
class QSpinBox;
class QLabel;
class QRadioButton;
class QCheckBox;
class QPushButton;
class QProgressBar;
class QTimer;

namespace VINCP::Network {

  class FleetMainWindow : public QMainWindow
  {
    Q_OBJECT

  public:
    explicit FleetMainWindow(QWidget* parent = nullptr);

  private slots:
    // Rebuild the fleet instance from the current control values.
    void regenerate();
    // Push the nearest-neighbour spin value into the view (no regeneration).
    void applyNearestK();
    // Apply the current plan-mode radio (none/closest/greedy/optimal).
    void applyPlanMode();
    // The "Asset Displayed" spinner moved: re-slice the map for that asset.
    void applyDisplayedAsset();
    // Drive every asset class in turn to its 2-exchange local optimum and
    // reallocate the vehicles (greedy plan only; synchronous, it is fast).
    void onSwapToOptimum();
    // Purify the working plan (greedy or optimal) on a worker thread.
    void onPurify();
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

    // 0-based column of the 1-based "Asset Displayed" spinner.
    Index displayedAsset() const;

    // Push the displayed asset's instance slice into the view + histogram.
    // (The view keeps its pan/zoom: the geometry is unchanged across assets.)
    void refreshMapForAsset();

    // Display the working plan: slice overlay, lists, buttons, status.
    void showWorkingPlan();

    void refreshStatus(const QString& modeNote);
    void refreshPlanStatus();     // live numbers from the working plan
    void refreshPlanControls();   // per-kind button enabling
    void showFlowLists(const Plan& planSlice);
    void hideFlowLists();

    // Busy bar shared by the optimal solve and purification (unknowable
    // durations; the bar refills until the worker reports back).
    void startBusy();
    void stopBusy();

    // Popup lines: node label, then C and D as one-entry-per-asset lists.
    QStringList nodeInfoLines(int node) const;

    FlowPlanView* view_ = nullptr;

    QComboBox* laydownBox_ = nullptr;
    QSpinBox* seedBox_ = nullptr;
    QSpinBox* supplyOnlyBox_ = nullptr;
    QSpinBox* bothBox_ = nullptr;
    QSpinBox* demandOnlyBox_ = nullptr;
    QSpinBox* transitBox_ = nullptr;
    QSpinBox* vehicleTypesBox_ = nullptr;   // catalog prefix, 1..10
    QSpinBox* assetTypesBox_ = nullptr;     // catalog prefix, 1..10
    QCheckBox* hugeFleetCheck_ = nullptr;   // x1000 vehicle counts
    QSpinBox* nearestBox_ = nullptr;
    QRadioButton* noneRadio_ = nullptr;
    QRadioButton* closestRadio_ = nullptr;
    QRadioButton* greedyRadio_ = nullptr;
    QRadioButton* optimalRadio_ = nullptr;
    QPushButton* swapOptButton_ = nullptr;
    QPushButton* purifyButton_ = nullptr;
    QPushButton* resetButton_ = nullptr;
    QProgressBar* busyBar_ = nullptr;
    QTimer* busyTimer_ = nullptr;
    int busyCount_ = 0;
    QCheckBox* labelsCheck_ = nullptr;
    CostHistogram* histogram_ = nullptr;
    QSpinBox* histBinsBox_ = nullptr;
    QSpinBox* assetBox_ = nullptr;          // "Asset Displayed", 1-based
    NodeListWidget* throughputList_ = nullptr;
    NodeListWidget* countList_ = nullptr;
    QLabel* statusLabel_ = nullptr;

    // The instance currently on screen; lastSeed_ produced it.
    FleetInstance current_;
    std::uint64_t lastSeed_ = 0;
    bool haveInstanceP_ = false;

    // The working plan and its kind: 0 none, 1 greedy, 2 optimal. The
    // working copy persists across mode/asset toggles and is MUTATED by
    // swaps and purification; Reset re-derives it from the caches below.
    FleetPlan workingPlan_;
    int planKind_ = 0;

    // Greedy cache (per instance): targets/hint/unserved for the status.
    FleetGreedyResult greedy_;
    bool greedyValidP_ = false;

    // --- optimal plan (solveFleetPlan on a worker thread) ---
    // Budgets are DATA, so no calibration pre-pass; cached per instance;
    // solveToken_ stamps launches so stale results are discarded.
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
    int solveToken_ = 0;

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
  };

} // namespace VINCP::Network

#endif // VINCP_NETWORK_FLEETMAINWINDOW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
