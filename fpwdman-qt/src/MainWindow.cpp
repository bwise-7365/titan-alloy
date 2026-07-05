// Copyright Ben Paul Wise. All Rights Reserved.
#include "MainWindow.h"

#include <algorithm>
#include <utility>

#include <QAction>
#include <QApplication>
#include <QCloseEvent>
#include <QFileDialog>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QVBoxLayout>

#include "ChangeMasterPassphraseDialog.h"
#include "ClipboardUtil.h"
#include "EditSiteEntry.h"
#include "EnterMasterPassphraseDialog.h"
#include "PasswordStore.h"
#include "PreferencesDialog.h"
#include "ViewSiteEntry.h"

namespace {
constexpr int kIdleMaxSeconds = 3600; // one hour, matches ClearFileCheckMaximum
constexpr int kMinMasterFloor = 6;    // matches the old MinKeyLength
} // namespace

MainWindow::MainWindow(const QString& initialPath, QWidget* parent) : QMainWindow(parent) {
    setWindowTitle("fpwdman-qt");
    resize(280, 380);

    loadPreferences();
    setupMenus();
    setupCentralWidget();
    updateStatusTiles();

    // Idle auto-close: reset on any user activity, fire once after the timeout.
    m_idleTimer = new QTimer(this);
    m_idleTimer->setSingleShot(true);
    connect(m_idleTimer, &QTimer::timeout, this, &MainWindow::onIdleTimeout);
    qApp->installEventFilter(this);
    resetIdle();

    if (!initialPath.isEmpty())
        QTimer::singleShot(0, this, [this, initialPath]() { openDatabase(initialPath); });
}

MainWindow::~MainWindow() = default;

// ---------------------------------------------------------------------------
// UI construction
// ---------------------------------------------------------------------------

void MainWindow::setupMenus() {
    QMenuBar* bar = menuBar();

    QMenu* fileMenu = bar->addMenu(tr("&File"));
    QAction* newFileAction = fileMenu->addAction(tr("&New"));
    QAction* openAction = fileMenu->addAction(tr("&Open ..."));
    QAction* saveAction = fileMenu->addAction(tr("&Save"));
    QAction* saveAsAction = fileMenu->addAction(tr("Save &As ..."));
    fileMenu->addSeparator();
    QAction* quitAction = fileMenu->addAction(tr("&Quit"));

    newFileAction->setShortcut(QKeySequence::New);
    openAction->setShortcut(QKeySequence::Open);
    saveAction->setShortcut(QKeySequence::Save);
    saveAsAction->setShortcut(QKeySequence::SaveAs);
    quitAction->setShortcut(QKeySequence::Quit);

    connect(newFileAction, &QAction::triggered, this, &MainWindow::onNewFile);
    connect(openAction, &QAction::triggered, this, &MainWindow::onOpen);
    connect(saveAction, &QAction::triggered, this, &MainWindow::onSave);
    connect(saveAsAction, &QAction::triggered, this, &MainWindow::onSaveAs);
    connect(quitAction, &QAction::triggered, this, &MainWindow::close);

    QMenu* editMenu = bar->addMenu(tr("&Edit"));
    QAction* newEntryAction = editMenu->addAction(tr("&New Entry"));
    QAction* editEntryAction = editMenu->addAction(tr("&Edit Entry"));
    QAction* deleteEntryAction = editMenu->addAction(tr("&Delete Entry"));
    connect(newEntryAction, &QAction::triggered, this, &MainWindow::onNewEntry);
    connect(editEntryAction, &QAction::triggered, this, &MainWindow::onEditEntry);
    connect(deleteEntryAction, &QAction::triggered, this, &MainWindow::onDeleteEntry);

    QMenu* toolsMenu = bar->addMenu(tr("&Tools"));
    QAction* sortAction = toolsMenu->addAction(tr("&Sort Sites"));
    QAction* prefsAction = toolsMenu->addAction(tr("&Preferences ..."));
    QAction* changeMpAction = toolsMenu->addAction(tr("&Change Master Passphrase ..."));
    connect(sortAction, &QAction::triggered, this, &MainWindow::onSortSites);
    connect(prefsAction, &QAction::triggered, this, &MainWindow::onPreferences);
    connect(changeMpAction, &QAction::triggered, this, &MainWindow::onChangeMasterPassphrase);

    QMenu* helpMenu = bar->addMenu(tr("&Help"));
    connect(helpMenu->addAction(tr("&About")), &QAction::triggered, this, &MainWindow::onAbout);
    connect(helpMenu->addAction(tr("&Usage")), &QAction::triggered, this, &MainWindow::onUsage);
}

