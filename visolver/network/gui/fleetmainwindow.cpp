// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// FleetMainWindow implementation: the model-specific half of the fleet
// viewer. The FlowPlanView / histogram / node lists are single-commodity
// widgets, so the window feeds them per-asset SLICES of the fleet instance
// and the working plan (the distance matrix doubles as the slice's cost
// matrix) and swaps slices when the "Asset Displayed" spinner moves. Plans:
// greedy (fast, synchronous, all assets at once) and optimal (solveFleetPlan
// on a worker thread, cached per instance); both can be swapped / purified
// in place, with Reset restoring the pristine plan. The shared skeleton and
// behavior live in PlannerGui.
// ----------------------------------------------
#include "fleetmainwindow.hpp"

#include "swap.hpp"

#include <QtConcurrent/QtConcurrent>

#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSlider>
#include <QSpinBox>

#include <cstdint>
#include <exception>
#include <utility>

namespace VINCP::Network {

  namespace {
    const int kMaxTypes = 10;   // catalog size (assetCatalog etc.)
    // The fleet-multiple slider works in hundredths (two displayed decimals):
    // 100..2500 ticks = x1.00 (the base catalog fleet) .. x25.00.
    const int kFleetMultipleScale = 100;
    const int kFleetMultipleMin = 100;
    const int kFleetMultipleMax = 2500;

    // The single-commodity view of one asset column: C/D/P from that column,
    // the shared distance matrix as the cost matrix, geometry carried over.
    Instance
    assetSlice(const FleetInstance& fleet, Index asset)
    {
      Instance inst;
      inst.numNodes = fleet.numNodes;
      inst.supplyCap = fleet.supplyCap.col(asset);
      inst.demand = fleet.demand.col(asset);
      inst.priority = fleet.priority.col(asset);
      inst.cost = fleet.distance;
      inst.xCoord = fleet.xCoord;
      inst.yCoord = fleet.yCoord;
      inst.labels = fleet.labels;
      return inst;
    }

    Plan
    planSlice(const FleetPlan& plan, Index asset)
    {
      Plan slice;
      slice.supplied = plan.supplied.col(asset);
      slice.resupply = plan.resupply.col(asset);
      slice.flow = plan.flow[static_cast<size_t>(asset)];
      return slice;
    }
  } // namespace

