// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pform GUI entry point: the SAOE graphical app (currently a placeholder shell).
// ----------------------------------------------
#include "pformmainwindow.hpp"

#include <QApplication>

int
main(int argc, char** argv)
{
  QApplication app(argc, argv);
  app.setApplicationName("pform -- SAOE");
  VINCP::App::PformMainWindow window;
  window.resize(720, 480);
  window.show();
  return app.exec();
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
