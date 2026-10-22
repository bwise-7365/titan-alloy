// Copyright Ben Paul Wise. All Rights Reserved.
#include "ChangeMasterPassphraseDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QVBoxLayout>

#include "PasswordField.h"
#include "UiMetrics.h"

ChangeMasterPassphraseDialog::ChangeMasterPassphraseDialog(bool requireCurrent, int minLength,
                                                           QWidget* parent)
    : QDialog(parent), m_minLength(minLength) {
    setWindowTitle(requireCurrent ? tr("Change Master Passphrase")
                                  : tr("Set Master Passphrase"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(ui::snugMargins());
    layout->setSpacing(ui::kSpacing);
    auto* form = new QFormLayout();
    form->setVerticalSpacing(ui::kSpacing);
    form->setHorizontalSpacing(ui::kFormHSpacing);

    if (requireCurrent) {
        m_current = new QLineEdit(this);
        form->addRow(tr("Current:"), m_current);
    }
    m_new = new QLineEdit(this);
    form->addRow(tr("New:"), m_new);

    m_confirm = new QLineEdit(this);
    form->addRow(tr("Confirm:"), m_confirm);

    layout->addLayout(form);

    auto* showBox = new QCheckBox(tr("Show passphrases"), this);
    layout->addWidget(showBox);
    ui::wireRevealToggle(showBox, {m_current, m_new, m_confirm});

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this,
            &ChangeMasterPassphraseDialog::validateAndAccept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    setMinimumWidth(360);
}

void ChangeMasterPassphraseDialog::validateAndAccept() {
    if (m_new->text().length() < m_minLength) {
        QMessageBox::warning(this, tr("Passphrase too short"),
                             tr("The new passphrase must be at least %1 characters.")
                                 .arg(m_minLength));
        return;
    }
    if (m_new->text() != m_confirm->text()) {
        QMessageBox::warning(this, tr("Passphrases do not match"),
                             tr("The new passphrase and its confirmation differ."));
        return;
    }
    accept();
}

QString ChangeMasterPassphraseDialog::currentPassphrase() const {
    return m_current ? m_current->text() : QString();
}

QString ChangeMasterPassphraseDialog::newPassphrase() const {
    return m_new->text();
}
// Copyright Ben Paul Wise. All Rights Reserved.
