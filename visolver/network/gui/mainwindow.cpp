// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// MainWindow implementation: builds the control panel, regenerates instances on
// demand via makeRandomInstance (seed 0 draws a fast-varying time-based seed),
// and drives the placement / greedy-plan display mode.
// ----------------------------------------------
#include "mainwindow.hpp"

#include "gravity.hpp"
#include "greedy.hpp"

#include <QtConcurrent/QtConcurrent>

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
#include <limits>
#include <utility>
#include <vector>

namespace VINCP::Network {

  namespace {
    const int kDefaultSeed = 20260704;
    const int kMaxClassCount = 400;   // per node class in the spin controls
    const int kMaxSeed = 2147483647;  // QSpinBox int ceiling (2^31 - 1)

    // A fast-varying seed for the "type 0 to reroll" convenience: the low 31
    // bits of the microseconds since the Unix epoch (masked to fit a QSpinBox's
    // int range, so the exact value used is shown back to the user to copy).
    // Precision is unimportant here -- only that it changes rapidly.
    int
    timeSeed()
    {
      const auto since = std::chrono::system_clock::now().time_since_epoch();
      const auto micros =
          std::chrono::duration_cast<std::chrono::microseconds>(since).count();
      return static_cast<int>(static_cast<std::uint64_t>(micros) & 0x7FFFFFFFULL);
    }

    // Per-node flow activity in a plan, over links to/from OTHER nodes (the
    // self-supply diagonal f_ii is excluded: it does not flow through the node).
    struct NodeFlowStat
    {
      Index node = 0;
      double throughput = 0.0;   // sum of |flows| to and from the node (tons)
      int count = 0;             // number of nonzero flows to and from the node
    };

    vector<NodeFlowStat>
    computeNodeFlowStats(const Plan& plan)
    {
      const Index m = plan.flow.rows();
      vector<NodeFlowStat> stats;
      stats.reserve(static_cast<size_t>(m));
      for (Index i = 0; i < m; ++i) {
        NodeFlowStat s;
        s.node = i;
        for (Index j = 0; j < m; ++j) {
          if (i != j) {
            const double outFlow = plan.flow(i, j);   // i -> j
            const double inFlow = plan.flow(j, i);    // j -> i
            s.throughput += outFlow + inFlow;
            if (outFlow > 0.0) {
              s.count += 1;
            }
            if (inFlow > 0.0) {
              s.count += 1;
            }
          }
        }
        stats.push_back(s);
      }
      return stats;
    }

    // Sort a copy of the stats by throughput (byThroughputP) or by count, then
    // fill the list with "label   value" rows carrying the node id in UserRole.
    void
    fillNodeList(NodeListWidget* list, const Instance& inst,
                 vector<NodeFlowStat> stats, bool byThroughputP)
    {
      std::sort(stats.begin(), stats.end(),
                [byThroughputP](const NodeFlowStat& a, const NodeFlowStat& b) {
                  if (byThroughputP) {
                    return a.throughput > b.throughput;
                  }
                  return a.count > b.count;
                });
      list->clear();
      for (const NodeFlowStat& s : stats) {
        const QString name = QString::fromStdString(nodeLabel(inst, s.node));
        const QString value = byThroughputP
                                  ? QString::number(s.throughput, 'f', 1)
                                  : QString::number(s.count);
        QListWidgetItem* item =
            new QListWidgetItem(QString("%1   %2").arg(name, value));
        item->setData(Qt::UserRole, static_cast<int>(s.node));
        list->addItem(item);
      }
      return;
    }
  } // namespace

  MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
  {
    setWindowTitle("Flow-plan instance viewer");

    view_ = new FlowPlanView(this);

    laydownBox_ = new QComboBox(this);
    laydownBox_->addItem("0 - random square", 0);
    laydownBox_->addItem("1 - banded", 1);

    seedBox_ = new QSpinBox(this);
    seedBox_->setRange(0, kMaxSeed);
    seedBox_->setValue(kDefaultSeed);
    seedBox_->setToolTip("Seed for the generator. Type 0 and Regenerate to draw "
                         "a fresh time-based seed (shown here to copy).");

    supplyOnlyBox_ = new QSpinBox(this);
    supplyOnlyBox_->setRange(0, kMaxClassCount);
    supplyOnlyBox_->setValue(20);

    bothBox_ = new QSpinBox(this);
    bothBox_->setRange(0, kMaxClassCount);
    bothBox_->setValue(20);

    demandOnlyBox_ = new QSpinBox(this);
    demandOnlyBox_->setRange(0, kMaxClassCount);
    demandOnlyBox_->setValue(30);

    transitBox_ = new QSpinBox(this);
    transitBox_->setRange(0, kMaxClassCount);
    transitBox_->setValue(0);

    nearestBox_ = new QSpinBox(this);
    // Constructed wide so the initial value below survives (a [0, 0] range
    // would clamp it to 0); regenerate() tightens the max to numNodes - 1.
    nearestBox_->setRange(0, kMaxClassCount);
    nearestBox_->setValue(5);
    nearestBox_->setToolTip("Draw an orange link from each node to its k "
                            "cheapest neighbours by (c_ij + c_ji)/2.");

    // Mutually exclusive link overlay: none / closest-links / greedy-plan.
    // (The Laydown menu already selects the placement kind, so placement itself
    // is not a mode here.) A QButtonGroup gives a single signal per selection.
    //noneRadio_ = new QRadioButton("None", this);
    closestRadio_ = new QRadioButton("Closest", this);
    greedyRadio_ = new QRadioButton("Greedy Plan", this);
    gravityRadio_ = new QRadioButton("Gravity Plan", this);
    optimalRadio_ = new QRadioButton("Optimal Plan", this);
    optimalRadio_->setToolTip("Solve the flow-planning QP (engine ipm + flow "
                              "Newton, keep-all) on a worker thread and overlay "
                              "the optimal flows.");
    closestRadio_->setChecked(true);
    QButtonGroup* linkGroup = new QButtonGroup(this);
    //linkGroup->addButton(noneRadio_);
    linkGroup->addButton(closestRadio_);
    linkGroup->addButton(greedyRadio_);
    linkGroup->addButton(gravityRadio_);
    linkGroup->addButton(optimalRadio_);

    QPushButton* regenButton = new QPushButton("Regenerate", this);
    QPushButton* recenterButton = new QPushButton("Recenter", this);
    recenterButton->setToolTip("Restore the centred, fitted view "
                               "(undo pan / zoom).");

    statusLabel_ = new QLabel(this);
    statusLabel_->setWordWrap(true);

    // --- control panel layout ---
    QGroupBox* genGroup = new QGroupBox("Instance", this);
    QFormLayout* genForm = new QFormLayout(genGroup);
    genForm->addRow("Laydown", laydownBox_);
    genForm->addRow("Seed", seedBox_);
    genForm->addRow("Supply-only", supplyOnlyBox_);
    genForm->addRow("Both", bothBox_);
    genForm->addRow("Demand-only", demandOnlyBox_);
    genForm->addRow("Transit", transitBox_);
    genForm->addRow(regenButton);

    // The closest-links count sits directly under its "Closest" radio, indented.
    QGroupBox* modeGroup = new QGroupBox("Show Links", this);
    QVBoxLayout* modeLayout = new QVBoxLayout(modeGroup);
    //modeLayout->addWidget(noneRadio_);
    modeLayout->addWidget(closestRadio_);
    QHBoxLayout* countRow = new QHBoxLayout();
    countRow->addSpacing(20);
    countRow->addWidget(new QLabel("links", this));
    countRow->addWidget(nearestBox_);
    countRow->addStretch(1);
    modeLayout->addLayout(countRow);
    modeLayout->addWidget(greedyRadio_);
    modeLayout->addWidget(gravityRadio_);
    modeLayout->addWidget(optimalRadio_);

    // Swap (2-exchange) actions on the greedy plan. Right-click a node on the
    // map for its best local swap; the buttons do the global / iterated forms.
    swapBestButton_ = new QPushButton("Best swap", this);
    swapOptButton_ = new QPushButton("Swap to optimum", this);
    swapResetButton_ = new QPushButton("Reset plan", this);
    purifyButton_ = new QPushButton("Purify (sparsify)", this);
    purifyButton_->setToolTip(
        "Swap-as-pivot crossover (F4): improving 2-exchanges, then arc-count-"
        "reducing ones within the ton-mile budget. Deliveries (and theta) are "
        "invariant; only the routing consolidates. Runs on a worker thread.");
    QGroupBox* swapGroup = new QGroupBox("Swaps (2-exchange)", this);
    QVBoxLayout* swapLayout = new QVBoxLayout(swapGroup);
    QLabel* swapHint = new QLabel(
        "right-click a node: best swap\nshift+right-click: node to optimum", this);
    swapHint->setWordWrap(true);
    swapLayout->addWidget(swapHint);
    swapLayout->addWidget(swapBestButton_);
    swapLayout->addWidget(swapOptButton_);
    swapLayout->addWidget(swapResetButton_);
    swapLayout->addWidget(purifyButton_);

    // Busy bar: background work (the optimal solve, purification) has no
    // knowable duration, so the bar just refills over and over until the
    // running operation reports back; idle, it sits empty.
    busyBar_ = new QProgressBar(this);
    busyBar_->setRange(0, 100);
    busyBar_->setValue(0);
    busyBar_->setTextVisible(false);
    busyBar_->setFixedHeight(12);
    busyTimer_ = new QTimer(this);
    busyTimer_->setInterval(50);
    connect(busyTimer_, &QTimer::timeout, this, [this]() {
      busyBar_->setValue((busyBar_->value() + 4) % (busyBar_->maximum() + 1));
    });

    // Cost histogram: all N^2 costs in K equal-width bins, K live via a spinner.
    histBinsBox_ = new QSpinBox(this);
    histBinsBox_->setRange(1, 25);
    histBinsBox_->setValue(10);
    histBinsBox_->setToolTip("Number of equal cost-range bins for the N^2 "
                             "costs.");
    histogram_ = new CostHistogram(this);

    QGroupBox* histGroup = new QGroupBox("Cost histogram", this);
    QVBoxLayout* histLayout = new QVBoxLayout(histGroup);
    QHBoxLayout* binsRow = new QHBoxLayout();
    binsRow->addWidget(new QLabel("bins", this));
    binsRow->addWidget(histBinsBox_);
    binsRow->addStretch(1);
    histLayout->addLayout(binsRow);
    histLayout->addWidget(histogram_);

    labelsCheck_ = new QCheckBox("Labels", this);
    labelsCheck_->setChecked(false);
    labelsCheck_->setToolTip("Show each node's label centred on its dot.");

    QGroupBox* dispGroup = new QGroupBox("Display", this);
    QVBoxLayout* dispLayout = new QVBoxLayout(dispGroup);
    dispLayout->addWidget(labelsCheck_);
    dispLayout->addWidget(recenterButton);

    QWidget* panel = new QWidget(this);
    QVBoxLayout* panelLayout = new QVBoxLayout(panel);
    panelLayout->addWidget(genGroup);
    panelLayout->addWidget(modeGroup);
    panelLayout->addWidget(swapGroup);
    panelLayout->addWidget(busyBar_);
    panelLayout->addWidget(dispGroup);
    panelLayout->addWidget(histGroup);
    panelLayout->addWidget(statusLabel_);
    panelLayout->addStretch(1);
    panel->setFixedWidth(240);

    // Right panel: two node rankings by plan flow, side by side. Greyed out
    // (disabled) until a plan is shown; clicking an item pops the node's info
    // box on the map, exactly like clicking the node.
    throughputList_ = new NodeListWidget(this);
    countList_ = new NodeListWidget(this);
    throughputList_->setEnabled(false);
    countList_->setEnabled(false);

    QVBoxLayout* tputCol = new QVBoxLayout();
    tputCol->addWidget(new QLabel("By tonnage", this));
    tputCol->addWidget(throughputList_);
    QVBoxLayout* countCol = new QVBoxLayout();
    countCol->addWidget(new QLabel("By count", this));
    countCol->addWidget(countList_);

    QGroupBox* rankGroup = new QGroupBox("Flow through nodes", this);
    QHBoxLayout* rankLayout = new QHBoxLayout(rankGroup);
    rankLayout->addLayout(tputCol);
    rankLayout->addLayout(countCol);
    QWidget* rightPanel = new QWidget(this);
    QVBoxLayout* rightLayout = new QVBoxLayout(rightPanel);
    rightLayout->addWidget(rankGroup);
    rightPanel->setFixedWidth(260);

    QWidget* central = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->addWidget(panel);
    mainLayout->addWidget(view_, 1);
    mainLayout->addWidget(rightPanel);
    setCentralWidget(central);

    // Regeneration is explicit (button) or on a structural control change; the
    // closest-links spin restyles the current instance; the mode radio overlays
    // or hides the greedy plan; recenter is view-only. Connecting only the
    // greedy radio's toggle fires applyPlanMode exactly once per mode switch.
    connect(regenButton, &QPushButton::clicked, this, &MainWindow::regenerate);
    connect(laydownBox_, &QComboBox::currentIndexChanged, this,
            &MainWindow::regenerate);
    connect(nearestBox_,
            QOverload<int>::of(&QSpinBox::valueChanged), this,
            &MainWindow::applyNearestK);
    connect(linkGroup, &QButtonGroup::buttonClicked, this,
            &MainWindow::applyPlanMode);
    connect(histBinsBox_,
            QOverload<int>::of(&QSpinBox::valueChanged), histogram_,
            &CostHistogram::setBinCount);
    connect(recenterButton, &QPushButton::clicked, view_,
            &FlowPlanView::recenter);
    connect(labelsCheck_, &QCheckBox::toggled, view_,
            &FlowPlanView::setShowLabels);

    // A press on either list pops the same node-info box on the map; release
    // dismisses it -- mirroring the map's press-and-hold behaviour.
    for (NodeListWidget* list : {throughputList_, countList_}) {
      connect(list, &NodeListWidget::nodePressed, view_,
              &FlowPlanView::showNodeInfo);
      connect(list, &NodeListWidget::pressEnded, view_,
              &FlowPlanView::hideNodeInfo);
    }

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

  InstanceProfile
  MainWindow::currentProfile() const
  {
    InstanceProfile profile;
    profile.numSupplyOnly = supplyOnlyBox_->value();
    profile.numBoth = bothBox_->value();
    profile.numDemandOnly = demandOnlyBox_->value();
    profile.numNeither = transitBox_->value();
    profile.laydownType = laydownBox_->currentData().toInt();
    return profile;
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
  MainWindow::regenerate()
  {
    const InstanceProfile profile = currentProfile();
    // Seed 0 is the "reroll" sentinel: draw a fresh time-based seed and write it
    // back so the field shows exactly what was used (the user can copy it).
    if (0 == seedBox_->value()) {
      seedBox_->setValue(timeSeed());
    }
    const std::uint64_t seed =
        static_cast<std::uint64_t>(seedBox_->value());
    try {
      validateProfile(profile);
      current_ = makeRandomInstance(profile, seed);
      lastSeed_ = seed;
      haveInstanceP_ = true;
      workingKind_ = 0;   // a new instance forces a fresh plan on the next mode
      optimalValidP_ = false;   // the cached optimal plan belongs to the old
      ++solveToken_;            //   instance; stamp out any solve in flight
      view_->setInstance(current_);
      histogram_->setInstance(current_);

      const int hi = std::max(0, static_cast<int>(current_.numNodes) - 1);
      nearestBox_->setRange(0, hi);

      applyPlanMode();   // overlays greedy / closest links / none per the radio
    }
    catch (const std::exception& ex) {
      haveInstanceP_ = false;
      view_->clearPlan();
      statusLabel_->setText(QString("Invalid profile: %1").arg(ex.what()));
    }
    return;
  }

  void
  MainWindow::applyNearestK()
  {
    // The count belongs to "Closest" mode; inert (and disabled) in None/Greedy.
    if (!haveInstanceP_ || !closestRadio_->isChecked()) {
      return;
    }
    view_->setNearestK(nearestBox_->value());
    refreshStatus(QString("closest links (k = %1)").arg(nearestBox_->value()));
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
        showFlowLists(workingPlan_);
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
      showFlowLists(workingPlan_);
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
        showFlowLists(workingPlan_);
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
      view_->setNearestK(nearestBox_->value());
      refreshStatus(QString("closest links (k = %1)").arg(nearestBox_->value()));
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
  MainWindow::startBusy()
  {
    ++busyCount_;
    if (!busyTimer_->isActive()) {
      busyBar_->setValue(0);
      busyTimer_->start();
    }
    return;
  }

  void
  MainWindow::stopBusy()
  {
    busyCount_ = std::max(0, busyCount_ - 1);
    if (0 == busyCount_) {
      busyTimer_->stop();
      busyBar_->setValue(0);
    }
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
      showFlowLists(workingPlan_);
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
      showFlowLists(workingPlan_);
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
    showFlowLists(workingPlan_);
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
    showFlowLists(workingPlan_);
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

  void
  MainWindow::showFlowLists(const Plan& plan)
  {
    const vector<NodeFlowStat> stats = computeNodeFlowStats(plan);
    fillNodeList(throughputList_, current_, stats, true);
    fillNodeList(countList_, current_, stats, false);
    throughputList_->setEnabled(true);
    countList_->setEnabled(true);
    return;
  }

  void
  MainWindow::hideFlowLists()
  {
    throughputList_->clear();
    countList_->clear();
    throughputList_->setEnabled(false);
    countList_->setEnabled(false);
    return;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
