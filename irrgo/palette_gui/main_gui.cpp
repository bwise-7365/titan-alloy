// Copyright Ben Paul Wise. All Rights Reserved.
#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    palette_widgets::MainWindow window;
    window.setWindowTitle("Palette Selector");
    window.resize(560, 660);
    window.show();
    return app.exec();
}
// Copyright Ben Paul Wise. All Rights Reserved.
