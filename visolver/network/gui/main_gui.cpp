// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Entry point for the flow-plan instance viewer (network_viewer).
// ----------------------------------------------
#include "mainwindow.hpp"

#include <QApplication>

int
main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  app.setApplicationName("Flow-plan instance viewer");
  VINCP::Network::MainWindow window;
  window.resize(1000, 680);
  window.show();
  return app.exec();
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