void MainWindow::setupCentralWidget() {
    auto* container = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(container);

    m_list = new QListWidget(this);
    m_list->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(m_list, &QListWidget::itemDoubleClicked, this, &MainWindow::onItemDoubleClicked);
    connect(m_list, &QListWidget::customContextMenuRequested, this,
            &MainWindow::onListContextMenu);
    mainLayout->addWidget(m_list, 1);

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(5);
    grid->setContentsMargins(0, 0, 0, 0);

    m_findLineEdit = new QLineEdit(this);
    grid->addWidget(new QLabel(tr("Find"), this), 0, 0);
    grid->addWidget(m_findLineEdit, 0, 1);

    m_fileLineEdit = new QLineEdit(this);
    m_fileLineEdit->setReadOnly(true);
    grid->addWidget(new QLabel(tr("File"), this), 1, 0);
    grid->addWidget(m_fileLineEdit, 1, 1);

    const int tileSize = 18;
    m_passphraseTile = new QLabel(this);
    m_passphraseTile->setFixedSize(tileSize, tileSize);
    m_passphraseTile->setToolTip(tr("Master passphrase set?"));
    m_changesTile = new QLabel(this);
    m_changesTile->setFixedSize(tileSize, tileSize);
    m_changesTile->setToolTip(tr("Changes saved?"));
    grid->addWidget(m_passphraseTile, 0, 2, Qt::AlignCenter);
    grid->addWidget(m_changesTile, 1, 2, Qt::AlignCenter);

    grid->setColumnStretch(1, 1);
    mainLayout->addLayout(grid, 0);

    connect(m_findLineEdit, &QLineEdit::returnPressed, this, &MainWindow::onFindReturnPressed);

    setCentralWidget(container);
}

// ---------------------------------------------------------------------------
// View helpers
// ---------------------------------------------------------------------------

void MainWindow::refreshList(int selectRow) {
    m_list->clear();
    for (const auto& e : m_db.entries)
        m_list->addItem(e.Title);
    if (selectRow >= 0 && selectRow < m_list->count())
        m_list->setCurrentRow(selectRow);
}

void MainWindow::updateStatusTiles() {
    auto setTile = [](QLabel* tile, const char* color) {
        tile->setStyleSheet(QString("background-color: %1; border: 1px solid gray;").arg(color));
    };

    if (!m_db.passphraseSet())
        setTile(m_passphraseTile, "red");
    else if (m_db.unsavedMpChange)
        setTile(m_passphraseTile, "orange");
    else
        setTile(m_passphraseTile, "blue");

    setTile(m_changesTile, (m_db.unsavedChanges || m_db.unsavedMpChange) ? "red" : "blue");

    m_fileLineEdit->setText(m_db.filePath.isEmpty() ? tr("(unsaved)") : m_db.filePath);
}

void MainWindow::markDirty() {
    m_db.unsavedChanges = true;
    updateStatusTiles();
}

void MainWindow::resetIdle() {
    if (m_idleTimer)
        m_idleTimer->start(kIdleMaxSeconds * 1000);
}

int MainWindow::currentRow() const {
    return m_list->currentRow();
}

SiteEntry* MainWindow::currentEntry() {
    const int row = currentRow();
    if (row < 0 || row >= static_cast<int>(m_db.entries.size()))
        return nullptr;
    return &m_db.entries[row];
}

// ---------------------------------------------------------------------------
// File actions
// ---------------------------------------------------------------------------

void MainWindow::onNewFile() {
    if (!maybeSaveGuard())
        return;
    m_db.reset();
    m_lastFoundRow = -1;
    m_lastSearch.clear();
    refreshList();
    updateStatusTiles();
}

