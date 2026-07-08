// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// MainWindow implementation: the model-specific half of the network viewer --
// the plan radios, the swap toolkit, and the greedy / gravity / optimal plan
// logic. The shared skeleton and behavior live in PlannerGui.
// ----------------------------------------------
#include "mainwindow.hpp"

#include "gravity.hpp"
#include "greedy.hpp"

#include <QtConcurrent/QtConcurrent>

#include <QComboBox>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>

#include <cstdint>
#include <exception>
#include <limits>
#include <utility>
#include <vector>

namespace VINCP::Network {

  MainWindow::MainWindow(QWidget* parent)
    : PlannerGui(Texts{.windowTitle = "Flow-plan instance viewer",
                       .nearestLinksTip =
                           "Draw an orange link from each node to its k "
                           "cheapest neighbours by (c_ij + c_ji)/2.",
                       .histogramTitle = "Cost histogram",
                       .histogramBinsTip =
                           "Number of equal cost-range bins for the N^2 costs.",
                       .throughputHeader = "By tonnage"},
                 parent)
  {
    // The plan radios beyond the base "Closest".
    greedyRadio_ = new QRadioButton("Greedy Plan", this);
    gravityRadio_ = new QRadioButton("Gravity Plan", this);
    optimalRadio_ = new QRadioButton("Optimal Plan", this);
    optimalRadio_->setToolTip("Solve the flow-planning QP (engine ipm + flow "
                              "Newton, keep-all) on a worker thread and overlay "
                              "the optimal flows.");
    addPlanRadio(greedyRadio_);
    addPlanRadio(gravityRadio_);
    addPlanRadio(optimalRadio_);

    // Swap (2-exchange) actions on the greedy plan. Right-click a node on the
    // map for its best local swap; the buttons do the global / iterated forms.
    QLabel* swapHint = new QLabel(
        "right-click a node: best swap\nshift+right-click: node to optimum", this);
    swapHint->setWordWrap(true);
    swapBestButton_ = new QPushButton("Best swap", this);
    swapOptButton_ = new QPushButton("Swap to optimum", this);
    swapResetButton_ = new QPushButton("Reset plan", this);
    purifyButton_ = new QPushButton("Purify (sparsify)", this);
    purifyButton_->setToolTip(
        "Swap-as-pivot crossover (F4): improving 2-exchanges, then arc-count-"
        "reducing ones within the ton-mile budget. Deliveries (and theta) are "
        "invariant; only the routing consolidates. Runs on a worker thread.");
    addSwapControl(swapHint);
    addSwapControl(swapBestButton_);
    addSwapControl(swapOptButton_);
    addSwapControl(swapResetButton_);
    addSwapControl(purifyButton_);

    // The optimal-plan solve reports back through a future watcher, so the
    // result lands on the GUI thread.
    optimalWatcher_ = new QFutureWatcher<OptimalOutcome>(this);
    connect(optimalWatcher_, &QFutureWatcher<OptimalOutcome>::finished, this,
            &MainWindow::onOptimalFinished);

    // Purification reports back the same way.
    purifyWatcher_ = new QFutureWatcher<PurifyOutcome>(this);
    connect(purifyWatcher_, &QFutureWatcher<PurifyOutcome>::finished, this,
            &MainWindow::onPurifyFinished);

    // Swap actions: right-click on the map, and the three buttons.
    connect(view_, &FlowPlanView::nodeSwapRequested, this,
            &MainWindow::onNodeSwap);
    connect(view_, &FlowPlanView::nodeSwapToOptimumRequested, this,
            &MainWindow::onNodeSwapToOptimum);
    connect(swapBestButton_, &QPushButton::clicked, this,
            &MainWindow::onBestSwap);
    connect(swapOptButton_, &QPushButton::clicked, this,
            &MainWindow::onSwapToOptimum);
    connect(swapResetButton_, &QPushButton::clicked, this,
            &MainWindow::onResetSwaps);
    connect(purifyButton_, &QPushButton::clicked, this,
            &MainWindow::onPurify);

    regenerate();
    return;
  }

