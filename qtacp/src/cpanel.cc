// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. See cpanel.h for the yr-to-Qt
// mapping. The original noted it was "just a template"; the
// controls that act on real state (display flags, sim traces,
// the repeatable seed) are wired up, and the Clear Boxes button
// keeps its original no-op callback.
// ------------------------------------------

#include "xtdemo.h"

// ----------------------------------

#include "xtsim.h"
#include "cpanel.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

// ----------------------------------

ControlPanel::ControlPanel(const char* name, QWidget* parent)
  : QWidget(parent, Qt::Window) {
  setWindowTitle(name);
}


ControlPanel::~ControlPanel() {
}


// ----------------------------------

void
ControlPanel::initialize() {
  QCheckBox* toggle = NULL;

  QVBoxLayout* yrf = new QVBoxLayout(this);
  yrf->addWidget(new QLabel("ACP Controls", this));

  QHBoxLayout* mainBar = new QHBoxLayout();
  yrf->addLayout(mainBar);
  QVBoxLayout* vFrame1 = new QVBoxLayout();
  QVBoxLayout* vFrame2 = new QVBoxLayout();
  QVBoxLayout* vFrame3 = new QVBoxLayout();
  mainBar->addLayout(vFrame1);
  mainBar->addLayout(vFrame2);
  mainBar->addLayout(vFrame3);

  SimGUIModule* sgm = getSGM();
  assert (NULL != sgm);

  // setup first column of stuff
  vFrame1->addWidget(new QLabel("Display", this));

  QPushButton* clearBoxesButton = new QPushButton("Clear Boxes", this);
  clearBoxesButton->setToolTip("Clear display of boxes");
  QObject::connect(clearBoxesButton, &QPushButton::clicked,
                   &ControlPanel::clearBoxesCallback);
  vFrame1->addWidget(clearBoxesButton);

  // ---------------------
  toggle = new QCheckBox("Units", this);
  toggle->setChecked(0 != sgm->displayUnitsP);
  toggle->setToolTip("toggle display of units");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { ControlPanel::toggleUnitsCallback(v ? 1 : 0); });
  vFrame1->addWidget(toggle);

  // ---------------------
  toggle = new QCheckBox("Big units", this);
  toggle->setChecked(0 != sgm->displayUnitsLargeP);
  toggle->setToolTip("toggle display-size of units");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { ControlPanel::toggleUnitSizeCallback(v ? 1 : 0); });
  vFrame1->addWidget(toggle);

  // ---------------------
  toggle = new QCheckBox("Boxes", this);
  toggle->setChecked(0 != sgm->displayBoxesP); // start in state 1 (displayed)
  toggle->setToolTip("toggle display of planning boxes");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { ControlPanel::toggleBoxesCallback(v ? 1 : 0); });
  vFrame1->addWidget(toggle);

  // ---------------------
  toggle = new QCheckBox("Grid", this);
  toggle->setChecked(0 != sgm->displayGridP); // start in current state 0 (not displayed)
  toggle->setToolTip("toggle display of terrain grid");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { ControlPanel::toggleGridCallback(v ? 1 : 0); });
  vFrame1->addWidget(toggle);

  // ---------------------
  toggle = new QCheckBox("Relief", this);
  toggle->setChecked(0 != sgm->displayReliefP); // start in current state 0 (not displayed)
  toggle->setToolTip("toggle display of terrain relief");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { ControlPanel::toggleReliefCallback(v ? 1 : 0); });
  vFrame1->addWidget(toggle);

  vFrame1->addStretch();

  // setup second column of stuff
  vFrame2->addWidget(new QLabel("Simulation", this));

  // ---------------------
  toggle = new QCheckBox("Orders", this);
  toggle->setChecked(ACPSim::traceOrders);
  toggle->setToolTip("toggle tracing of orders");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { XtTestApp::toggleTraceOrders(v ? 1 : 0); });
  vFrame2->addWidget(toggle);

  // ---------------------
  toggle = new QCheckBox("Moves", this);
  toggle->setChecked(ACPSim::traceMoves);
  toggle->setToolTip("toggle tracing of movement");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { XtTestApp::toggleTraceMoves(v ? 1 : 0); });
  vFrame2->addWidget(toggle);

  // ---------------------
  toggle = new QCheckBox("Shots", this);
  toggle->setChecked(ACPSim::traceShots);
  toggle->setToolTip("toggle tracing of shots, detonations, etc.");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { XtTestApp::toggleTraceShots(v ? 1 : 0); });
  vFrame2->addWidget(toggle);

  // ---------------------
  toggle = new QCheckBox("Plans", this);
  toggle->setChecked(ACPSim::tracePlanning);
  toggle->setToolTip("toggle tracing of plans");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { XtTestApp::toggleTracePlans(v ? 1 : 0); });
  vFrame2->addWidget(toggle);

  // ---------------------

  AAA::FSM::debugFSM = 0;
  ACPSim::traceFSM = false;

  toggle = new QCheckBox("FSM", this);
  toggle->setChecked(ACPSim::traceFSM);
  toggle->setToolTip("toggle tracing of FSM states");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { XtTestApp::toggleTraceFSM(v ? 1 : 0); });
  vFrame2->addWidget(toggle);

  // ---------------------
  toggle = new QCheckBox("Sensors", this);
  toggle->setChecked(ACPSim::traceSensors);
  toggle->setToolTip("toggle tracing of sensor scans");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { XtTestApp::toggleTraceSensors(v ? 1 : 0); });
  vFrame2->addWidget(toggle);

  // ---------------------
  toggle = new QCheckBox("Geometry", this);
  toggle->setChecked(ACPSim::traceGeometry);
  toggle->setToolTip("toggle tracing of low-level\ngeometric operations");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { XtTestApp::toggleTraceGeometry(v ? 1 : 0); });
  vFrame2->addWidget(toggle);

  vFrame2->addStretch();

  // setup third column of stuff
  vFrame3->addWidget(new QLabel("Application", this));

  // ---------------------
  toggle = new QCheckBox("Repeatable", this);
  toggle->setChecked(ACPSim::RepeatableSeedP); // start in state 0 (not repeatable)
  toggle->setToolTip("toggle whether or not the\nsimulation is repeatable");
  QObject::connect(toggle, &QCheckBox::toggled,
                   [](bool v) { ControlPanel::repeatableSeedsCallback(v ? 1 : 0); });
  vFrame3->addWidget(toggle);

  vFrame3->addStretch();

  // ---------------------
  // add dismiss
  QPushButton* dismissButton = new QPushButton("Dismiss", this);
  dismissButton->setToolTip("hide this control panel");
  QObject::connect(dismissButton, &QPushButton::clicked,
                   [this]() { hide(); });
  yrf->addWidget(dismissButton);

  return;
}

