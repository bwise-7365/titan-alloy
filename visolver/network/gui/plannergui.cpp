// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// PlannerGui implementation: builds the widget skeleton shared by the network
// and fleet viewers and carries the behavior that is identical in both.
// ----------------------------------------------
#include "plannergui.hpp"

#include "utils.hpp"

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
#include <cstdint>
#include <exception>
#include <string>
#include <vector>

namespace VINCP::Network {

  namespace {
    const int kDefaultSeed = 20260704;
    const int kMaxClassCount = 400;   // per node class in the spin controls
    const int kMaxSeed = 2147483647;  // QSpinBox int ceiling (2^31 - 1)

    // A fast-varying seed for the "type 0 to reroll" convenience: the low 31
    // bits of the shared microsecond seed (masked to fit a QSpinBox's int
    // range, so the exact value used is shown back to the user to copy).
    int
    timeSeed()
    {
      return static_cast<int>(microsecondSeed() & 0x7FFFFFFFULL);
    }

    // Per-node flow activity in a plan, over links to/from OTHER nodes (the
    // self-supply diagonal f_ii is excluded: it does not flow through the node).
    struct NodeFlowStat
    {
      Index node = 0;
      double throughput = 0.0;   // sum of flows to and from the node
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
    fillNodeList(NodeListWidget* list, const vector<string>& labels,
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
        const QString name = nodeText(labels, s.node);
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

  QString
  nodeText(const std::vector<std::string>& labels, Index node)
  {
    if (0 <= node && node < static_cast<Index>(labels.size())) {
      return QString::fromStdString(labels[static_cast<size_t>(node)]);
    }
    return QString("#%1").arg(static_cast<int>(node));
  }

  PlannerGui::PlannerGui(const Texts& texts, QWidget* parent)
    : QMainWindow(parent)
  {
    setWindowTitle(texts.windowTitle);

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
    nearestBox_->setToolTip(texts.nearestLinksTip);

    // Mutually exclusive link overlay: the base contributes "Closest" (with
    // its count spinner); the derived window adds its plan radios below via
    // addPlanRadio. A QButtonGroup gives a single signal per selection.
    closestRadio_ = new QRadioButton("Closest", this);
    closestRadio_->setChecked(true);
    linkGroup_ = new QButtonGroup(this);
    linkGroup_->addButton(closestRadio_);

    QPushButton* regenButton = new QPushButton("Regenerate", this);
    QPushButton* recenterButton = new QPushButton("Recenter", this);
    recenterButton->setToolTip("Restore the centred, fitted view "
                               "(undo pan / zoom).");

    statusLabel_ = new QLabel(this);
    statusLabel_->setWordWrap(true);

    // --- control panel layout ---
    QGroupBox* genGroup = new QGroupBox("Instance", this);
    instanceForm_ = new QFormLayout(genGroup);
    instanceForm_->addRow("Laydown", laydownBox_);
    instanceForm_->addRow("Seed", seedBox_);
    instanceForm_->addRow("Supply-only", supplyOnlyBox_);
    instanceForm_->addRow("Both", bothBox_);
    instanceForm_->addRow("Demand-only", demandOnlyBox_);
    instanceForm_->addRow("Transit", transitBox_);
    instanceForm_->addRow(regenButton);

    // The closest-links count sits directly under its "Closest" radio,
    // indented; the derived window's plan radios follow.
    QGroupBox* modeGroup = new QGroupBox("Show Links", this);
    modeLayout_ = new QVBoxLayout(modeGroup);
    modeLayout_->addWidget(closestRadio_);
    QHBoxLayout* countRow = new QHBoxLayout();
    countRow->addSpacing(20);
    countRow->addWidget(new QLabel("links", this));
    countRow->addWidget(nearestBox_);
    countRow->addStretch(1);
    modeLayout_->addLayout(countRow);

    // The derived window fills this with its swap / purify / reset controls.
    QGroupBox* swapGroup = new QGroupBox("Swaps (2-exchange)", this);
    swapLayout_ = new QVBoxLayout(swapGroup);

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

    // Cost / distance histogram: all N^2 values in K equal-width bins, K live
    // via a spinner.
    histBinsBox_ = new QSpinBox(this);
    histBinsBox_->setRange(1, 25);
    histBinsBox_->setValue(10);
    histBinsBox_->setToolTip(texts.histogramBinsTip);
    histogram_ = new CostHistogram(this);

    QGroupBox* histGroup = new QGroupBox(texts.histogramTitle, this);
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
    panelLayout->addWidget(statusLabel_);
    panelLayout->addStretch(1);
    panel->setFixedWidth(240);

    // Right panel: the histogram on top (keeping the left panel short), then
    // two node rankings by plan flow, side by side. The lists are greyed out
    // (disabled) until a plan is shown; clicking an item pops the node's info
    // box on the map, exactly like clicking the node.
    throughputList_ = new NodeListWidget(this);
    countList_ = new NodeListWidget(this);
    throughputList_->setEnabled(false);
    countList_->setEnabled(false);

    QVBoxLayout* tputCol = new QVBoxLayout();
    tputCol->addWidget(new QLabel(texts.throughputHeader, this));
    tputCol->addWidget(throughputList_);
    QVBoxLayout* countCol = new QVBoxLayout();
    countCol->addWidget(new QLabel("By count", this));
    countCol->addWidget(countList_);

    QGroupBox* rankGroup = new QGroupBox("Flow through nodes", this);
    QHBoxLayout* rankLayout = new QHBoxLayout(rankGroup);
    rankLayout->addLayout(tputCol);
    rankLayout->addLayout(countCol);
    QWidget* rightPanel = new QWidget(this);
    rightLayout_ = new QVBoxLayout(rightPanel);
    rightLayout_->addWidget(histGroup);
    rightLayout_->addWidget(rankGroup);
    rightPanel->setFixedWidth(260);

    QWidget* central = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->addWidget(panel);
    mainLayout->addWidget(view_, 1);
    mainLayout->addWidget(rightPanel);
    setCentralWidget(central);

    // Regeneration is explicit (button) or on a structural control change; the
    // closest-links spin restyles the current instance; the mode radios drive
    // the derived window's applyPlanMode; recenter is view-only. The virtual
    // hooks dispatch to the derived window at signal time.
    connect(regenButton, &QPushButton::clicked, this, &PlannerGui::regenerate);
    connect(laydownBox_, &QComboBox::currentIndexChanged, this,
            &PlannerGui::regenerate);
    connect(nearestBox_,
            QOverload<int>::of(&QSpinBox::valueChanged), this,
            &PlannerGui::applyNearestK);
    connect(linkGroup_, &QButtonGroup::buttonClicked, this,
            &PlannerGui::applyPlanMode);
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
    return;
  }

  void
  PlannerGui::regenerate()
  {
    // Seed 0 is the "reroll" sentinel: draw a fresh time-based seed and write
    // it back so the field shows exactly what was used (the user can copy it).
    if (0 == seedBox_->value()) {
      seedBox_->setValue(timeSeed());
    }
    const std::uint64_t seed = static_cast<std::uint64_t>(seedBox_->value());
    try {
      // Stamp out any background solve / purify in flight BEFORE the rebuild:
      // rescoping a control inside rebuildInstance can fire a signal chain
      // that launches token-stamped work for the new instance.
      ++solveToken_;
      rebuildInstance(seed);
      lastSeed_ = seed;
      haveInstanceP_ = true;

      refreshMap();
      // Rescope the links spinner only after the view holds the new
      // instance: a clamped value fires applyNearestK at the view.
      const int hi = std::max(0, static_cast<int>(nodeCount()) - 1);
      nearestBox_->setRange(0, hi);

      applyPlanMode();
    }
    catch (const std::exception& ex) {
      haveInstanceP_ = false;
      view_->clearPlan();
      statusLabel_->setText(QString("Invalid profile: %1").arg(ex.what()));
    }
    return;
  }

  void
  PlannerGui::applyNearestK()
  {
    // The count belongs to "Closest" mode; inert (and disabled) otherwise.
    if (!haveInstanceP_ || !closestRadio_->isChecked()) {
      return;
    }
    view_->setNearestK(nearestBox_->value());
    refreshStatus(QString("closest links (k = %1)").arg(nearestBox_->value()));
    return;
  }

  void
  PlannerGui::startBusy()
  {
    ++busyCount_;
    if (!busyTimer_->isActive()) {
      busyBar_->setValue(0);
      busyTimer_->start();
    }
    return;
  }

  void
  PlannerGui::stopBusy()
  {
    busyCount_ = std::max(0, busyCount_ - 1);
    if (0 == busyCount_) {
      busyTimer_->stop();
      busyBar_->setValue(0);
    }
    return;
  }

  void
  PlannerGui::showFlowLists(const Plan& plan,
                            const std::vector<std::string>& labels)
  {
    const vector<NodeFlowStat> stats = computeNodeFlowStats(plan);
    fillNodeList(throughputList_, labels, stats, true);
    fillNodeList(countList_, labels, stats, false);
    throughputList_->setEnabled(true);
    countList_->setEnabled(true);
    return;
  }

  void
  PlannerGui::hideFlowLists()
  {
    throughputList_->clear();
    countList_->clear();
    throughputList_->setEnabled(false);
    countList_->setEnabled(false);
    return;
  }

  InstanceProfile
  PlannerGui::geometryProfile() const
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
  PlannerGui::addInstanceRow(const QString& label, QWidget* field)
  {
    // Insert above the Regenerate button, which stays the form's last row.
    instanceForm_->insertRow(instanceForm_->rowCount() - 1, label, field);
    return;
  }

  void
  PlannerGui::addInstanceRow(QWidget* row)
  {
    instanceForm_->insertRow(instanceForm_->rowCount() - 1, row);
    return;
  }

  void
  PlannerGui::addPlanRadio(QRadioButton* radio)
  {
    modeLayout_->addWidget(radio);
    linkGroup_->addButton(radio);
    return;
  }

  void
  PlannerGui::addSwapControl(QWidget* widget)
  {
    swapLayout_->addWidget(widget);
    return;
  }

  void
  PlannerGui::addRightPanelTop(QWidget* widget)
  {
    rightLayout_->insertWidget(0, widget);
    return;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
