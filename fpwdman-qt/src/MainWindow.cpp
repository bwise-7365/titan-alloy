// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QPlainTextEdit>
#include <QApplication>
#include <algorithm>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QTextCursor>
#include <QFileDialog>
#include <QFile>
#include <QXmlStreamWriter>
#include <QXmlStreamReader>
#include <QPushButton>
#include <QMessageBox>
#include <QDialog>
#include <QPalette>
#include <QColor>
#include "EditSiteEntry.h"
#include "ViewSiteEntry.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_lastFoundIndex(-1) {
    setWindowTitle("fpwdman-qt");
    resize(250, 360);

    // Set tooltip style using QPalette
    /*
    QPalette pal = QApplication::palette();
    pal.setColor(QPalette::ToolTipBase, QColor("#FFFFDD"));
    pal.setColor(QPalette::ToolTipText, Qt::black);
    QApplication::setPalette(pal);
    */

    setupMenus();
    setupCentralWidget();
}

MainWindow::~MainWindow() {
}

void MainWindow::setupMenus() {
    QMenuBar *menuBar = this->menuBar();

    // File menu
    QMenu *fileMenu = menuBar->addMenu(tr("&File"));
    QAction *saveAction = fileMenu->addAction(tr("&Save"));
    QAction *saveAsAction = fileMenu->addAction(tr("Save &As ..."));
    QAction *loadAction = fileMenu->addAction(tr("&Load"));
    fileMenu->addSeparator();
    QAction *quitAction = fileMenu->addAction(tr("&Quit"));

    saveAction->setShortcut(QKeySequence::Save);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    loadAction->setShortcut(QKeySequence::Open);
    quitAction->setShortcut(QKeySequence::Quit);

    connect(saveAction, &QAction::triggered, this, &MainWindow::onSaveActionTriggered);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAsActionTriggered);
    connect(loadAction, &QAction::triggered, this, &MainWindow::onLoadActionTriggered);
    connect(quitAction, &QAction::triggered, qApp, &QApplication::quit);

    // Edit menu
    QMenu *editMenu = menuBar->addMenu(tr("&Edit"));
    editMenu->addAction(tr("&New"));
    QAction *editAction = editMenu->addAction(tr("&Edit"));
    editMenu->addAction(tr("&Delete"));
    connect(editAction, &QAction::triggered, this, &MainWindow::onEditActionTriggered);

    QMenu *toolsMenu = menuBar->addMenu(tr("&Tools"));
    QAction *sortAction = toolsMenu->addAction(tr("&Sort Sites"));
    connect(sortAction, &QAction::triggered, this, &MainWindow::onSortSitesTriggered);

    menuBar->addMenu(tr("&Help"));
}

