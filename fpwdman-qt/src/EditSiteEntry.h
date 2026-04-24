// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef EDITSITEENTRY_H
#define EDITSITEENTRY_H

#include <QDialog>
#include "SiteEntry.h"

class QLineEdit;

class EditSiteEntry : public QDialog {
    Q_OBJECT

public:
    explicit EditSiteEntry(SiteEntry *entry, QWidget *parent = nullptr);

private slots:
    void onOkClicked();
    void onResetClicked();

protected:
    void keyPressEvent(QKeyEvent *event) override;

private:
    SiteEntry *m_entry;
    QLineEdit *m_titleEdit;
    QLineEdit *m_siteEdit;
    QLineEdit *m_userIdEdit;
    QLineEdit *m_passwordEdit;
    QLineEdit *m_commentEdit;
};

#endif // EDITSITEENTRY_H
// Copyright Ben Paul Wise. All Rights Reserved.