void
ControlPanel::toggleUnitsCallback(int v) {
  cout << "setting toggleUnits to " << v << endl;
  SimGUIModule* sgm = getSGM();
  sgm->displayUnitsP = v;
  sgm->displaySim();
  return;
}

void
ControlPanel::toggleUnitSizeCallback(int v) {
  cout << "setting toggleUnitSize to " << v << endl;

  SimGUIModule* sgm = getSGM();
  sgm->displayUnitsLargeP = v;
  sgm->displaySim();
  return;
}

void
ControlPanel::toggleBoxesCallback(int v) {
  cout << "setting toggleBoxes to " << v << endl;
  SimGUIModule* sgm = getSGM();
  sgm->displayBoxesP = v;
  sgm->displaySim();
  return ;
}

SimGUIModule*
ControlPanel::getSGM() {
  SimGUIModule* sgm = NULL;
  XtTestApp* xta = xtTestApp;
  assert (NULL != xta);
  sgm = xta->simGUImodule;

  if (NULL == sgm) {
    xta->setupSimDemo();
    sgm = xta->simGUImodule;
  }

  assert (NULL != sgm);
  return sgm;
}

void
ControlPanel::toggleGridCallback(int v) {
  cout << "setting toggleGrid to " << v << endl;
  SimGUIModule* sgm = getSGM();
  sgm->displayGridP = v;
  sgm->displaySim();
  return;
}

void
ControlPanel::toggleReliefCallback(int v) {
  cout << "setting toggleRelief to " << v << endl;
  SimGUIModule* sgm = getSGM();
  sgm->displayReliefP = v;
  sgm->displaySim();
  return;
}

void
ControlPanel::repeatableSeedsCallback(int v) {
  cout << "setting repeatableSeeds to " << v << endl;
  ACPSim::RepeatableSeedP = v;
  return;
}

void
ControlPanel::clearBoxesCallback() {
  cout << "clearing Boxes" << endl;
  cout << " (really, no-op)"<<endl<<flush;
  return;
}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
