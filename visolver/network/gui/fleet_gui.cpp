// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Entry point for the fleet-plan instance viewer (fleet_viewer).
// ----------------------------------------------
#include "fleetmainwindow.hpp"

#include <QApplication>

int
main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  app.setApplicationName("Fleet-plan instance viewer");
  VINCP::Network::FleetMainWindow window;
  window.resize(1280, 720);
  window.show();
  return app.exec();
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
