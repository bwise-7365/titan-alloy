// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include "SiteEntry.h"

class QPlainTextEdit;
class QLineEdit;
class QLabel;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onFindReturnPressed();
    void onEditActionTriggered();
    void onSaveActionTriggered();
    void onSaveAsActionTriggered();
    void onLoadActionTriggered();

private:
    void setupMenus();
    void setupCentralWidget();
    void updateTextOutput();
    bool saveToFile(const QString &fileName);

    QPlainTextEdit *m_textOutput;
    QLineEdit *m_findLineEdit;
    QLineEdit *m_fileLineEdit;
    QLabel *m_entropyTile;
    QLabel *m_passphraseTile;
    QLabel *m_changesTile;
    QString m_lastSearchString;
    std::vector<SiteEntry> m_entries;
    int m_lastFoundIndex;
};

#endif // MAINWINDOW_H
// Copyright Ben Paul Wise. All Rights Reserved.
