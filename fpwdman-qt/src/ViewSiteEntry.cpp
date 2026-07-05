// Copyright Ben Paul Wise. All Rights Reserved.
#include "ViewSiteEntry.h"

#include <QClipboard>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

ViewSiteEntry::ViewSiteEntry(const SiteEntry* entry, QWidget* parent) : QDialog(parent) {
    setWindowTitle(tr("View Site Entry"));

    auto* mainLayout = new QVBoxLayout(this);
    auto* formLayout = new QFormLayout();

    const int commonSpacing = 6;
    formLayout->setVerticalSpacing(commonSpacing);
    mainLayout->setSpacing(commonSpacing);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    auto createReadOnlyField = [&](const QString& label, const QString& value) {
        auto* lineEdit = new QLineEdit(value, this);
        lineEdit->setReadOnly(true);
        formLayout->addRow(label + ":", lineEdit);
    };

    const QString password = entry ? entry->Password : QString();

    if (entry) {
        createReadOnlyField(tr("Title"), entry->Title);
        createReadOnlyField(tr("Site"), entry->Site);
        createReadOnlyField(tr("UserID"), entry->UserID);
        createReadOnlyField(tr("Password"), entry->Password);
        createReadOnlyField(tr("Comment"), entry->Comment);
    }

    mainLayout->addLayout(formLayout);

    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);

    auto* copyButton = new QPushButton(tr("Copy Password"), this);
    connect(copyButton, &QPushButton::clicked, this,
            [password]() { QGuiApplication::clipboard()->setText(password); });
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