  FleetMainWindow::FleetMainWindow(QWidget* parent)
    : PlannerGui(Texts{.windowTitle = "Fleet-plan instance viewer",
                       .nearestLinksTip =
                           "Draw an orange link from each node to its k "
                           "cheapest neighbours by (d_ij + d_ji)/2.",
                       .histogramTitle = "Distance histogram",
                       .histogramBinsTip =
                           "Number of equal distance-range bins for the "
                           "N^2 distances.",
                       .throughputHeader = "By units"},
                 parent)
  {
    view_->setNodeInfoProvider(
        [this](Index node) { return nodeInfoLines(static_cast<int>(node)); });

    // The fleet-shape controls, above the Regenerate button.
    vehicleTypesBox_ = new QSpinBox(this);
    vehicleTypesBox_->setRange(1, kMaxTypes);
    vehicleTypesBox_->setValue(3);
    vehicleTypesBox_->setToolTip("Number of vehicle types, taken as a prefix "
                                 "of the fixed catalog (truck, airlift, ...).");

    assetTypesBox_ = new QSpinBox(this);
    assetTypesBox_->setRange(1, kMaxTypes);
    assetTypesBox_->setValue(4);
    assetTypesBox_->setToolTip("Number of asset types, taken as a prefix of "
                               "the fixed catalog (dense, bulky, ...).");

    fleetMultipleSlider_ = new QSlider(Qt::Horizontal, this);
    fleetMultipleSlider_->setRange(kFleetMultipleMin, kFleetMultipleMax);
    fleetMultipleSlider_->setValue(kFleetMultipleMin);
    fleetMultipleSlider_->setToolTip(
        "Scale every vehicle type's catalog count (and so its vehicle-mile "
        "budget B_k = N_k v_k H) by this factor, x1.00 to x25.00 in steps "
        "of 0.01. Set it to the greedy plan's fleet scale hint to give the "
        "plan just enough fleet to serve every rationed target. The scaled "
        "counts show in the status line.");
    fleetMultipleLabel_ = new QLabel(this);
    QWidget* multipleRow = new QWidget(this);
    QHBoxLayout* multipleLayout = new QHBoxLayout(multipleRow);
    multipleLayout->setContentsMargins(0, 0, 0, 0);
    multipleLayout->addWidget(fleetMultipleSlider_, 1);
    multipleLayout->addWidget(fleetMultipleLabel_);
    fleetMultipleLabel_->setText(
        QString("x%1").arg(fleetMultiple(), 0, 'f', 2));

    addInstanceRow("Vehicle types", vehicleTypesBox_);
    addInstanceRow("Asset types", assetTypesBox_);
    addInstanceRow("Fleet multiple", multipleRow);

    // The plan radios beyond the base "Closest".
    greedyRadio_ = new QRadioButton("Greedy Fleet Plan", this);
    greedyRadio_->setToolTip("Run the greedy fleet planner (all assets, all "
                             "vehicle types) and overlay the displayed "
                             "asset's flows.");
    optimalRadio_ = new QRadioButton("Optimal Fleet Plan", this);
    optimalRadio_->setToolTip(
        "Solve the reduced conservative fleet QP (KKT mixed complementarity "
        "problem, interior-point engine with the structured fleet Newton "
        "factory, keep-all -- exact by construction) on a worker thread and "
        "overlay the optimal flows. The interior-point solution spreads "
        "tonnage over the optimal face; Purify drives it to a corner.");
    addPlanRadio(greedyRadio_);
    addPlanRadio(optimalRadio_);

    swapOptButton_ = new QPushButton("Swap to Optimum", this);
    swapOptButton_->setEnabled(false);
    swapOptButton_->setToolTip(
        "Drive each asset class in turn to its 2-exchange local optimum "
        "(round-trip distances, deliveries unchanged), then reallocate the "
        "vehicles to the swapped flows. Works on the greedy or the optimal "
        "plan; on the optimal plan it is the faster alternative to Purify "
        "-- mileage drops, but there is no deliberate arc-count reduction. "
        "Runs on a worker thread.");
    purifyButton_ = new QPushButton("Purify (sparsify)", this);
    purifyButton_->setEnabled(false);
    purifyButton_->setToolTip(
        "Swap-as-pivot crossover per asset class within each asset's entry "
        "mileage (G-F9): improving 2-exchanges, then arc-count-reducing "
        "ones at non-negative saving. Deliveries and theta are invariant. "
        "Runs on a worker thread.");
    resetButton_ = new QPushButton("Reset plan", this);
    resetButton_->setEnabled(false);
    addSwapControl(swapOptButton_);
    addSwapControl(purifyButton_);
    addSwapControl(resetButton_);

    // Right panel: the asset selector on top (it scopes everything below).
    assetBox_ = new QSpinBox(this);
    assetBox_->setRange(1, 1);
    assetBox_->setValue(1);
    assetBox_->setToolTip("Which asset type the map, lists, and histogram "
                          "slice shows; popups always show every asset.");
    QGroupBox* assetGroup = new QGroupBox("Asset shown", this);
    QFormLayout* assetForm = new QFormLayout(assetGroup);
    assetForm->addRow("Asset Displayed", assetBox_);
    addRightPanelTop(assetGroup);

    connect(fleetMultipleSlider_, &QSlider::valueChanged, this,
            &FleetMainWindow::onFleetMultipleMoved);
    connect(fleetMultipleSlider_, &QSlider::sliderReleased, this,
            &FleetMainWindow::regenerate);   // counts change the instance
    connect(assetBox_,
            QOverload<int>::of(&QSpinBox::valueChanged), this,
            &FleetMainWindow::applyDisplayedAsset);
    connect(swapOptButton_, &QPushButton::clicked, this,
            &FleetMainWindow::onSwapToOptimum);
    connect(purifyButton_, &QPushButton::clicked, this,
            &FleetMainWindow::onPurify);
    connect(resetButton_, &QPushButton::clicked, this,
            &FleetMainWindow::onResetPlan);

    optimalWatcher_ = new QFutureWatcher<OptimalOutcome>(this);
    connect(optimalWatcher_, &QFutureWatcher<OptimalOutcome>::finished, this,
            &FleetMainWindow::onOptimalFinished);
    purifyWatcher_ = new QFutureWatcher<PurifyOutcome>(this);
    connect(purifyWatcher_, &QFutureWatcher<PurifyOutcome>::finished, this,
            &FleetMainWindow::onPurifyFinished);
    swapWatcher_ = new QFutureWatcher<SwapOutcome>(this);
    connect(swapWatcher_, &QFutureWatcher<SwapOutcome>::finished, this,
            &FleetMainWindow::onSwapFinished);

    regenerate();
    return;
  }

