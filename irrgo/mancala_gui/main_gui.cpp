// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include <QApplication>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("Mancala");
    MainWindow w;
    w.show();
    return app.exec();
}
// Copyright Ben Paul Wise. All Rights Reserved.
