// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
// Ported from yracp for qtacp. YR::MainWindow becomes a QWidget
// window; the toggle commands become QCheckBoxes wired to the
// same static callbacks. The yr-library rows (Trace App, Trace
// UIC, Trace Memory, Trace Tips) are dropped with the yr library.
// ------------------------------------------

#ifndef YACP_CONTROL_PANEL_H
#define YACP_CONTROL_PANEL_H

// ------------------------------------------

#include "xtdemo.h"

#include <QWidget>

class SimGUIModule;

// ------------------------------------------

class ControlPanel : public QWidget {

public:
  ControlPanel(const char* name, QWidget* parent);
  ~ControlPanel();

  void initialize();

  static void toggleUnitsCallback(int v);
  static void toggleUnitSizeCallback(int v);
  static void toggleBoxesCallback(int v);
  static void clearBoxesCallback();
  static void toggleGridCallback(int v);
  static void toggleReliefCallback(int v);
  static void repeatableSeedsCallback(int v);

protected:

private:

  static SimGUIModule* getSGM();

};


// ------------------------------------------

#endif

// ------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ------------------------------------------
