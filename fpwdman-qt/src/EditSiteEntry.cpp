// Copyright Ben Paul Wise. All Rights Reserved.
#include "EditSiteEntry.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "UiMetrics.h"

EditSiteEntry::EditSiteEntry(SiteEntry* entry, PasswordGenerator::Mode pwMode, int suggestedLength,
                             QWidget* parent)
    : QDialog(parent), m_entry(entry), m_pwMode(pwMode), m_suggestedLength(suggestedLength) {
    setWindowTitle(tr("Edit Site Entry"));

    auto* mainLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();

    formLayout->setVerticalSpacing(ui::kSpacing);
    formLayout->setHorizontalSpacing(ui::kFormHSpacing);
    mainLayout->setSpacing(ui::kSpacing);
    mainLayout->setContentsMargins(ui::kMargin, ui::kMargin, ui::kMargin, ui::kMargin);

    m_titleEdit = new QLineEdit(this);
    m_siteEdit = new QLineEdit(this);
    m_userIdEdit = new QLineEdit(this);
    m_passwordEdit = new QLineEdit(this);
    m_commentEdit = new QLineEdit(this);

    formLayout->addRow(tr("Title:"), m_titleEdit);
    formLayout->addRow(tr("Site:"), m_siteEdit);
    formLayout->addRow(tr("UserID:"), m_userIdEdit);

    // Password row: field + Suggest button.
    auto* passwordRow = new QHBoxLayout();
    passwordRow->setContentsMargins(0, 0, 0, 0);
    passwordRow->setSpacing(ui::kSpacing);
    passwordRow->addWidget(m_passwordEdit, 1);
    auto* suggestButton = new QPushButton(tr("Suggest"), this);
    passwordRow->addWidget(suggestButton);
    formLayout->addRow(tr("Password:"), passwordRow);

    auto* showBox = new QCheckBox(tr("Show password"), this);
    formLayout->addRow(QString(), showBox);

    formLayout->addRow(tr("Comment:"), m_commentEdit);

    m_passwordEdit->setEchoMode(QLineEdit::Password);
    connect(showBox, &QCheckBox::toggled, this, [this](bool on) {
        m_passwordEdit->setEchoMode(on ? QLineEdit::Normal : QLineEdit::Password);
    });

    onResetClicked(); // populate fields from the entry

    mainLayout->addLayout(formLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(ui::kSpacing);

    auto* okButton = new QPushButton(tr("OK"), this);
    auto* resetButton = new QPushButton(tr("Reset"), this);
    auto* cancelButton = new QPushButton(tr("Cancel"), this);
    okButton->setDefault(true); // Enter now confirms (was Cancel in the prototype)

    buttonLayout->addWidget(okButton);
    buttonLayout->addWidget(resetButton);
    buttonLayout->addWidget(cancelButton);
    mainLayout->addLayout(buttonLayout);

    connect(okButton, &QPushButton::clicked, this, &EditSiteEntry::onOkClicked);
    connect(resetButton, &QPushButton::clicked, this, &EditSiteEntry::onResetClicked);
    connect(cancelButton, &QPushButton::clicked, this, &QDialog::reject);
    connect(suggestButton, &QPushButton::clicked, this, &EditSiteEntry::onSuggestClicked);

    setFixedWidth(420);
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

void EditSiteEntry::onSuggestClicked() {
    m_passwordEdit->setText(PasswordGenerator::generate(m_suggestedLength, m_pwMode));
}
// Copyright Ben Paul Wise. All Rights Reserved.
