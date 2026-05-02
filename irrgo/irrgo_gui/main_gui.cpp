// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include <QApplication>
#include <QIcon>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/icons/irrgo-logo-gold-v2.png"));
    MainWindow w;
    w.resize(900, 720);
    w.show();
    return app.exec();
}
