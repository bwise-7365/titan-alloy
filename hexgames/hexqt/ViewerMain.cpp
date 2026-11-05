// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// pgg_trc_viewer [file] -- steps through a game of PGG or TRC. The file is a hexsave script or a run file from
// doc/pgg-runs; without one, File > Open asks.
// ----------------------------------------------
#include <QApplication>

#include "ViewerWindow.h"

int
main(int argc, char** argv)
{
  QApplication app(argc, argv);
  HexQt::ViewerWindow window;
  window.show();
  if (1 < argc) {
    window.openFile(QString::fromLocal8Bit(argv[1]));
  }
  return app.exec();
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
