// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef CHANGEMASTERPASSPHRASEDIALOG_H
#define CHANGEMASTERPASSPHRASEDIALOG_H

#include <QDialog>

class QLineEdit;

// Sets or changes the master passphrase. In "change" mode it also asks for the
// current passphrase; in "create" mode (first passphrase for a new database) it
// asks only for the new one twice. Ports ChangeMPUI plus the create case.
class ChangeMasterPassphraseDialog : public QDialog {
    Q_OBJECT

public:
    ChangeMasterPassphraseDialog(bool requireCurrent, int minLength, QWidget* parent = nullptr);

    QString currentPassphrase() const; // empty in create mode
    QString newPassphrase() const;

private slots:
    void validateAndAccept();

private:
    int m_minLength;
    QLineEdit* m_current = nullptr;
    QLineEdit* m_new = nullptr;
    QLineEdit* m_confirm = nullptr;
};

#endif // CHANGEMASTERPASSPHRASEDIALOG_H
// Copyright Ben Paul Wise. All Rights Reserved.
