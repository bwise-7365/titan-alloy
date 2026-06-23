// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"

#include <QApplication>
#include <QFontDatabase>
#include <QString>
#include <QStringList>

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    app.setApplicationName("Latrunculi");

    // Bundled OFL banner font (a Trajan-style Roman face). A missing or unreadable
    // font is surfaced as a warning rather than silently substituted; the banner
    // then falls back to the default UI font.
    QString bannerFamily;

    // Marcellus banner font -- kept (commented out) in case we revert; currently
    // trying Constantine instead.
    // const int fontId = QFontDatabase::addApplicationFont(":/fonts/Marcellus-Regular.ttf");
    // if (fontId < 0) {
    //     qWarning("Latrunculi: banner font ':/fonts/Marcellus-Regular.ttf' failed to "
    //              "load; using the default UI font for the banner.");
    // } else {
    //     const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
    //     if (!families.isEmpty()) {
    //         bannerFamily = families.first();
    //     }
    // }

    const int fontId = QFontDatabase::addApplicationFont(":/fonts/Constantine.ttf");
    if (fontId < 0) {
        qWarning("Latrunculi: banner font ':/fonts/Constantine.ttf' failed to "
                 "load; using the default UI font for the banner.");
    } else {
        const QStringList families = QFontDatabase::applicationFontFamilies(fontId);
        if (!families.isEmpty()) {
            bannerFamily = families.first();
        }
    }

    MainWindow window;
    window.setBannerFont(bannerFamily);
    window.show();
    return app.exec();
}
// Copyright Ben Paul Wise. All Rights Reserved.
