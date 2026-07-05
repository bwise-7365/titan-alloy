// Copyright Ben Paul Wise. All Rights Reserved.
#include <QApplication>

#include "MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Identify the app so QSettings has a stable per-user location for preferences.
    QApplication::setOrganizationName("BenPaulWise");
    QApplication::setApplicationName("fpwdman-qt");

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
        "}"
        // Collapse the wide icon/checkmark gutter Qt reserves on the left of every
        // menu item -- none of these menus use icons, so it was dead space.
        "QMenu {"
        "padding: 4px;"
        "}"
        "QMenu::item {"
        "padding: 4px 24px 4px 10px;"
        "}"
        "QMenu::item:selected {"
        "background-color: #3399FF;"
        "color: white;"
        "}"
        "QMenu::separator {"
        "height: 1px;"
        "background: #555555;"
        "margin: 4px 6px;"
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