void MainWindow::onOpen() {
    if (!maybeSaveGuard())
        return;
    const QString fn = QFileDialog::getOpenFileName(
        this, tr("Open Password File"), QString(),
        tr("Encrypted files (*.sbc);;All Files (*)"));
    if (fn.isEmpty())
        return;
    openDatabase(fn);
}

void MainWindow::openDatabase(const QString& path) {
    for (;;) {
        EnterMasterPassphraseDialog dlg(
            tr("Enter the master passphrase for:\n%1").arg(path), this);
        if (dlg.exec() != QDialog::Accepted)
            return;
        const QString pass = dlg.passphrase();
        try {
            std::vector<SiteEntry> entries = pwstore::openFile(path, pass);
            m_db.entries = std::move(entries);
            m_db.filePath = path;
            m_db.passphrase = pass;
            m_db.unsavedChanges = false;
            m_db.unsavedMpChange = false;
            m_lastFoundRow = -1;
            m_lastSearch.clear();
            refreshList();
            updateStatusTiles();
            return;
        } catch (const pwstore::WrongPassphrase&) {
            QMessageBox::warning(this, tr("Wrong passphrase"),
                                 tr("That passphrase did not decrypt the file. Try again."));
            // loop and re-prompt
        } catch (const pwstore::CorruptFile&) {
            QMessageBox::critical(this, tr("Unreadable file"),
                                  tr("The file could not be decrypted (corrupt or not a "
                                     "password file)."));
            return;
        } catch (const pwstore::IoError& e) {
            QMessageBox::critical(this, tr("Error"), e.message);
            return;
        }
    }
}

void MainWindow::onSave() {
    if (m_db.filePath.isEmpty())
        onSaveAs();
    else
        doSave(m_db.filePath);
}

void MainWindow::onSaveAs() {
    QString fn = QFileDialog::getSaveFileName(this, tr("Save As"), QString(),
                                              tr("Encrypted files (*.sbc);;All Files (*)"));
    if (fn.isEmpty())
        return;
    if (!fn.endsWith(".sbc", Qt::CaseInsensitive))
        fn += ".sbc";
    doSave(fn);
}

bool MainWindow::ensurePassphraseForSave() {
    if (m_db.passphraseSet())
        return true;
    ChangeMasterPassphraseDialog dlg(/*requireCurrent=*/false,
                                     std::max(kMinMasterFloor, m_prefs.minMasterLength), this);
    if (dlg.exec() != QDialog::Accepted)
        return false;
    m_db.passphrase = dlg.newPassphrase();
    m_db.unsavedMpChange = false; // it will be written now
    return true;
}

bool MainWindow::doSave(const QString& path) {
    if (!ensurePassphraseForSave())
        return false;
    try {
        pwstore::saveFile(path, m_db.passphrase, m_db.entries);
    } catch (const pwstore::IoError& e) {
        QMessageBox::critical(this, tr("Error"),
                              tr("Could not write the file: %1").arg(e.message));
        return false;
    } catch (const pwstore::Error&) {
        QMessageBox::critical(this, tr("Error"), tr("Encryption failed."));
        return false;
    }
    m_db.filePath = path;
    m_db.unsavedChanges = false;
    m_db.unsavedMpChange = false;
    updateStatusTiles();
    return true;
}

// ---------------------------------------------------------------------------
// Entry actions
// ---------------------------------------------------------------------------

void MainWindow::onNewEntry() {
    SiteEntry entry;
    EditSiteEntry dlg(&entry, m_prefs.pwMode, m_prefs.suggestedSiteLength, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_db.entries.push_back(entry);
        markDirty();
        refreshList(static_cast<int>(m_db.entries.size()) - 1);
    }
}

void MainWindow::onEditEntry() {
    SiteEntry* entry = currentEntry();
    if (!entry)
        return;
    EditSiteEntry dlg(entry, m_prefs.pwMode, m_prefs.suggestedSiteLength, this);
    if (dlg.exec() == QDialog::Accepted) {
        markDirty();
        refreshList(currentRow());
    }
}

