// Copyright Ben Paul Wise. All Rights Reserved.
#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    app.setStyleSheet(
        "QPlainTextEdit, QLineEdit, QListWidget {"
        "background-color: #FFFFDD;"
        "color: black;"
        "font-family: 'Helvetica', 'Arial', 'Monaco', 'Courier New', monospace, sans-serif;"
        "border: 2px solid #555555;"
        "selection-background-color: #3399FF;"
        "selection-color: white;"
        "}"
        "QToolTip {"
        "color: white;"
        "}");

    // Optional command-line argument: a .sbc file to open on startup.
    QString initialPath;
    if (argc > 1)
        initialPath = QString::fromLocal8Bit(argv[1]);

    MainWindow window(initialPath);
    window.show();

    return QApplication::exec();
}
// Copyright Ben Paul Wise. All Rights Reserved.
