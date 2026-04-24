// Copyright Ben Paul Wise. All Rights Reserved.
#include "ViewSiteEntry.h"
#include <QVBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QKeyEvent>

ViewSiteEntry::ViewSiteEntry(const SiteEntry *entry, QWidget *parent)
    : QDialog(parent) {
    setWindowTitle(tr("View Site Entry"));
    // resize(400, 300); // Removed to allow for a compact layout

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    QFormLayout *formLayout = new QFormLayout();

    // Ensure even and compact spacing
    const int commonSpacing = 6;
    formLayout->setVerticalSpacing(commonSpacing);
    mainLayout->setSpacing(commonSpacing);
    mainLayout->setContentsMargins(10, 10, 10, 10);

    auto createReadOnlyField = [&](const QString &label, const QString &value) {
        QLineEdit *lineEdit = new QLineEdit(value, this);
        lineEdit->setReadOnly(true);
        formLayout->addRow(label + ":", lineEdit);
    };

    if (entry) {
        createReadOnlyField(tr("Title"), entry->Title);
        createReadOnlyField(tr("Site"), entry->Site);
        createReadOnlyField(tr("UserID"), entry->UserID);
        createReadOnlyField(tr("Password"), entry->Password);
        createReadOnlyField(tr("Comment"), entry->Comment);
    }

    mainLayout->addLayout(formLayout);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    buttonLayout->addStretch();
    QPushButton *closeButton = new QPushButton(tr("Close"), this);
    closeButton->setDefault(true);
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    buttonLayout->addWidget(closeButton);

    mainLayout->addLayout(buttonLayout);

    // Set fixed width to match user's general window size preference
    // but allow height to be determined by the compact layout.
    setFixedWidth(400);
}

void ViewSiteEntry::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        accept();
        return;
    }
    QDialog::keyPressEvent(event);
}
// Copyright Ben Paul Wise. All Rights Reserved.
