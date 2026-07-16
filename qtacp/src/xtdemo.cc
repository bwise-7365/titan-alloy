// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. See xtdemo.h for the yr-to-Qt
// mapping. The window and application codenames are kept: the
// application instance is "Babel", the main window is "Ebony".
// ------------------------------------------

#include "xtdemo.h"
#include "acpsim.h"

#include <QApplication>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>

using AAA::Logical;
using AAA::LTrue;
using AAA::LFalse;

// ----------------------------------

// the single application-window instance. the old code created
// this in a global constructor and the yr library's main() used
// it; now main.cpp constructs it after QApplication exists.
XtTestApp* xtTestApp = NULL;

// ----------------------------------

void appBell() {
  QApplication::beep();
}

// ----------------------------------

XtTestApp::XtTestApp(const char* name) : QWidget(NULL) {
  setWindowTitle(name);
  simGUImodule = NULL;
  ymw2 = NULL;
  ymw5 = NULL;
  setupGUI();
}


XtTestApp::~XtTestApp() {
  // simGUImodule and the control panel are deleted by Qt if they
  // are widget children; simGUImodule owns the sim and is deleted
  // here (stage 4 wires it up)
}

// ----------------------------------
// the main window "Ebony": a label and the three buttons

void
XtTestApp::setupGUI() {
  setWindowTitle("Ebony");

  QVBoxLayout* frame1 = new QVBoxLayout(this);
  QLabel* ylbl2 = new QLabel("qtACP Main", this);
  frame1->addWidget(ylbl2);

  QHBoxLayout* frame11 = new QHBoxLayout();
  frame1->addLayout(frame11);

  QPushButton* controlsButton = new QPushButton("Controls", this);
  controlsButton->setToolTip("Partial mockup of ACP controls\nNon-functional");
  QObject::connect(controlsButton, &QPushButton::clicked,
                   &XtTestApp::launchControlPanel);
  frame11->addWidget(controlsButton);

  QPushButton* simButton = new QPushButton("Sim", this);
  simButton->setToolTip("Partial mockup of ACP GUI\nNon-functional");
  QObject::connect(simButton, &QPushButton::clicked,
                   &XtTestApp::launchSimDemo);
  frame11->addWidget(simButton);

  QPushButton* quitButton = new QPushButton("Quit", this);
  quitButton->setToolTip("Quit App Immediately");
  QObject::connect(quitButton, &QPushButton::clicked,
                   QApplication::instance(), &QApplication::quit);
  frame11->addWidget(quitButton);

  return;
}

// ----------------------------------
// launchSimDemo, setupSimDemo, launchControlPanel and
// setupControlPanel are defined in xtsim.cc, as in the original.

// ----------------------------------

void
XtTestApp::toggleTraceOrders(int i) {
  Logical v;
  if (0==i)
    v = LFalse;
  else
    v = LTrue;

  if (LTrue == v)
    cout <<" --- start tracing Orders";
  else
    cout <<" --- stop tracing Orders";
  cout << endl << endl;
  ACPSim::traceOrders = v;
  return;
}

void
XtTestApp::toggleTraceMoves(int i) {
  Logical v;
  if (0==i)
    v = LFalse;
  else
    v = LTrue;

  if (LTrue == v)
    cout <<" --- start tracing Moves";
  else
    cout <<" --- stop tracing Moves";
  cout << endl << endl;
  ACPSim::traceMoves = v;
  return;
}

void
XtTestApp::toggleTraceShots(int i) {
  Logical v;
  if (0==i)
    v = LFalse;
  else
    v = LTrue;

  if (LTrue == v)
    cout <<" --- start tracing Shots";
  else
    cout <<" --- stop tracing Shots";
  cout << endl << endl;
  ACPSim::traceShots = v;
  return;
}

void
XtTestApp::toggleTracePlans(int i) {
  Logical v;
  if (0==i)
    v = LFalse;
  else
    v = LTrue;

  if (LTrue == v)
    cout <<" --- start tracing Planning";
  else
    cout <<" --- stop tracing Planning";
  cout << endl << endl;
  ACPSim::tracePlanning = v;
  return;
}

void
XtTestApp::toggleTraceFSM(int i) {
  Logical v;
  if (0==i)
    v = LFalse;
  else
    v = LTrue;

  if (LTrue == v)
    cout <<" --- start tracing FSM";
  else
    cout <<" --- stop tracing FSM";
  cout << endl << endl;
  ACPSim::traceFSM = v;
  AAA::FSM::debugFSM = v;
  return;
}

void
XtTestApp::toggleTraceSensors(int i) {
  Logical v;
  if (0==i)
    v = LFalse;
  else
    v = LTrue;

  if (LTrue == v)
    cout <<" --- start tracing Sensors";
  else
    cout <<" --- stop tracing Sensors";
  cout << endl << endl;
  ACPSim::traceSensors = v;
  return;
}

void
XtTestApp::toggleTraceGeometry(int i) {
  Logical v;
  if (0==i)
    v = LFalse;
  else
    v = LTrue;

  if (LTrue == v)
    cout <<" --- start tracing Geometry";
  else
    cout <<" --- stop tracing Geometry";
  cout << endl << endl;
  ACPSim::traceGeometry = v;
  return;
}

// ----------------------------------

void
XtTestApp::raiseErrorWindow() {
  appBell();
  QMessageBox::warning(xtTestApp, "Notice Window",
                       "That is not a\nlegal operation.");
  return;
}


// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
