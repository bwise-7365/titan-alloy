// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "MainWindow.h"

#include <QApplication>

int
main(int argc, char** argv)
{
  QApplication app(argc, argv);
  QApplication::setApplicationName("HexMapEd");
  HexQt::MainWindow window;
  if (argc > 1) {
    window.open(QString::fromLocal8Bit(argv[1]));
  }
  window.show();
  return QApplication::exec();
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
