// Copyright Ben Paul Wise. All Rights Reserved.
#include "ViewSiteEntry.h"

#include <QCheckBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

#include "ClipboardUtil.h"
#include "PasswordField.h"
#include "UiMetrics.h"

ViewSiteEntry::ViewSiteEntry(const SiteEntry* entry, int clipboardClearMs, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("View Site Entry"));

    auto* mainLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();

    formLayout->setVerticalSpacing(ui::kSpacing);
    formLayout->setHorizontalSpacing(ui::kFormHSpacing);
    mainLayout->setSpacing(ui::kSpacing);
    mainLayout->setContentsMargins(ui::snugMargins());

    auto createReadOnlyField = [&](const QString& label, const QString& value) {
        auto* lineEdit = new QLineEdit(value, this);
        lineEdit->setReadOnly(true);
        lineEdit->setFocusPolicy(Qt::StrongFocus);
        lineEdit->home(false);
        formLayout->addRow(label + ":", lineEdit);
        return lineEdit;
    };

    const QString password = entry ? entry->Password : QString();

    if (entry) {
        createReadOnlyField(tr("Title"), entry->Title);
        createReadOnlyField(tr("Site"), entry->Site);
        createReadOnlyField(tr("UserID"), entry->UserID);

        // The password is masked by default; a checkbox reveals it on demand,
        // matching the Edit dialog and keeping it off-screen at a glance.
        auto* passwordEdit = createReadOnlyField(tr("Password"), entry->Password);
        auto* showBox = new QCheckBox(tr("Show password"), this);
        formLayout->addRow(QString(), showBox);
        ui::wireRevealToggle(showBox, {passwordEdit});

        createReadOnlyField(tr("Comment"), entry->Comment);
    }

    mainLayout->addLayout(formLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->setSpacing(ui::kSpacing);

    auto* copyButton = new QPushButton(tr("Copy Password"), this);
    connect(copyButton, &QPushButton::clicked, this,
            [password, clipboardClearMs]() { cliputil::copySensitive(password, clipboardClearMs); });
    buttonLayout->addWidget(copyButton);

    buttonLayout->addStretch();
    auto* closeButton = new QPushButton(tr("Close"), this);
    closeButton->setDefault(true);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    setFixedWidth(420);
}

void ViewSiteEntry::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
        return;
    }
    QDialog::keyPressEvent(event);
}
// Copyright Ben Paul Wise. All Rights Reserved.