void MainWindow::setupCentralWidget() {
    QWidget *container = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(container);

    m_textOutput = new QPlainTextEdit(this);
    m_textOutput->setReadOnly(true);

    // Generate 250 site entries
    for (int i = 1; i <= 250; ++i) {
        SiteEntry entry;
        entry.Title = QString("Title %1").arg(i, 3, 10, QChar('0'));
        entry.Site = "abcd";
        entry.UserID = "abcd";
        entry.Password = "abcd";
        entry.Comment = "abcd";
        m_entries.push_back(entry);
    }
    updateTextOutput();

    mainLayout->addWidget(m_textOutput, 1);

    // Bottom container for both text rows and the status tiles
    // We use a QGridLayout to ensure perfect alignment between 2 rows on the left and 3 tiles on the right.
    // Total 6 units of height: each row on left spans 3 units, each tile on right spans 2 units.
    QGridLayout *bottomGrid = new QGridLayout();
    bottomGrid->setVerticalSpacing(0);
    bottomGrid->setHorizontalSpacing(5);
    bottomGrid->setContentsMargins(0, 0, 0, 0);

    // Find row (spans rows 0-2)
    QLabel *findLabel = new QLabel(tr("Find"), this);
    m_findLineEdit = new QLineEdit(this);
    bottomGrid->addWidget(findLabel, 0, 0, 3, 1);
    bottomGrid->addWidget(m_findLineEdit, 0, 1, 3, 1);

    // File row (spans rows 3-5)
    QLabel *fileLabel = new QLabel(tr("File"), this);
    m_fileLineEdit = new QLineEdit(this);
    m_fileLineEdit->setReadOnly(true);
    bottomGrid->addWidget(fileLabel, 3, 0, 3, 1);
    bottomGrid->addWidget(m_fileLineEdit, 3, 1, 3, 1);

    // Three status tiles: 3 tiles * 18px = 54px total height. 
    // This matches the height of 6 units of 9px each.
    const int tileSize = 18;

    m_entropyTile = new QLabel(this);
    m_entropyTile->setFixedSize(tileSize, tileSize);
    m_entropyTile->setToolTip(tr("Sufficient Entropy?"));
    m_entropyTile->setStyleSheet("background-color: red; border: 1px solid gray;");

    m_passphraseTile = new QLabel(this);
    m_passphraseTile->setFixedSize(tileSize, tileSize);
    m_passphraseTile->setToolTip(tr("Passphrase set?"));
    m_passphraseTile->setStyleSheet("background-color: blue; border: 1px solid gray;");

    m_changesTile = new QLabel(this);
    m_changesTile->setFixedSize(tileSize, tileSize);
    m_changesTile->setToolTip(tr("Changes saved?"));
    m_changesTile->setStyleSheet("background-color: blue; border: 1px solid gray;");

    // Right part: Three status tiles (spans 2 units each)
    bottomGrid->addWidget(m_entropyTile, 0, 2, 2, 1, Qt::AlignCenter);
    bottomGrid->addWidget(m_passphraseTile, 2, 2, 2, 1, Qt::AlignCenter);
    bottomGrid->addWidget(m_changesTile, 4, 2, 2, 1, Qt::AlignCenter);

    // Ensure rows are equal height units
    for (int i = 0; i < 6; ++i) {
        bottomGrid->setRowStretch(i, 1);
        bottomGrid->setRowMinimumHeight(i, 9);
    }
    bottomGrid->setColumnStretch(1, 1); // Let line edits expand

    mainLayout->addLayout(bottomGrid, 0);

    connect(m_findLineEdit, &QLineEdit::returnPressed, this, &MainWindow::onFindReturnPressed);

    m_textOutput->viewport()->installEventFilter(this);

    setCentralWidget(container);
}

void MainWindow::onEditActionTriggered() {
    int lineIndex = m_textOutput->textCursor().blockNumber();
    if (lineIndex >= 0 && lineIndex < static_cast<int>(m_entries.size())) {
        EditSiteEntry dialog(&m_entries[lineIndex], this);
        if (dialog.exec() == QDialog::Accepted) {
            updateTextOutput();
            // Restore cursor to the edited line
            QTextCursor cursor(m_textOutput->document());
            for (int i = 0; i < lineIndex; ++i) {
                cursor.movePosition(QTextCursor::NextBlock);
            }
            cursor.select(QTextCursor::LineUnderCursor);
            m_textOutput->setTextCursor(cursor);
            m_textOutput->ensureCursorVisible();
        }
    }
}

void MainWindow::onSortSitesTriggered() {
    std::sort(m_entries.begin(), m_entries.end(), [](const SiteEntry &a, const SiteEntry &b) {
        return a.Title.compare(b.Title, Qt::CaseInsensitive) < 0;
    });
    updateTextOutput();
}

void MainWindow::onSaveActionTriggered() {
    QString fileName = m_fileLineEdit->text();
    if (fileName.isEmpty() || !QFile::exists(fileName)) {
        onSaveAsActionTriggered();
    } else {
        saveToFile(fileName);
    }
}

