// Copyright Ben Paul Wise. All Rights Reserved.
#include "EditSiteEntry.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QKeyEvent>

EditSiteEntry::EditSiteEntry(SiteEntry *entry, QWidget *parent)
    : QDialog(parent), m_entry(entry) {
    setWindowTitle(tr("Edit Site Entry"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    // Ensure even and compact spacing, matching ViewSiteEntry
    const int commonSpacing = 6;
    formLayout->setVerticalSpacing(commonSpacing);
    mainLayout->setSpacing(commonSpacing);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    m_titleEdit = new QLineEdit(this);
    m_siteEdit = new QLineEdit(this);
    m_userIdEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_commentEdit = new QLineEdit(this);

    formLayout->addRow(tr("Title:"), m_titleEdit);
    formLayout->addRow(tr("Site:"), m_siteEdit);
    formLayout->addRow(tr("UserID:"), m_userIdEdit);
    formLayout->addRow(tr("Password:"), m_passwordEdit);
    formLayout->addRow(tr("Comment:"), m_commentEdit);

    onResetClicked(); // Populate fields from entry

    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    
    QPushButton *okButton = new QPushButton(tr("OK"), this);
    QPushButton *resetButton = new QPushButton(tr("Reset"), this);
    QPushButton *cancelButton = new QPushButton(tr("Cancel"), this);

    // Order: OK (left), Reset (middle), Cancel (right)
    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(cancelButton);

    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &EditSiteEntry::onOkClicked);
    connect(resetButton, &QPushButton::clicked, this, &EditSiteEntry::onResetClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);

    // Set fixed width to match user's general window size preference
    setFixedWidth(400);
}

void EditSiteEntry::onOkClicked() {
    if (m_entry) {
        m_entry->Title = m_titleEdit->text();
        m_entry->Site = m_siteEdit->text();
        m_entry->UserID = m_userIdEdit->text();
        m_entry->Password = m_passwordEdit->text();
        m_entry->Comment = m_commentEdit->text();
    }
    accept();
}

void EditSiteEntry::onResetClicked() {
    if (m_entry) {
        m_titleEdit->setText(m_entry->Title);
        m_siteEdit->setText(m_entry->Site);
        m_userIdEdit->setText(m_entry->UserID);
        m_passwordEdit->setText(m_entry->Password);
        m_commentEdit->setText(m_entry->Comment);
    }
}

void EditSiteEntry::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        // "If the user hits 'return', it does the Cancel action."
        reject();
        return;
    }
    QDialog::keyPressEvent(event);
}
// Copyright Ben Paul Wise. All Rights Reserved.
