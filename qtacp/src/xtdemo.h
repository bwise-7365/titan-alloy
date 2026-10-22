// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. The X Athena Widget application
// (YR::XtApp) becomes a plain QWidget main window; the yr command
// objects become QPushButton signal connections; the retired demo
// members (toggle groups, sliders, snake/work-proc machinery, the
// memory-leak toggle) are dropped. The trace-toggle statics and the
// error window keep their old names and behavior.
//
// because this evolved as a set of different demo windows,
// everything sort of got dumped into the XtTestApp.
// ------------------------------------------

#ifndef XT_TESTAPP_H
#define XT_TESTAPP_H

#include "frwrdec.h"

#include <QWidget>

class XtTestApp;

extern XtTestApp* xtTestApp;

class SimGUIModule;
class ControlPanel;

// ------------------------------------------

class XtTestApp : public QWidget {
public:
  XtTestApp(const char* name);  // name of this instance
  virtual ~XtTestApp();

  void setupGUI(); // this sets up all the user stuff

  // because these were connected to general N-state
  // toggles, they must input int, not Logical
  static void toggleTraceOrders(int v);
  static void toggleTraceMoves(int v);
  static void toggleTraceShots(int v);
  static void toggleTracePlans(int v);
  static void toggleTraceFSM(int v);
  static void toggleTraceSensors(int v);
  static void toggleTraceGeometry(int v);

  static void raiseErrorWindow();

  // this displays and manages the simulation
  SimGUIModule* simGUImodule;

  // the simulation control panel window
  ControlPanel* ymw2;

  // the simulation demo's main window ("Feldspar")
  QWidget* ymw5;

  // these two are created on first use (if ever),
  // then raised on later requests. defined in xtsim.cc,
  // as in the original.
  static void launchSimDemo();
  static void launchControlPanel();

  void setupSimDemo();      // builds ymw5 (xtsim.cc)
  void setupControlPanel(); // builds ymw2 (stage 5)

protected:

private:

};

// ------------------------------------------

#endif

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