  FleetProfile
  FleetMainWindow::currentProfile() const
  {
    FleetProfile profile;
    profile.geometry = geometryProfile();
    profile.assets = assetCatalog(assetTypesBox_->value());
    profile.vehicles = vehicleCatalog(vehicleTypesBox_->value());
    // Budgets B_k = N_k v_k H scale with the counts, so the fleet multiple
    // scales movement capacity directly; the per-vehicle carrying capacity
    // kappa_ak is untouched. Fractional counts are legal by design (partial
    // availability over the horizon).
    for (VehicleType& vehicle : profile.vehicles) {
      vehicle.count *= fleetMultiple();
    }
    return profile;
  }

  double
  FleetMainWindow::fleetMultiple() const
  {
    return static_cast<double>(fleetMultipleSlider_->value())
           / kFleetMultipleScale;
  }

  void
  FleetMainWindow::onFleetMultipleMoved()
  {
    fleetMultipleLabel_->setText(
        QString("x%1").arg(fleetMultiple(), 0, 'f', 2));
    // While the slider is dragged, only the readout follows; the instance is
    // rebuilt once, on release (regenerating per tick would reroll a
    // surprise-me seed of 0 mid-drag and replan per pixel). Keyboard and
    // page steps arrive with the slider up and regenerate immediately.
    if (!fleetMultipleSlider_->isSliderDown()) {
      regenerate();
    }
    return;
  }

  Index
  FleetMainWindow::displayedAsset() const
  {
    return static_cast<Index>(assetBox_->value() - 1);
  }

  void
  FleetMainWindow::rebuildInstance(std::uint64_t seed)
  {
    const FleetProfile profile = currentProfile();
    validateFleetProfile(profile);
    current_ = makeRandomFleetInstance(profile, seed);
    // Plans and caches belong to the old instance (the base's solveToken_
    // stamps out any solve/purify in flight).
    planKind_ = 0;
    greedyValidP_ = false;
    optimalValidP_ = false;
    // The asset spinner scopes to the new instance; setRange clamps the
    // value, and any resulting valueChanged re-slices harmlessly.
    assetBox_->setRange(1, static_cast<int>(numAssets(current_)));
    return;
  }

  Index
  FleetMainWindow::nodeCount() const
  {
    return current_.numNodes;
  }

  void
  FleetMainWindow::refreshMap()
  {
    const Instance slice = assetSlice(current_, displayedAsset());
    view_->setInstance(slice);      // same geometry: pan/zoom preserved
    histogram_->setInstance(slice);
    return;
  }

  void
  FleetMainWindow::applyPlanMode()
  {
    if (!haveInstanceP_) {
      return;
    }
    nearestBox_->setEnabled(closestRadio_->isChecked());
    if (greedyRadio_->isChecked()) {
      view_->setNearestK(0);
      try {
        if (1 != planKind_) {
          // One plan covers ALL assets and vehicle types; the spinner only
          // picks which slice is drawn. Fast enough for the UI thread.
          if (!greedyValidP_) {
            greedy_ = greedyFleetPlan(current_);
            greedyValidP_ = true;
          }
          workingPlan_ = greedy_.plan;
          planKind_ = 1;
        }
        showWorkingPlan();
      }
      catch (const std::exception& ex) {
        planKind_ = 0;
        view_->clearPlan();
        hideFlowLists();
        refreshPlanControls();
        refreshStatus(QString("fleet greedy failed: %1").arg(ex.what()));
      }
    }
    else if (optimalRadio_->isChecked()) {
      view_->setNearestK(0);
      if (optimalValidP_) {
        if (2 != planKind_) {
          workingPlan_ = optimalResult_.plan;
          planKind_ = 2;
        }
        showWorkingPlan();
      }
      else if (!optimalBusyP_) {
        view_->clearPlan();
        hideFlowLists();
        refreshPlanControls();
        startOptimalSolve();
      }
      else {
        refreshPlanControls();
        refreshStatus("solving optimal fleet plan...");
      }
    }
    else if (closestRadio_->isChecked()) {
      view_->clearPlan();
      hideFlowLists();
      refreshPlanControls();
      applyNearestK();
    }
    else {   // None
      view_->clearPlan();
      hideFlowLists();
      refreshPlanControls();
      view_->setNearestK(0);
      refreshStatus("no links");
    }
    return;
  }

