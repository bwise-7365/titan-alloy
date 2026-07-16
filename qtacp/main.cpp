// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// qtacp entry point. The old yr library owned main() and built
// the application from a global constructor; here QApplication
// is constructed first and the XtTestApp window after it, so
// widget construction never precedes the application object.
// ----------------------------------------------

#include <QApplication>

#include "xtdemo.h"

int main(int argc, char** argv) {
  QApplication app(argc, argv);
  QApplication::setApplicationName("qtacp");

  xtTestApp = new XtTestApp("Babel");
  xtTestApp->show();

  int rc = QApplication::exec();

  delete xtTestApp;
  xtTestApp = NULL;

  return rc;
}

// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
