// Copyright Ben Paul Wise. All Rights Reserved.
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setStyleSheet(
        "QPlainTextEdit, QLineEdit {"
        "background-color: #FFFFDD;"
        "color: black;"
        "font-family: 'Helvetica', 'Arial', 'Monaco', 'Courier New', monospace, sans-serif;"
        "border: 2px solid #555555;"
        "selection-background-color: #3399FF;"
        "selection-color: white;"
        "}"
        "QToolTip {"
        "color: white;"
        "}"
    );

    MainWindow window;
    window.show();

    return QApplication::exec();
}
// Copyright Ben Paul Wise. All Rights Reserved.