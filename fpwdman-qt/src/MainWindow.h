// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

#include "PreferencesDialog.h" // Preferences struct
#include "SiteDatabase.h"

class QListWidget;
class QLineEdit;
class QLabel;
class QTimer;
class QPoint;

// The password-manager main window: an encrypted-database controller ported
// from the old FLTK FPwdMan. Handles open/save (SBC-encrypted), the master-
// passphrase flow, entry CRUD, find, sort, clipboard, live status lights, and
// the idle auto-close.
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString& initialPath = QString(), QWidget* parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

private slots:
    // File
    void onNewFile();
    void onOpen();
    void onSave();
    void onSaveAs();
    // Edit
    void onNewEntry();
    void onEditEntry();
    void onDeleteEntry();
    // Tools
    void onSortSites();
    void onPreferences();
    void onChangeMasterPassphrase();
    // Help
    void onAbout();
    void onUsage();
    // list / find
    void onFindReturnPressed();
    void onItemDoubleClicked();
    void onCopyPassword();
    void onListContextMenu(const QPoint& pos);
    // security
    void onIdleTimeout();

private:
    void setupMenus();
    void setupCentralWidget();
    void refreshList(int selectRow = -1);
    void updateStatusTiles();
    void markDirty();
    void resetIdle();
    int currentRow() const;

    bool maybeSaveGuard();               // true if it's OK to proceed (discard/saved)
    bool ensurePassphraseForSave();      // prompt-create a passphrase if none set
    bool doSave(const QString& path);    // encrypt+write; true on success
    void openDatabase(const QString& path); // prompt passphrase + open (with retry)

    QListWidget* m_list = nullptr;
    QLineEdit* m_findLineEdit = nullptr;
    QLineEdit* m_fileLineEdit = nullptr;
    QLabel* m_passphraseTile = nullptr;
    QLabel* m_changesTile = nullptr;
    QTimer* m_idleTimer = nullptr;

    SiteDatabase m_db;
    Preferences m_prefs;

    QString m_lastSearch;
    int m_lastFoundRow = -1;
    bool m_idleQuitting = false;
};

#endif // MAINWINDOW_H
// Copyright Ben Paul Wise. All Rights Reserved.
