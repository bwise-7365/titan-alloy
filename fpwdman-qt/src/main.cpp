// Copyright Ben Paul Wise. All Rights Reserved.
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return QApplication::exec();
}
// Copyright Ben Paul Wise. All Rights Reserved.