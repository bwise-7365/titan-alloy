// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <vector>
#include "SiteEntry.h"

class QPlainTextEdit;
class QLineEdit;

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

private:
    void setupMenus();
    void setupCentralWidget();

    QPlainTextEdit *m_textOutput;
    QLineEdit *m_findLineEdit;
    QLineEdit *m_fileLineEdit;
    QString m_lastSearchString;
    std::vector<SiteEntry> m_entries;
    int m_lastFoundIndex;
};

#endif // MAINWINDOW_H
// Copyright Ben Paul Wise. All Rights Reserved.