void MainWindow::onLoadActionTriggered() {
    QString fileName = QFileDialog::getOpenFileName(this, tr("Open File"), "", tr("XML Files (*.xml);;All Files (*)"));
    if (fileName.isEmpty()) {
        return;
    }

    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QDialog errorDialog(this);
        errorDialog.setWindowTitle(tr("Error"));
        QVBoxLayout *layout = new QVBoxLayout(&errorDialog);
        layout->addWidget(new QLabel(tr("The file is unreadable."), &errorDialog));
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        QPushButton *closeBtn = new QPushButton(tr("Close"), &errorDialog);
        connect(closeBtn, &QPushButton::clicked, &errorDialog, &QDialog::accept);
        btnLayout->addWidget(closeBtn);
        layout->addLayout(btnLayout);
        errorDialog.setFixedWidth(300);
        errorDialog.exec();
        return;
    }

    QXmlStreamReader reader(&file);
    std::vector<SiteEntry> newEntries;

    while (reader.readNextStartElement()) {
        if (reader.name() == QStringLiteral("FpwdMan")) {
            while (reader.readNextStartElement()) {
                if (reader.name() == QStringLiteral("SiteTable")) {
                    while (reader.readNextStartElement()) {
                        if (reader.name() == QStringLiteral("SiteEntry")) {
                            SiteEntry entry;
                            while (reader.readNextStartElement()) {
                                QString name = reader.name().toString();
                                QString text = reader.readElementText();
                                if (name == "title") entry.Title = text;
                                else if (name == "site") entry.Site = text;
                                else if (name == "userid") entry.UserID = text;
                                else if (name == "password") entry.Password = text;
                                else if (name == "comments") entry.Comment = text;
                            }
                            newEntries.push_back(entry);
                        } else {
                            reader.skipCurrentElement();
                        }
                    }
                } else {
                    reader.skipCurrentElement();
                }
            }
        } else {
            reader.skipCurrentElement();
        }
    }

    if (reader.hasError()) {
        QDialog errorDialog(this);
        errorDialog.setWindowTitle(tr("Error"));
        QVBoxLayout *layout = new QVBoxLayout(&errorDialog);
        layout->addWidget(new QLabel(tr("The file is unreadable."), &errorDialog));
        QHBoxLayout *btnLayout = new QHBoxLayout();
        btnLayout->addStretch();
        QPushButton *closeBtn = new QPushButton(tr("Close"), &errorDialog);
        connect(closeBtn, &QPushButton::clicked, &errorDialog, &QDialog::accept);
        btnLayout->addWidget(closeBtn);
        layout->addLayout(btnLayout);
        errorDialog.setFixedWidth(300);
        errorDialog.exec();
    } else {
        m_entries = std::move(newEntries);
        m_lastFoundIndex = -1;
        m_lastSearchString.clear();
        updateTextOutput();
        m_fileLineEdit->setText(fileName);
    }

    file.close();
}

void MainWindow::onSaveAsActionTriggered() {
    QString fileName = QFileDialog::getSaveFileName(this, tr("Save As"), "", tr("XML Files (*.xml);;All Files (*)"));
    if (fileName.isEmpty()) {
        return;
    }

    if (saveToFile(fileName)) {
        m_fileLineEdit->setText(fileName);
    }
}

bool MainWindow::saveToFile(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(this, tr("Error"), tr("Could not open file for writing: %1").arg(file.errorString()));
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();

    xml.writeStartElement("FpwdMan");
    xml.writeAttribute("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance");
    xml.writeAttribute("xsi:noNamespaceSchemaLocation", "fpwdman.xsd");

    xml.writeStartElement("SiteTable");

    for (const auto& entry : m_entries) {
        xml.writeStartElement("SiteEntry");
        xml.writeTextElement("title", entry.Title);
        xml.writeTextElement("site", entry.Site);
        xml.writeTextElement("userid", entry.UserID);
        xml.writeTextElement("password", entry.Password);
        xml.writeTextElement("comments", entry.Comment);
        xml.writeEndElement(); // SiteEntry
    }

    xml.writeEndElement(); // SiteTable
    xml.writeEndElement(); // FpwdMan
    xml.writeEndDocument();

    file.close();
    return true;
}

void MainWindow::updateTextOutput() {
    QString content;
    for (const auto& entry : m_entries) {
        content += entry.Title + "\n";
    }
    m_textOutput->setPlainText(content);
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
        cursor.select(QTextCursor::LineUnderCursor);
        m_textOutput->setTextCursor(cursor);
        int lineIndex = cursor.blockNumber();
        if (lineIndex >= 0 && lineIndex < static_cast<int>(m_entries.size())) {
            ViewSiteEntry dialog(&m_entries[lineIndex], this);
            dialog.exec();
        }
        return true;
    }
    return QMainWindow::eventFilter(watched, event);
}
// Copyright Ben Paul Wise. All Rights Reserved.