  void
  MainWindow::rebuildInstance(std::uint64_t seed)
  {
    const InstanceProfile profile = geometryProfile();
    validateProfile(profile);
    current_ = makeRandomInstance(profile, seed);
    // A new instance forces a fresh plan on the next mode apply; the cached
    // optimal plan belongs to the old instance (the base's solveToken_ stamps
    // out any solve in flight).
    workingKind_ = 0;
    optimalValidP_ = false;
    return;
  }

  Index
  MainWindow::nodeCount() const
  {
    return current_.numNodes;
  }

  void
  MainWindow::refreshMap()
  {
    view_->setInstance(current_);
    histogram_->setInstance(current_);
    return;
  }

  void
  MainWindow::refreshStatus(const QString& modeNote)
  {
    statusLabel_->setText(
        QString("%1 nodes (laydown %2, seed %3) - %4")
            .arg(static_cast<int>(current_.numNodes))
            .arg(laydownBox_->currentData().toInt())
            .arg(static_cast<qulonglong>(lastSeed_))
            .arg(modeNote));
    return;
  }

  void
  MainWindow::applyPlanMode()
  {
    if (!haveInstanceP_) {
      return;
    }
    // The count spinner belongs to Closest mode; disable it otherwise. The
    // overlays are mutually exclusive.
    nearestBox_->setEnabled(closestRadio_->isChecked());
    if (greedyRadio_->isChecked()) {
      view_->setNearestK(0);
      try {
        if (1 != workingKind_) {
          // Compute the fresh greedy plan once; swaps then mutate this copy.
          const GreedyResult result = greedyPlan(current_);
          workingPlan_ = result.plan;
          greedyTargets_ = result.targets;
          baseTonMiles_ = result.tonMilesUsed;
          swapCount_ = 0;
          workingKind_ = 1;
          view_->clearSwapResult();
        }
        view_->setPlan(workingPlan_);
        showFlowLists(workingPlan_, current_.labels);
        refreshSwapControls();
        refreshPlanStatus();
      }
      catch (const std::exception& ex) {
        workingKind_ = 0;
        view_->clearPlan();
        hideFlowLists();
        setSwapControlsEnabled(false);
        refreshStatus(QString("greedy failed: %1").arg(ex.what()));
      }
    }
    else if (gravityRadio_->isChecked()) {
      // The gravity plan is dense and cost-blind: view it, but no swaps -- a
      // swap pass over its ~(sources x sinks) arcs is infeasibly slow.
      view_->setNearestK(0);
      view_->clearSwapResult();
      setSwapControlsEnabled(false);
      if (2 != workingKind_) {
        workingPlan_ = gravityPlan(current_);
        baseTonMiles_ = tonMiles(current_, workingPlan_);
        swapCount_ = 0;
        workingKind_ = 2;
      }
      view_->setPlan(workingPlan_);
      showFlowLists(workingPlan_, current_.labels);
      refreshPlanStatus();
    }
    else if (optimalRadio_->isChecked()) {
      // The optimal plan is solver output: theta is already optimal, but the
      // FLOWS are the analytic center of the optimal face -- full of tiny
      // shipments -- so purification (and Reset back to the pristine solve)
      // applies. Like the greedy branch, the working copy persists across
      // mode toggles; only a fresh instance or Reset re-copies the cache.
      view_->setNearestK(0);
      view_->clearSwapResult();
      if (optimalValidP_) {
        if (3 != workingKind_) {
          workingPlan_ = optimalResult_.plan;
          baseTonMiles_ = tonMiles(current_, workingPlan_);
          swapCount_ = 0;
          workingKind_ = 3;
        }
        view_->setPlan(workingPlan_);
        showFlowLists(workingPlan_, current_.labels);
        refreshSwapControls();
        refreshPlanStatus();
      }
      else if (!optimalBusyP_) {
        setSwapControlsEnabled(false);
        view_->clearPlan();
        hideFlowLists();
        startOptimalSolve();
      }
      else {
        setSwapControlsEnabled(false);
        refreshStatus("solving optimal plan...");
      }
    }
    else if (closestRadio_->isChecked()) {
      setSwapControlsEnabled(false);
      view_->clearSwapResult();
      view_->clearPlan();
      hideFlowLists();
      applyNearestK();
    }
    else {   // None
      setSwapControlsEnabled(false);
      view_->clearSwapResult();
      view_->clearPlan();
      hideFlowLists();
      view_->setNearestK(0);
      refreshStatus("no links");
    }
    return;
  }

