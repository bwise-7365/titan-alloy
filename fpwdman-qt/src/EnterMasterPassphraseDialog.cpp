// Copyright Ben Paul Wise. All Rights Reserved.
#include "EnterMasterPassphraseDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QVBoxLayout>

#include "PasswordField.h"
#include "UiMetrics.h"

EnterMasterPassphraseDialog::EnterMasterPassphraseDialog(const QString& prompt, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Master Passphrase"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(ui::snugMargins());
    layout->setSpacing(ui::kSpacing);
    layout->addWidget(new QLabel(prompt, this));

    m_edit = new QLineEdit(this);
    layout->addWidget(m_edit);

    auto* showBox = new QCheckBox(tr("Show passphrase"), this);
    layout->addWidget(showBox);
    ui::wireRevealToggle(showBox, {m_edit});

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);

    setMinimumWidth(340);
    m_edit->setFocus();
}

QString EnterMasterPassphraseDialog::passphrase() const {
    return m_edit->text();
}
// Copyright Ben Paul Wise. All Rights Reserved.
