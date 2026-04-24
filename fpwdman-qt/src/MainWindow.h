#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class QPlainTextEdit;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void setupMenus();
    void setupCentralWidget();

    QPlainTextEdit *m_textOutput;
};

#endif // MAINWINDOW_H
