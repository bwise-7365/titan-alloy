// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Entry point for fleet_app: the fleet planning GUI, solving through the
// App::Fleet problem class and reusing the shared viewer_common widgets.
// ----------------------------------------------
#include "fleetappmainwindow.hpp"

#include <QApplication>

int
main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  app.setApplicationName("Fleet-plan app");
  VINCP::App::FleetAppMainWindow window;
  window.resize(1280, 720);
  window.show();
  return app.exec();
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
