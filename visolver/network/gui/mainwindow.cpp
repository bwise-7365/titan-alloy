// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// MainWindow implementation: builds the control panel, regenerates instances on
// demand via makeRandomInstance (seed 0 draws a fast-varying time-based seed),
// and keeps the closest-links spin in range.
// ----------------------------------------------
#include "mainwindow.hpp"

#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSpinBox>
#include <QVBoxLayout>
#include <QWidget>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <exception>

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

    QGroupBox* dispGroup = new QGroupBox("Display", this);
    QFormLayout* dispForm = new QFormLayout(dispGroup);
    dispForm->addRow("Closest links", nearestBox_);
    dispForm->addRow(recenterButton);

    QWidget* panel = new QWidget(this);
    QVBoxLayout* panelLayout = new QVBoxLayout(panel);
    panelLayout->addWidget(genGroup);
    panelLayout->addWidget(dispGroup);
    panelLayout->addWidget(statusLabel_);
    panelLayout->addStretch(1);
    panel->setFixedWidth(240);

    QWidget* central = new QWidget(this);
    QHBoxLayout* mainLayout = new QHBoxLayout(central);
    mainLayout->addWidget(panel);
    mainLayout->addWidget(view_, 1);
    setCentralWidget(central);

    // Regeneration is explicit (button) or on a structural control change; the
    // closest-links spin only restyles the current instance; recenter is view-
    // only.
    connect(regenButton, &QPushButton::clicked, this, &MainWindow::regenerate);
    connect(laydownBox_, &QComboBox::currentIndexChanged, this,
            &MainWindow::regenerate);
    connect(nearestBox_,
            QOverload<int>::of(&QSpinBox::valueChanged), this,
            &MainWindow::applyNearestK);
    connect(recenterButton, &QPushButton::clicked, view_,
            &FlowPlanView::recenter);

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
      const Instance inst = makeRandomInstance(profile, seed);
      view_->setInstance(inst);

      const int hi = std::max(0, static_cast<int>(inst.numNodes) - 1);
      nearestBox_->setRange(0, hi);
      view_->setNearestK(nearestBox_->value());

      statusLabel_->setText(
          QString("%1 nodes placed (laydown %2, seed %3).")
              .arg(static_cast<int>(inst.numNodes))
              .arg(profile.laydownType)
              .arg(static_cast<qulonglong>(seed)));
    }
    catch (const std::exception& ex) {
      statusLabel_->setText(QString("Invalid profile: %1").arg(ex.what()));
    }
    return;
  }

  void
  MainWindow::applyNearestK()
  {
    view_->setNearestK(nearestBox_->value());
    return;
  }

} // namespace VINCP::Network
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
