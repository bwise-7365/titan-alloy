#include "MainWindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QPlainTextEdit>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    setWindowTitle("fpwdman-qt");
    resize(400, 600);

    setupMenus();
    setupCentralWidget();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupMenus() {
    QMenuBar *menuBar = this->menuBar();

    // File menu
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));
    QAction *quitAction = fileMenu->addAction(tr("&Quit"));
    quitAction->setShortcut(QKeySequence::Quit);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    // Edit, Tools, Help menus (empty for now)
    menuBar->addMenu(tr("&Edit"));
    menuBar->addMenu(tr("&Tools"));
    menuBar->addMenu(tr("&Help"));
}

void MainWindow::setupCentralWidget() {
    m_textOutput = new QPlainTextEdit(this);
    m_textOutput->setReadOnly(true);

    // Styling: Background #FFFFDD, Black monospaced font
    m_textOutput->setStyleSheet(
        "QPlainTextEdit {"
        "background-color: #FFFFDD;"
        "color: black;"
        "font-family: 'Consolas', 'Monaco', 'Courier New', monospace;"
        "}"
    );

    // Populate with 200 lines: "Text %03d"
    QString content;
    for (int i = 1; i <= 200; ++i) {
        content += QString("Text %1\n").arg(i, 3, 10, QChar('0'));
    }
    m_textOutput->setPlainText(content);

    setCentralWidget(m_textOutput);
}
