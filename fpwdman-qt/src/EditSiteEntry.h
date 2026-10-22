// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef EDITSITEENTRY_H
#define EDITSITEENTRY_H

#include <QDialog>

#include "PasswordGenerator.h"
#include "SiteEntry.h"

class QLineEdit;

// Add / edit a site entry. Writes back to the entry only on OK. Includes a
// Suggest-Password button (ported from suggestSEpassword) and a show/hide toggle.
class EditSiteEntry : public QDialog {
    Q_OBJECT

public:
    EditSiteEntry(SiteEntry* entry, PasswordGenerator::Mode pwMode, int suggestedLength,
                  QWidget* parent = nullptr);

private slots:
    void onOkClicked();
    void onResetClicked();
    void onSuggestClicked();

private:
    SiteEntry* m_entry;
    PasswordGenerator::Mode m_pwMode;
    int m_suggestedLength;

    QLineEdit* m_titleEdit;
    QLineEdit* m_siteEdit;
    QLineEdit* m_userIdEdit;
    QLineEdit* m_passwordEdit;
    QLineEdit* m_commentEdit;
};

#endif // EDITSITEENTRY_H
// Copyright Ben Paul Wise. All Rights Reserved.
