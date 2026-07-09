// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pform GUI stub: the main window for the SAOE graphical app. Placeholder only
// -- the interactive design is specified later; this fixes the target wiring.
// ----------------------------------------------
#ifndef VINCP_APPS_PFORMMAINWINDOW_HPP
#define VINCP_APPS_PFORMMAINWINDOW_HPP

// pform is the graphical companion to the pmatrix CLI: it will read/edit an
// SAOE instance and display the SaoeResult. It is built fresh as a QMainWindow
// (NOT derived from the network viewers' PlannerGui, which is specialized to
// spatial flow problems and is a poor fit for SAOE's matrix input / table
// output). When the interactive design lands, the solve should follow the
// viewers' proven off-thread pattern -- QtConcurrent::run producing an outcome,
// a QFutureWatcher delivering it back on the GUI thread, and a solve-token
// staleness stamp so a superseded solve is discarded (see the reference in
// network/gui/fleetmainwindow.cpp). The SAOE class (saoeproblem.hpp) is the
// solve entry point; nothing under network/ is reused or modified.

#include <QMainWindow>

namespace VINCP::App {

  class PformMainWindow : public QMainWindow {
    Q_OBJECT

  public:
    explicit PformMainWindow(QWidget* parent = nullptr);
    ~PformMainWindow() override = default;

  protected:
  private:
  };

} // namespace VINCP::App

#endif // VINCP_APPS_PFORMMAINWINDOW_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
