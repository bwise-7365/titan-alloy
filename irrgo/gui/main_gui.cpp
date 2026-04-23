// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    MainWindow w;
    w.resize(900, 720);
    w.show();
    return app.exec();
}
