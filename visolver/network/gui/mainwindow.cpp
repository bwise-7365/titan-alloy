// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// MainWindow implementation: builds the control panel, regenerates instances on
// demand via makeRandomInstance (seed 0 draws a fast-varying time-based seed),
// and drives the placement / greedy-plan display mode.
// ----------------------------------------------
#include "mainwindow.hpp"

#include "greedy.hpp"

#include <QButtonGroup>
#include <QCheckBox>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>
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
    nearestBox_->setRange(0, 0);
    nearestBox_->setValue(0);
    nearestBox_->setToolTip("Draw an orange link from each node to its k "
                            "cheapest neighbours by (c_ij + c_ji)/2.");

    // Mutually exclusive link overlay: none / closest-links / greedy-plan.
    // (The Laydown menu already selects the placement kind, so placement itself
    // is not a mode here.) A QButtonGroup gives a single signal per selection.
    noneRadio_ = new QRadioButton("None", this);
    closestRadio_ = new QRadioButton("Closest", this);
    greedyRadio_ = new QRadioButton("Greedy Plan", this);
    noneRadio_->setChecked(true);
    QButtonGroup* linkGroup = new QButtonGroup(this);
    linkGroup->addButton(noneRadio_);
    linkGroup->addButton(closestRadio_);
    linkGroup->addButton(greedyRadio_);

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
    modeLayout->addWidget(noneRadio_);
    modeLayout->addWidget(closestRadio_);
    QHBoxLayout* countRow = new QHBoxLayout();
    countRow->addSpacing(20);
    countRow->addWidget(new QLabel("links", this));
    countRow->addWidget(nearestBox_);
    countRow->addStretch(1);
    modeLayout->addLayout(countRow);
    modeLayout->addWidget(greedyRadio_);

    // Swap (2-exchange) actions on the greedy plan. Right-click a node on the
    // map for its best local swap; the buttons do the global / iterated forms.
    swapBestButton_ = new QPushButton("Best swap", this);
    swapOptButton_ = new QPushButton("Swap to optimum", this);
    swapResetButton_ = new QPushButton("Reset plan", this);
    QGroupBox* swapGroup = new QGroupBox("Swaps (2-exchange)", this);
    QVBoxLayout* swapLayout = new QVBoxLayout(swapGroup);
    QLabel* swapHint = new QLabel(
        "right-click a node: best swap\nshift+right-click: node to optimum", this);
    swapHint->setWordWrap(true);
    swapLayout->addWidget(swapHint);
    swapLayout->addWidget(swapBestButton_);
    swapLayout->addWidget(swapOptButton_);
    swapLayout->addWidget(swapResetButton_);

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
      workingPlanValidP_ = false;   // a new instance forces a fresh greedy plan
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
        if (!workingPlanValidP_) {
          // Compute the fresh greedy plan once; swaps then mutate this copy.
          const GreedyResult result = greedyPlan(current_);
          workingPlan_ = result.plan;
          greedyTargets_ = result.targets;
          baseTonMiles_ = result.tonMilesUsed;
          swapCount_ = 0;
          workingPlanValidP_ = true;
        }
        view_->setPlan(workingPlan_);
        showFlowLists(workingPlan_);
        setSwapControlsEnabled(true);
        refreshGreedyStatus();
      }
      catch (const std::exception& ex) {
        workingPlanValidP_ = false;
        view_->clearPlan();
        hideFlowLists();
        setSwapControlsEnabled(false);
        refreshStatus(QString("greedy failed: %1").arg(ex.what()));
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
  MainWindow::refreshGreedyStatus()
  {
    // The "objective" values are the weighted quadratic shortfall
    // theta = sum P_i ((target_i - R_i)/D_i)^2 -- a normalized, DIMENSIONLESS
    // score, NOT tons. "delivered / unmet" is the raw ton gap (~ D - C).
    const double tm = tonMiles(current_, workingPlan_);
    const double totC = totalSupplyCap(current_);
    const double totD = totalDemand(current_);
    const double delivered = workingPlan_.resupply.sum();
    const double unmet = totD - delivered;
    const double saved = baseTonMiles_ - tm;
    const double rationedShort =
        shortfallVsTarget(current_, greedyTargets_, workingPlan_.resupply);
    const double originalShort = shortfallObjective(current_, workingPlan_);
    const QString head =
        (swapCount_ > 0)
            ? QString("greedy plan (+%1 swaps, saved %2 t-mi)")
                  .arg(swapCount_)
                  .arg(saved, 0, 'g', 4)
            : QString("greedy plan");
    refreshStatus(head
                  + QString("\nton-miles: %1\n"
                            "supply / demand: %2 / %3 t\n"
                            "delivered / unmet: %4 / %5 t\n"
                            "objective vs rationed: %6\n"
                            "objective vs original: %7")
                        .arg(tm, 0, 'g', 4)
                        .arg(totC, 0, 'f', 0)
                        .arg(totD, 0, 'f', 0)
                        .arg(delivered, 0, 'f', 0)
                        .arg(unmet, 0, 'f', 0)
                        .arg(rationedShort, 0, 'g', 4)
                        .arg(originalShort, 0, 'g', 4));
    return;
  }

  void
  MainWindow::setSwapControlsEnabled(bool onP)
  {
    swapBestButton_->setEnabled(onP);
    swapOptButton_->setEnabled(onP);
    swapResetButton_->setEnabled(onP);
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
      refreshGreedyStatus();
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
    if (!workingPlanValidP_ || !greedyRadio_->isChecked()) {
      return;
    }
    applyAndShowSwap(bestSwapAtNode(current_, workingPlan_, node), node);
    return;
  }

  void
  MainWindow::onNodeSwapToOptimum(int node)
  {
    if (!workingPlanValidP_ || !greedyRadio_->isChecked()) {
      return;
    }
    const SwapSummary summary =
        swapNodeToLocalOptimum(current_, workingPlan_, node);
    const QString name = QString::fromStdString(nodeLabel(current_, node));
    if (summary.swaps > 0) {
      swapCount_ += summary.swaps;
      view_->setPlan(workingPlan_);
      showFlowLists(workingPlan_);
      refreshGreedyStatus();
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
    if (!workingPlanValidP_) {
      return;
    }
    const SwapMove move = bestSwap(current_, workingPlan_);
    applyAndShowSwap(move, move.improvingP ? static_cast<int>(move.i) : -1);
    return;
  }

  void
  MainWindow::onSwapToOptimum()
  {
    if (!workingPlanValidP_) {
      return;
    }
    const SwapSummary summary = swapToLocalOptimum(current_, workingPlan_);
    swapCount_ += summary.swaps;
    view_->setPlan(workingPlan_);
    showFlowLists(workingPlan_);
    refreshGreedyStatus();
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
    workingPlanValidP_ = false;   // force a fresh greedy on the next applyPlanMode
    view_->clearSwapResult();
    applyPlanMode();
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