  void
  FleetMainWindow::showWorkingPlan()
  {
    const Plan slice = planSlice(workingPlan_, displayedAsset());
    view_->setPlan(slice);
    showFlowLists(slice, current_.labels);
    refreshPlanControls();
    refreshPlanStatus();
    return;
  }

  void
  FleetMainWindow::startOptimalSolve()
  {
    optimalBusyP_ = true;
    startBusy();
    refreshStatus("solving optimal fleet plan (ipm, screened + certified)...");

    // Everything the worker needs is captured BY VALUE; it touches no
    // widget. Budgets are DATA (B_k = N_k v_k H), so unlike the network
    // viewer there is no calibration pre-pass.
    FleetInstance inst = current_;
    const int token = solveToken_;
    optimalWatcher_->setFuture(QtConcurrent::run([inst, token]() mutable {
      OptimalOutcome out;
      out.token = token;
      try {
        out.result = solveFleetPlan(inst, FleetSolveParams{});
        out.okP = true;
      }
      catch (const std::exception& ex) {
        out.error = QString::fromUtf8(ex.what());
      }
      return out;
    }));
    return;
  }

  void
  FleetMainWindow::onOptimalFinished()
  {
    optimalBusyP_ = false;
    stopBusy();
    const OptimalOutcome out = optimalWatcher_->result();
    if (out.token != solveToken_) {
      refreshPlanControls();
      return;   // stale: the instance changed while the solve ran
    }
    if (!out.okP) {
      refreshPlanControls();
      refreshStatus(QString("optimal solve failed: %1").arg(out.error));
      return;
    }
    optimalResult_ = out.result;
    optimalValidP_ = true;
    if (optimalRadio_->isChecked()) {
      applyPlanMode();   // shows the now-cached plan
    }
    return;
  }

  void
  FleetMainWindow::onSwapToOptimum()
  {
    if (swapBusyP_ || purifyBusyP_ || (1 != planKind_ && 2 != planKind_)) {
      return;
    }
    swapBusyP_ = true;
    startBusy();
    refreshPlanControls();
    refreshStatus("swapping fleet plan (2-exchange per asset)...");

    FleetInstance inst = current_;
    FleetPlan plan = workingPlan_;
    const int token = solveToken_;
    const int kind = planKind_;
    swapWatcher_->setFuture(QtConcurrent::run([inst, plan, token,
                                               kind]() mutable {
      SwapOutcome out;
      out.token = token;
      out.kind = kind;
      try {
        out.summary = swapFleetToLocalOptimum(inst, plan);
        out.plan = std::move(plan);
        out.okP = true;
      }
      catch (const std::exception& ex) {
        out.error = QString::fromUtf8(ex.what());
      }
      return out;
    }));
    return;
  }

  void
  FleetMainWindow::onSwapFinished()
  {
    swapBusyP_ = false;
    stopBusy();
    const SwapOutcome out = swapWatcher_->result();
    if (out.token != solveToken_ || out.kind != planKind_) {
      refreshPlanControls();
      return;   // stale: instance or displayed plan kind changed mid-run
    }
    if (!out.okP) {
      // The worker mutated only its own copy, so the displayed plan is
      // intact; report and keep it.
      refreshPlanControls();
      view_->showSwapResult(
          -1,
          QStringList{QString("swap failed: %1").arg(out.error),
                      "plan unchanged"},
          {});
      return;
    }
    workingPlan_ = out.plan;
    showWorkingPlan();
    QStringList perAsset;
    for (Index a = 0; a < numAssets(current_); ++a) {
      perAsset << QString::number(
          out.summary.swapsPerAsset[static_cast<size_t>(a)]);
    }
    const QStringList lines =
        (out.summary.totalSwaps > 0)
            ? QStringList{QString("%1 swaps (per asset: %2)")
                              .arg(out.summary.totalSwaps)
                              .arg(perAsset.join(", ")),
                          QString("vehicle-miles %1 -> %2")
                              .arg(out.summary.milesUsedBefore.sum(), 0, 'g', 4)
                              .arg(out.summary.milesUsedAfter.sum(), 0, 'g', 4)}
            : QStringList{"already at a local optimum"};
    view_->showSwapResult(-1, lines, {});
    return;
  }

