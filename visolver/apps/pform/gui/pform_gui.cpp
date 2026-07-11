// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Entry point for pform_gui: the PFORM parliament-formation GUI, solving
// through the App::PForm problem class exactly as pform_cli does.
// ----------------------------------------------
#include "pformmainwindow.hpp"

#include <QApplication>

int
main(int argc, char* argv[])
{
  QApplication app(argc, argv);
  app.setApplicationName("PForm parliament-formation app");
  VINCP::App::PformMainWindow window;
  window.resize(1150, 620);
  window.show();
  return app.exec();
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
