// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Main window for the fleet_app: a copy of the network fleet viewer's window,
// renamespaced to VINCP::App and routing ALL solving through the App::Fleet
// problem class. The shared skeleton and widgets (PlannerGui, FlowPlanView,
// CostHistogram, NodeListWidget) are REUSED unchanged from viewer_common.
// ----------------------------------------------
#ifndef VINCP_APPS_FLEETAPPMAINWINDOW_HPP
#define VINCP_APPS_FLEETAPPMAINWINDOW_HPP

#include "fleetproblem.hpp"   // App::Fleet + the network fleet types it wraps
#include "plannergui.hpp"     // VINCP::Network::PlannerGui, from viewer_common

#include <QFutureWatcher>
#include <QString>
#include <QStringList>

class QLabel;
class QPushButton;
class QRadioButton;
class QSlider;
class QSpinBox;

namespace VINCP::App {

  // Derives from the shared viewer base (in namespace VINCP::Network). Only the
  // solver call sites differ from the network fleet viewer: they go through an
  // App::Fleet instead of calling the network free functions directly.
  class FleetAppMainWindow : public VINCP::Network::PlannerGui
  {
    Q_OBJECT

  public:
    explicit FleetAppMainWindow(QWidget* parent = nullptr);

  protected:
    // PlannerGui hooks.
    void rebuildInstance(std::uint64_t seed) override;
    Index nodeCount() const override;
    void refreshMap() override;
    void applyPlanMode() override;
    void refreshStatus(const QString& modeNote) override;

  private slots:
    void onFleetMultipleMoved();
    void applyDisplayedAsset();
    void onSwapToOptimum();
    void onPurify();
    void onResetPlan();
    void onOptimalFinished();
    void onPurifyFinished();
    void onSwapFinished();

  private:
    Network::FleetProfile currentProfile() const;
    double fleetMultiple() const;
    Index displayedAsset() const;
    void showWorkingPlan();
    void refreshPlanStatus();
    void refreshPlanControls();
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

    Network::FleetInstance current_;

    Network::FleetPlan workingPlan_;
    int planKind_ = 0;

    Network::FleetGreedyResult greedy_;
    bool greedyValidP_ = false;

    // --- optimal plan (Fleet::solve on a worker thread) ---
    struct OptimalOutcome {
      bool okP = false;
      QString error;
      FleetResult result;          // App::FleetResult (from Fleet::solve)
      int token = 0;
    };
    void startOptimalSolve();
    QFutureWatcher<OptimalOutcome>* optimalWatcher_ = nullptr;
    FleetResult optimalResult_;    // valid iff optimalValidP_
    bool optimalValidP_ = false;
    bool optimalBusyP_ = false;

    // --- purification (Fleet::sparsify on a worker thread) ---
    struct PurifyOutcome {
      bool okP = false;
      QString error;
      Network::FleetPlan plan;
      Network::FleetPurifySummary summary;
      int token = 0;
      int kind = 0;
    };
    QFutureWatcher<PurifyOutcome>* purifyWatcher_ = nullptr;
    bool purifyBusyP_ = false;

    // --- swap to optimum (Fleet::swapToLocalOptimum on a worker thread) ---
    // Same discipline as purification: the worker owns a copy, results are
    // stamped by (token, kind), and swap/purify exclude each other because
    // both replace the working plan.
    struct SwapOutcome {
      bool okP = false;
      QString error;
      Network::FleetPlan plan;
      Network::FleetSwapSummary summary;
      int token = 0;
      int kind = 0;
    };
    QFutureWatcher<SwapOutcome>* swapWatcher_ = nullptr;
    bool swapBusyP_ = false;
  };

} // namespace VINCP::App

#endif // VINCP_APPS_FLEETAPPMAINWINDOW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