  void
  FleetMainWindow::onPurify()
  {
    if (swapBusyP_ || purifyBusyP_ || (1 != planKind_ && 2 != planKind_)) {
      return;
    }
    purifyBusyP_ = true;
    startBusy();
    refreshPlanControls();
    refreshStatus("purifying fleet plan (swap pivots per asset)...");

    FleetInstance inst = current_;
    FleetPlan plan = workingPlan_;
    const int token = solveToken_;
    const int kind = planKind_;
    purifyWatcher_->setFuture(QtConcurrent::run([inst, plan, token,
                                                 kind]() mutable {
      PurifyOutcome out;
      out.token = token;
      out.kind = kind;
      try {
        out.summary = purifyFleetPlan(inst, plan);
        out.plan = std::move(plan);
        out.okP = true;
      }
      catch (const std::exception& ex) {
        out.error = QString::fromUtf8(ex.what());
      }
      return out;
    }));
    return;
  }

  void
  FleetMainWindow::onPurifyFinished()
  {
    purifyBusyP_ = false;
    stopBusy();
    const PurifyOutcome out = purifyWatcher_->result();
    if (out.token != solveToken_ || out.kind != planKind_) {
      refreshPlanControls();
      return;   // stale: instance or displayed plan kind changed mid-run
    }
    if (!out.okP) {
      // The worker mutated only its own copy (the throw-then-recompute
      // contract applies to that copy), so the displayed plan is intact;
      // report and keep it.
      refreshPlanControls();
      view_->showSwapResult(
          -1,
          QStringList{QString("purify failed: %1").arg(out.error),
                      "plan unchanged"},
          {});
      return;
    }
    workingPlan_ = out.plan;
    showWorkingPlan();
    int arcsBefore = 0, arcsAfter = 0;
    for (size_t a = 0; a < out.summary.arcsBeforePerAsset.size(); ++a) {
      arcsBefore += out.summary.arcsBeforePerAsset[a];
      arcsAfter += out.summary.arcsAfterPerAsset[a];
    }
    const int pivots =
        out.summary.improvingSwaps + out.summary.consolidatingSwaps;
    const QStringList lines =
        (pivots > 0)
            ? QStringList{QString("arcs %1 -> %2 (all assets)")
                              .arg(arcsBefore)
                              .arg(arcsAfter),
                          QString("%1 improving + %2 consolidating pivots")
                              .arg(out.summary.improvingSwaps)
                              .arg(out.summary.consolidatingSwaps),
                          QString("vehicle-miles %1 -> %2")
                              .arg(out.summary.milesUsedBefore.sum(), 0, 'g', 4)
                              .arg(out.summary.milesUsedAfter.sum(), 0, 'g', 4)}
            : QStringList{"already purified"};
    view_->showSwapResult(-1, lines, {});
    return;
  }

  void
  FleetMainWindow::onResetPlan()
  {
    planKind_ = 0;   // re-derive from the caches on the next mode apply
    view_->clearSwapResult();
    applyPlanMode();
    return;
  }

  void
  FleetMainWindow::applyDisplayedAsset()
  {
    if (!haveInstanceP_) {
      return;
    }
    refreshMap();      // setInstance clears the overlay...
    applyPlanMode();   // ...and this re-applies the new asset's slice
    return;
  }

  void
  FleetMainWindow::refreshStatus(const QString& modeNote)
  {
    // The fleet composition (scaled counts) is always visible, so the fleet
    // size is never a mystery: "fleet x3 [truck 120, airlift 9, van 180]".
    QStringList fleet;
    for (const VehicleType& vehicle : current_.vehicles) {
      fleet << QString("%1 %2")
                   .arg(QString::fromStdString(vehicle.name))
                   .arg(vehicle.count, 0, 'g', 6);
    }
    statusLabel_->setText(
        QString("%1 nodes, %2 assets, fleet x%3 [%4] (seed %5) - %6")
            .arg(static_cast<int>(current_.numNodes))
            .arg(static_cast<int>(numAssets(current_)))
            .arg(fleetMultiple(), 0, 'f', 2)
            .arg(fleet.join(", "))
            .arg(static_cast<qulonglong>(lastSeed_))
            .arg(modeNote));
    return;
  }

