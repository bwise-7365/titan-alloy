// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QPlainTextEdit>
#include <QApplication>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTextCursor>
#include "ViewSiteEntry.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_lastFoundIndex(-1) {
    setWindowTitle("fpwdman-qt");
    resize(300, 480);

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
    QWidget *container = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(container);

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

    // Generate 250 site entries
    QString content;
    for (int i = 1; i <= 250; ++i) {
        SiteEntry entry;
        entry.Title = QString("Title %1").arg(i, 3, 10, QChar('0'));
        entry.Site = "abcd";
        entry.UserID = "abcd";
        entry.Password = "abcd";
        entry.Comment = "abcd";
        m_entries.push_back(entry);
        content += entry.Title + "\n";
    }
    m_textOutput->setPlainText(content);

    mainLayout->addWidget(m_textOutput);

    // Find row
    QHBoxLayout *findLayout = new QHBoxLayout();
    QLabel *findLabel = new QLabel(tr("Find"), this);
    m_findLineEdit = new QLineEdit(this);
    findLayout->addWidget(findLabel);
    findLayout->addWidget(m_findLineEdit);
    mainLayout->addLayout(findLayout);

    // File row
    QHBoxLayout *fileLayout = new QHBoxLayout();
    QLabel *fileLabel = new QLabel(tr("File"), this);
    m_fileLineEdit = new QLineEdit(this);
    m_fileLineEdit->setReadOnly(true);
    fileLayout->addWidget(fileLabel);
    fileLayout->addWidget(m_fileLineEdit);
    mainLayout->addLayout(fileLayout);

    connect(m_findLineEdit, &QLineEdit::returnPressed, this, &MainWindow::onFindReturnPressed);

    m_textOutput->viewport()->installEventFilter(this);

    setCentralWidget(container);
}

void MainWindow::onFindReturnPressed() {
    QString searchString = m_findLineEdit->text();
    if (searchString.isEmpty()) return;

    if (searchString != m_lastSearchString) {
        m_lastFoundIndex = -1;
        m_lastSearchString = searchString;
    }

    bool found = false;
    int totalEntries = static_cast<int>(m_entries.size());
    if (totalEntries == 0) return;

    int startIndex = (m_lastFoundIndex + 1) % totalEntries;

    for (int i = 0; i < totalEntries; ++i) {
        int currentIndex = (startIndex + i) % totalEntries;
        const SiteEntry& entry = m_entries[currentIndex];

        if (entry.Title.contains(searchString, Qt::CaseInsensitive) ||
            entry.Site.contains(searchString, Qt::CaseInsensitive) ||
            entry.UserID.contains(searchString, Qt::CaseInsensitive) ||
            entry.Password.contains(searchString, Qt::CaseInsensitive) ||
            entry.Comment.contains(searchString, Qt::CaseInsensitive)) {
            
            m_lastFoundIndex = currentIndex;
            found = true;
            break;
        }
    }

    if (found) {
        QTextCursor cursor(m_textOutput->document());
        for (int i = 0; i < m_lastFoundIndex; ++i) {
            cursor.movePosition(QTextCursor::NextBlock);
        }
        cursor.select(QTextCursor::LineUnderCursor);
        m_textOutput->setTextCursor(cursor);
        m_textOutput->setFocus();
        m_textOutput->ensureCursorVisible();
    }
}

void MainWindow::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // If focus is not on the find line edit, trigger find again
        if (!m_findLineEdit->hasFocus()) {
            onFindReturnPressed();
            return;
        }
    }
    QMainWindow::keyPressEvent(event);
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event) {
    if (watched == m_textOutput->viewport() && event->type() == QEvent::MouseButtonDblClick) {
        QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
        QTextCursor cursor = m_textOutput->cursorForPosition(mouseEvent->pos());
        int lineIndex = cursor.blockNumber();
        if (lineIndex >= 0 && lineIndex < static_cast<int>(m_entries.size())) {
            ViewSiteEntry dialog(&m_entries[lineIndex], this);
            dialog.exec();
            return true;
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
// Copyright Ben Paul Wise. All Rights Reserved.
