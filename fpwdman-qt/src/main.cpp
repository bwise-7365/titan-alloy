// Copyright Ben Paul Wise. All Rights Reserved.
#include <QApplication>

#include "MainWindow.h"
#include "UiTheme.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    // Identify the app so QSettings has a stable per-user location for preferences.
    QApplication::setOrganizationName("BenPaulWise");
    QApplication::setApplicationName("fpwdman-qt");

    // All colors and the frame/field split live in ui::Theme (src/UiTheme.h),
    // so the palette can be retuned in one place. The menu gutter stays collapsed
    // there too -- none of these menus use icons, so it was dead space.
    app.setStyleSheet(ui::Theme::styleSheet());

    // Optional command-line argument: a .sbc file to open on startup.
    QString initialPath;
    if (argc > 1)
        initialPath = QString::fromLocal8Bit(argv[1]);

    MainWindow window(initialPath);
    window.show();

    return QApplication::exec();
}
// Copyright Ben Paul Wise. All Rights Reserved.