void MainWindow::onDeleteEntry() {
    SiteEntry* entry = currentEntry();
    if (!entry)
        return;
    const int row = currentRow();
    const SiteEntry& e = *entry;
    const auto answer = QMessageBox::question(
        this, tr("Delete entry"),
        tr("Delete \"%1\" (%2)?").arg(e.Title, e.Site),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;
    m_db.entries.erase(m_db.entries.begin() + row);
    markDirty();
    refreshList(std::min(row, static_cast<int>(m_db.entries.size()) - 1));
}

// ---------------------------------------------------------------------------
// Tools
// ---------------------------------------------------------------------------

void MainWindow::onSortSites() {
    std::sort(m_db.entries.begin(), m_db.entries.end(),
              [](const SiteEntry& a, const SiteEntry& b) {
                  return a.Title.compare(b.Title, Qt::CaseInsensitive) < 0;
              });
    markDirty();
    refreshList();
}

void MainWindow::onPreferences() {
    PreferencesDialog dlg(m_prefs, this);
    if (dlg.exec() == QDialog::Accepted) {
        m_prefs = dlg.preferences();
        savePreferences();
    }
}

// Preferences persist across runs in QSettings (per-user; no passwords are stored
// there). Unknown/first-run keys fall back to the struct's compiled-in defaults.
void MainWindow::loadPreferences() {
    QSettings settings;
    settings.beginGroup("preferences");
    const Preferences d; // defaults
    m_prefs.searchFullEntry = settings.value("searchFullEntry", d.searchFullEntry).toBool();
    m_prefs.pwMode = static_cast<PasswordGenerator::Mode>(
        settings.value("pwMode", static_cast<int>(d.pwMode)).toInt());
    m_prefs.minSiteLength = settings.value("minSiteLength", d.minSiteLength).toInt();
    m_prefs.suggestedSiteLength =
        settings.value("suggestedSiteLength", d.suggestedSiteLength).toInt();
    m_prefs.minMasterLength = settings.value("minMasterLength", d.minMasterLength).toInt();
    m_prefs.clipboardClearSeconds =
        settings.value("clipboardClearSeconds", d.clipboardClearSeconds).toInt();
    settings.endGroup();
}

void MainWindow::savePreferences() const {
    QSettings settings;
    settings.beginGroup("preferences");
    settings.setValue("searchFullEntry", m_prefs.searchFullEntry);
    settings.setValue("pwMode", static_cast<int>(m_prefs.pwMode));
    settings.setValue("minSiteLength", m_prefs.minSiteLength);
    settings.setValue("suggestedSiteLength", m_prefs.suggestedSiteLength);
    settings.setValue("minMasterLength", m_prefs.minMasterLength);
    settings.setValue("clipboardClearSeconds", m_prefs.clipboardClearSeconds);
    settings.endGroup();
}

void MainWindow::onChangeMasterPassphrase() {
    const int minLen = std::max(kMinMasterFloor, m_prefs.minMasterLength);
    if (!m_db.passphraseSet()) {
        ChangeMasterPassphraseDialog dlg(/*requireCurrent=*/false, minLen, this);
        if (dlg.exec() == QDialog::Accepted) {
            m_db.passphrase = dlg.newPassphrase();
            m_db.unsavedMpChange = true;
            updateStatusTiles();
        }
        return;
    }
    ChangeMasterPassphraseDialog dlg(/*requireCurrent=*/true, minLen, this);
    if (dlg.exec() != QDialog::Accepted)
        return;
    if (dlg.currentPassphrase() != m_db.passphrase) {
        QMessageBox::warning(this, tr("Wrong passphrase"),
                             tr("The current master passphrase is incorrect."));
        return;
    }
    m_db.passphrase = dlg.newPassphrase();
    m_db.unsavedMpChange = true; // takes effect on the next Save
    updateStatusTiles();
}

void MainWindow::onAbout() {
    QMessageBox::about(
        this, tr("About fpwdman-qt"),
        tr("<b>fpwdman-qt</b><br>A Qt password manager.<br><br>"
           "Reads legacy SBC-encrypted (.sbc) files and writes a modern, salted, "
           "authenticated container. Each entry holds a title, site, user ID, "
           "password, and comments."));
}

void MainWindow::onUsage() {
    QMessageBox::information(
        this, tr("Usage"),
        tr("Open a .sbc file (old or new) with its master passphrase, or start a "
           "new database and set a passphrase when you first save.\n\n"
           "Edit -> New Entry adds a site; double-click an entry to view it; "
           "right-click for Copy Password. Find searches your entries; Tools -> "
           "Preferences controls the find scope and generated-password style.\n\n"
           "The database auto-closes after one hour of inactivity."));
}

// ---------------------------------------------------------------------------
// List / find / clipboard
// ---------------------------------------------------------------------------

void MainWindow::onFindReturnPressed() {
    const QString term = m_findLineEdit->text();
    if (term.isEmpty())
        return;

    if (term != m_lastSearch) {
        m_lastFoundRow = -1;
        m_lastSearch = term;
    }

    const int total = static_cast<int>(m_db.entries.size());
    if (total == 0)
        return;

    const int start = (m_lastFoundRow + 1) % total;
    for (int i = 0; i < total; ++i) {
        const int row = (start + i) % total;
        const SiteEntry& e = m_db.entries[row];
        bool hit = e.Title.contains(term, Qt::CaseInsensitive);
        if (!hit && m_prefs.searchFullEntry) {
            hit = e.Site.contains(term, Qt::CaseInsensitive) ||
                  e.UserID.contains(term, Qt::CaseInsensitive) ||
                  e.Password.contains(term, Qt::CaseInsensitive) ||
                  e.Comment.contains(term, Qt::CaseInsensitive);
        }
        if (hit) {
            m_lastFoundRow = row;
            m_list->setCurrentRow(row);
            m_list->setFocus();
            return;
        }
    }
}

void MainWindow::onItemDoubleClicked() {
    SiteEntry* entry = currentEntry();
    if (!entry)
        return;
    ViewSiteEntry dlg(entry, m_prefs.clipboardClearSeconds * 1000, this);
    dlg.exec();
}

void MainWindow::onCopyPassword() {
    SiteEntry* entry = currentEntry();
    if (!entry)
        return;
    cliputil::copySensitive(entry->Password, m_prefs.clipboardClearSeconds * 1000);
}

void MainWindow::onListContextMenu(const QPoint& pos) {
    if (currentRow() < 0)
        return;
    QMenu menu(this);
    QAction* copyAction = menu.addAction(tr("Copy Password"));
    QAction* viewAction = menu.addAction(tr("View"));
    QAction* editAction = menu.addAction(tr("Edit"));
    QAction* deleteAction = menu.addAction(tr("Delete"));
    QAction* chosen = menu.exec(m_list->viewport()->mapToGlobal(pos));
    if (chosen == copyAction)
        onCopyPassword();
    else if (chosen == viewAction)
        onItemDoubleClicked();
    else if (chosen == editAction)
        onEditEntry();
    else if (chosen == deleteAction)
        onDeleteEntry();
}

// ---------------------------------------------------------------------------
// Unsaved-changes guard, idle close
// ---------------------------------------------------------------------------

bool MainWindow::maybeSaveGuard() {
    if (!m_db.unsavedChanges && !m_db.unsavedMpChange)
        return true;
    const auto answer = QMessageBox::warning(
        this, tr("Unsaved changes"),
        tr("There are unsaved changes. Save them?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
    if (answer == QMessageBox::Save) {
        onSave();
        // If the save was cancelled or failed, the flags remain set.
        return !(m_db.unsavedChanges || m_db.unsavedMpChange);
    }
    return answer == QMessageBox::Discard;
}

void MainWindow::onIdleTimeout() {
    m_idleQuitting = true; // bypass the guard: closing decrypted data is the point
    close();
}

bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    switch (event->type()) {
    case QEvent::KeyPress:
    case QEvent::MouseButtonPress:
    case QEvent::MouseMove:
        resetIdle();
        break;
    default:
        break;
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (m_idleQuitting) {
        cliputil::clearIfOurs(); // don't leave a copied password on the clipboard
        event->accept();
        return;
    }
    if (maybeSaveGuard()) {
        cliputil::clearIfOurs();
        event->accept();
    } else {
        event->ignore();
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.
