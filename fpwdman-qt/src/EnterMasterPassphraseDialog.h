// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef ENTERMASTERPASSPHRASEDIALOG_H
#define ENTERMASTERPASSPHRASEDIALOG_H

#include <QDialog>

class QLineEdit;

// Prompts for the master passphrase when opening a file (single secret field
// with a show/hide toggle). Ports the old EnterMPUI.
class EnterMasterPassphraseDialog : public QDialog {
    Q_OBJECT

public:
    explicit EnterMasterPassphraseDialog(const QString& prompt, QWidget* parent = nullptr);
    QString passphrase() const;

private:
    QLineEdit* m_edit;
};

#endif // ENTERMASTERPASSPHRASEDIALOG_H
// Copyright Ben Paul Wise. All Rights Reserved.