  void
  FleetMainWindow::refreshPlanStatus()
  {
    const Index a = displayedAsset();
    const Plan slice = planSlice(workingPlan_, a);
    const QString assetName =
        QString::fromStdString(current_.assets[static_cast<size_t>(a)].name);
    const bool optimalP = (2 == planKind_);

    QString head = optimalP
                       ? QString("optimal fleet plan (%1%2)")
                             .arg(optimalResult_.certifiedP
                                      ? "certified"
                                      : "NOT certified")
                             .arg(optimalResult_.vi.converged
                                      ? ""
                                      : ", NOT converged")
                       : QString("fleet greedy plan");

    const double delivered = slice.resupply.sum();
    const double totD = totalFleetDemand(current_, a);
    QString body =
        QString("\nasset %1/%2: %3\narcs: %4\n"
                "supply / demand: %5 / %6 u\ndelivered / unmet: %7 / %8 u")
            .arg(assetBox_->value())
            .arg(static_cast<int>(numAssets(current_)))
            .arg(assetName)
            .arg(positiveArcCount(slice))
            .arg(totalFleetSupplyCap(current_, a), 0, 'f', 0)
            .arg(totD, 0, 'f', 0)
            .arg(delivered, 0, 'f', 0)
            .arg(totD - delivered, 0, 'f', 0);

    // Whole-plan facts, LIVE from the working plan (swaps and purification
    // change mileage but never deliveries).
    body += QString("\nobjective (all assets): %1")
                .arg(fleetShortfallObjective(current_, workingPlan_), 0, 'g', 4);
    if (1 == planKind_) {
      body += QString("\nfleet scale hint: %1")
                  .arg(greedy_.fleetScaleHint, 0, 'g', 3);
    }
    QStringList usage;
    for (Index k = 0; k < numVehicleTypes(current_); ++k) {
      const double budget = vehicleBudget(current_, k);
      const double used = vehicleMiles(current_, workingPlan_, k);
      usage << QString("%1 %2%")
                   .arg(QString::fromStdString(
                       current_.vehicles[static_cast<size_t>(k)].name))
                   .arg(0.0 < budget ? 100.0 * used / budget : 0.0, 0, 'f', 0);
    }
    body += QString("\nutilization: %1").arg(usage.join(", "));
    if (optimalP) {
      QStringList lambdas;
      for (Index k = 0; k < numVehicleTypes(current_); ++k) {
        lambdas << QString::number(optimalResult_.budgetShadowPrice(k), 'g', 3);
      }
      body += QString("\nlambda per type: [%1]").arg(lambdas.join(", "));
    }
    refreshStatus(head + body);
    return;
  }

  void
  FleetMainWindow::refreshPlanControls()
  {
    if (swapBusyP_ || purifyBusyP_) {
      swapOptButton_->setEnabled(false);
      purifyButton_->setEnabled(false);
      resetButton_->setEnabled(false);
      return;
    }
    swapOptButton_->setEnabled(1 == planKind_ || 2 == planKind_);
    purifyButton_->setEnabled(1 == planKind_ || 2 == planKind_);
    resetButton_->setEnabled(0 != planKind_);
    return;
  }

  QStringList
  FleetMainWindow::nodeInfoLines(int node) const
  {
    if (!haveInstanceP_ || node < 0 || current_.numNodes <= node) {
      return QStringList{};
    }
    // C and D as one-entry-per-asset lists, catalog order.
    const auto vectorText = [this, node](const MatrixXd& matrix) {
      QStringList parts;
      for (Index a = 0; a < numAssets(current_); ++a) {
        parts << QString::number(matrix(node, a), 'f', 0);
      }
      return parts.join(", ");
    };
    return QStringList{
        nodeText(current_.labels, node),
        QString("C = [%1]").arg(vectorText(current_.supplyCap)),
        QString("D = [%1]").arg(vectorText(current_.demand)),
    };
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