  void
  MainWindow::startOptimalSolve()
  {
    optimalBusyP_ = true;
    startBusy();
    refreshStatus("solving optimal plan (ipm + flow Newton, keep-all)...");

    // Everything the worker needs is captured BY VALUE (the instance, the
    // token); it touches no widget. The greedy pre-pass calibrates the
    // ton-mile budget exactly as the benchmark pipeline does (~80% of greedy
    // usage), because a generated instance arrives uncalibrated.
    Instance inst = current_;
    const int token = solveToken_;
    optimalWatcher_->setFuture(QtConcurrent::run([inst, token]() mutable {
      OptimalOutcome out;
      out.token = token;
      try {
        const GreedyResult greedy = greedyPlan(inst);
        inst.tonMileLimit = greedy.suggestedLimit;
        out.budgetUsed = greedy.suggestedLimit;

        FlowPlanParams params;
        params.engine = "ipm";        // banded/large production default:
        params.ipmNewton = "flow";    //   structured Newton, keep-all,
        params.iterMax = 200;         //   nothing screened out to certify
        out.result = solveFlowPlan(inst, params);
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
  MainWindow::onOptimalFinished()
  {
    optimalBusyP_ = false;
    stopBusy();
    const OptimalOutcome out = optimalWatcher_->result();
    if (out.token != solveToken_) {
      return;   // stale: the instance changed while the solve ran
    }
    if (!out.okP) {
      refreshStatus(QString("optimal solve failed: %1").arg(out.error));
      return;
    }
    optimalResult_ = out.result;
    optimalBudget_ = out.budgetUsed;
    optimalValidP_ = true;
    if (optimalRadio_->isChecked()) {
      applyPlanMode();   // shows the now-cached plan
    }
    return;
  }

  void
  MainWindow::refreshPlanStatus()
  {
    // The "objective" values are the weighted quadratic shortfall
    // theta = sum P_i ((target_i - R_i)/D_i)^2 -- a normalized, DIMENSIONLESS
    // score, NOT tons. "delivered / unmet" is the raw ton gap (~ D - C).
    const bool greedyP = (1 == workingKind_);
    const bool optimalP = (3 == workingKind_);
    const double tm = tonMiles(current_, workingPlan_);
    const double totC = totalSupplyCap(current_);
    const double totD = totalDemand(current_);
    const double delivered = workingPlan_.resupply.sum();
    const double unmet = totD - delivered;
    const double originalShort = shortfallObjective(current_, workingPlan_);

    QString head;
    if (optimalP) {
      head = QString("optimal plan (%1%2)")
                 .arg(optimalResult_.certifiedP ? "certified" : "NOT certified")
                 .arg(optimalResult_.vi.converged ? "" : ", NOT converged");
    }
    else {
      head = greedyP ? QString("greedy plan") : QString("gravity plan");
    }
    if ((greedyP || optimalP) && swapCount_ > 0) {
      head += QString(" (+%1 pivots, saved %2 t-mi)")
                  .arg(swapCount_)
                  .arg(baseTonMiles_ - tm, 0, 'g', 4);
    }
    QString body = QString("\nton-miles: %1\n"
                           "arcs: %2\n"
                           "supply / demand: %3 / %4 t\n"
                           "delivered / unmet: %5 / %6 t")
                       .arg(tm, 0, 'g', 4)
                       .arg(positiveArcCount(workingPlan_))
                       .arg(totC, 0, 'f', 0)
                       .arg(totD, 0, 'f', 0)
                       .arg(delivered, 0, 'f', 0)
                       .arg(unmet, 0, 'f', 0);
    if (greedyP) {
      const double rationedShort =
          shortfallVsTarget(current_, greedyTargets_, workingPlan_.resupply);
      body += QString("\nobjective vs rationed: %1").arg(rationedShort, 0, 'g', 4);
    }
    body += QString("\nobjective vs original: %1").arg(originalShort, 0, 'g', 4);
    if (optimalP) {
      body += QString("\ntheta* %1, budget %2 t-mi, lambda %3")
                  .arg(optimalResult_.shortfall, 0, 'g', 6)
                  .arg(optimalBudget_, 0, 'g', 4)
                  .arg(optimalResult_.budgetShadowPrice, 0, 'g', 3);
    }
    refreshStatus(head + body);
    return;
  }

  void
  MainWindow::setSwapControlsEnabled(bool onP)
  {
    swapBestButton_->setEnabled(onP);
    swapOptButton_->setEnabled(onP);
    swapResetButton_->setEnabled(onP);
    purifyButton_->setEnabled(onP);
    return;
  }

  void
  MainWindow::refreshSwapControls()
  {
    if (purifyBusyP_) {
      setSwapControlsEnabled(false);   // nothing until the worker reports back
      return;
    }
    if (1 == workingKind_) {
      setSwapControlsEnabled(true);    // greedy: the full swap toolkit
    }
    else if (3 == workingKind_) {
      // Optimal: interactive swaps are pointless (theta is optimal and the
      // 2-exchange cannot change theta), but purification and the reset back
      // to the pristine cached solve both apply.
      swapBestButton_->setEnabled(false);
      swapOptButton_->setEnabled(false);
      swapResetButton_->setEnabled(true);
      purifyButton_->setEnabled(true);
    }
    else {
      setSwapControlsEnabled(false);
    }
    return;
  }

  QStringList
  MainWindow::buildSwapLines(const SwapMove& move) const
  {
    const auto nm = [this](Index x) {
      return QString::fromStdString(nodeLabel(current_, x));
    };
    return QStringList{
        QString("saved %1 t-mi").arg(move.saving, 0, 'g', 4),
        QString("moved %1 t").arg(move.amount, 0, 'f', 1),
        QString("- %1->%2, %3->%4").arg(nm(move.i), nm(move.j), nm(move.m),
                                        nm(move.n)),
        QString("+ %1->%2, %3->%4").arg(nm(move.i), nm(move.n), nm(move.m),
                                        nm(move.j)),
    };
  }

  void
  MainWindow::applyAndShowSwap(const SwapMove& move, int anchorNode)
  {
    if (move.improvingP) {
      applySwap(workingPlan_, move);
      ++swapCount_;
      view_->setPlan(workingPlan_);
      showFlowLists(workingPlan_, current_.labels);
      refreshPlanStatus();
      const vector<std::pair<int, int>> arcs = {
          {static_cast<int>(move.i), static_cast<int>(move.j)},
          {static_cast<int>(move.m), static_cast<int>(move.n)},
          {static_cast<int>(move.i), static_cast<int>(move.n)},
          {static_cast<int>(move.m), static_cast<int>(move.j)},
      };
      view_->showSwapResult(anchorNode, buildSwapLines(move), arcs);
    }
    else {
      view_->showSwapResult(anchorNode, QStringList{"no improving swap"}, {});
    }
    return;
  }

  void
  MainWindow::onNodeSwap(int node)
  {
    // Right-click swaps apply only to the displayed greedy plan (gravity is too
    // dense to swap interactively).
    if (!greedyRadio_->isChecked() || 1 != workingKind_) {
      return;
    }
    applyAndShowSwap(bestSwapAtNode(current_, workingPlan_, node), node);
    return;
  }

  void
  MainWindow::onNodeSwapToOptimum(int node)
  {
    if (!greedyRadio_->isChecked() || 1 != workingKind_) {
      return;
    }
    const SwapSummary summary =
        swapNodeToLocalOptimum(current_, workingPlan_, node);
    const QString name = QString::fromStdString(nodeLabel(current_, node));
    if (summary.swaps > 0) {
      swapCount_ += summary.swaps;
      view_->setPlan(workingPlan_);
      showFlowLists(workingPlan_, current_.labels);
      refreshPlanStatus();
      view_->showSwapResult(
          node,
          QStringList{QString("%1: %2 swaps").arg(name).arg(summary.swaps),
                      QString("saved %1 t-mi").arg(summary.totalSaving, 0, 'g',
                                                   4)},
          {});
    }
    else {
      view_->showSwapResult(node,
                            QStringList{QString("%1: no improving swap").arg(name)},
                            {});
    }
    return;
  }

  void
  MainWindow::onBestSwap()
  {
    if (1 != workingKind_) {
      return;
    }
    const SwapMove move = bestSwap(current_, workingPlan_);
    applyAndShowSwap(move, move.improvingP ? static_cast<int>(move.i) : -1);
    return;
  }

  void
  MainWindow::onSwapToOptimum()
  {
    if (1 != workingKind_) {
      return;
    }
    const SwapSummary summary = swapToLocalOptimum(current_, workingPlan_);
    swapCount_ += summary.swaps;
    view_->setPlan(workingPlan_);
    showFlowLists(workingPlan_, current_.labels);
    refreshPlanStatus();
    const QStringList lines =
        (summary.swaps > 0)
            ? QStringList{QString("%1 swaps applied").arg(summary.swaps),
                          QString("saved %1 t-mi").arg(summary.totalSaving, 0,
                                                       'g', 4)}
            : QStringList{"already at a local optimum"};
    view_->showSwapResult(-1, lines, {});
    return;
  }

  void
  MainWindow::onResetSwaps()
  {
    workingKind_ = 0;   // force a fresh plan (greedy or gravity) on the next mode
    view_->clearSwapResult();
    applyPlanMode();
    return;
  }

  void
  MainWindow::onPurify()
  {
    if (purifyBusyP_ || (1 != workingKind_ && 3 != workingKind_)) {
      return;
    }
    // Purify a COPY off the UI thread: the spread optimal plan can carry
    // thousands of positive arcs and each pivot rescans O(arcs^2) pairs. The
    // greedy plan is uncapped (it ignores the budget by design); the optimal
    // plan must stay within the budget its solve was run with.
    purifyBusyP_ = true;
    startBusy();
    refreshSwapControls();
    refreshStatus("purifying plan (swap pivots)...");

    Instance inst = current_;
    Plan plan = workingPlan_;
    const double cap = (3 == workingKind_)
                           ? optimalBudget_
                           : std::numeric_limits<double>::infinity();
    const int token = solveToken_;
    const int kind = workingKind_;
    purifyWatcher_->setFuture(QtConcurrent::run([inst, plan, cap, token,
                                                 kind]() mutable {
      PurifyOutcome out;
      out.summary = purifyPlan(inst, plan, cap);
      out.plan = std::move(plan);
      out.token = token;
      out.kind = kind;
      return out;
    }));
    return;
  }

  void
  MainWindow::onPurifyFinished()
  {
    purifyBusyP_ = false;
    stopBusy();
    const PurifyOutcome out = purifyWatcher_->result();
    if (out.token != solveToken_ || out.kind != workingKind_) {
      // Stale: the instance or the displayed plan kind changed mid-run.
      // Restore whatever button states the current mode wants and move on.
      refreshSwapControls();
      return;
    }
    workingPlan_ = out.plan;
    const int pivots =
        out.summary.improvingSwaps + out.summary.consolidatingSwaps;
    swapCount_ += pivots;
    view_->setPlan(workingPlan_);
    showFlowLists(workingPlan_, current_.labels);
    refreshSwapControls();
    refreshPlanStatus();
    const QStringList lines =
        (pivots > 0)
            ? QStringList{QString("arcs %1 -> %2")
                              .arg(out.summary.arcsBefore)
                              .arg(out.summary.arcsAfter),
                          QString("%1 improving + %2 consolidating pivots")
                              .arg(out.summary.improvingSwaps)
                              .arg(out.summary.consolidatingSwaps),
                          QString("net saved %1 t-mi")
                              .arg(out.summary.totalSaving, 0, 'g', 4)}
            : QStringList{"already purified"};
    view_->showSwapResult(-1, lines, {});
    return;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
