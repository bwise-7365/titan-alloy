// Copyright Ben Paul Wise. All Rights Reserved.
#include "PreferencesDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QRadioButton>
#include <QSpinBox>
#include <QVBoxLayout>

#include "UiMetrics.h"

PreferencesDialog::PreferencesDialog(const Preferences& prefs, QWidget* parent)
    : QDialog(parent) {
    setWindowTitle(tr("Preferences"));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(ui::snugMargins());
    layout->setSpacing(ui::kSpacing);

    // --- Find scope ---
    auto* findBox = new QGroupBox(tr("Find searches"), this);
    auto* findLayout = new QVBoxLayout(findBox);
    findLayout->setContentsMargins(ui::kGroupMargin, ui::kGroupMargin, ui::kGroupMargin,
                                   ui::kGroupMargin);
    findLayout->setSpacing(ui::kSpacing);
    m_findTitleOnly = new QRadioButton(tr("Title only"), findBox);
    m_findFull = new QRadioButton(tr("Entire entry"), findBox);
    findLayout->addWidget(m_findTitleOnly);
    findLayout->addWidget(m_findFull);
    (prefs.searchFullEntry ? m_findFull : m_findTitleOnly)->setChecked(true);
    layout->addWidget(findBox);

    // --- Generated-password characters ---
    auto* charBox = new QGroupBox(tr("Suggested-password characters"), this);
    auto* charLayout = new QVBoxLayout(charBox);
    charLayout->setContentsMargins(ui::kGroupMargin, ui::kGroupMargin, ui::kGroupMargin,
                                   ui::kGroupMargin);
    charLayout->setSpacing(ui::kSpacing);
    m_charDd = new QRadioButton(tr("Upper, lower, digits"), charBox);
    m_charDds = new QRadioButton(tr("Upper, lower, digits, symbols"), charBox);
    charLayout->addWidget(m_charDd);
    charLayout->addWidget(m_charDds);
    (prefs.pwMode == PasswordGenerator::Mode::UcLcDds ? m_charDds : m_charDd)->setChecked(true);
    layout->addWidget(charBox);

    // --- Lengths ---
    auto* lenBox = new QGroupBox(tr("Lengths"), this);
    auto* lenForm = new QFormLayout(lenBox);
    lenForm->setContentsMargins(ui::kGroupMargin, ui::kGroupMargin, ui::kGroupMargin,
                                ui::kGroupMargin);
    lenForm->setVerticalSpacing(ui::kSpacing);
    lenForm->setHorizontalSpacing(ui::kFormHSpacing);
    m_minSite = new QSpinBox(lenBox);
    m_minSite->setRange(1, 250);
    m_minSite->setValue(prefs.minSiteLength);
    m_suggestedSite = new QSpinBox(lenBox);
    m_suggestedSite->setRange(1, 250);
    m_suggestedSite->setValue(prefs.suggestedSiteLength);
    m_minMaster = new QSpinBox(lenBox);
    m_minMaster->setRange(4, 64);
    m_minMaster->setValue(prefs.minMasterLength);
    lenForm->addRow(tr("Minimum site password:"), m_minSite);
    lenForm->addRow(tr("Suggested site password:"), m_suggestedSite);
    lenForm->addRow(tr("Minimum master passphrase:"), m_minMaster);
    layout->addWidget(lenBox);

    // --- Clipboard ---
    auto* clipBox = new QGroupBox(tr("Clipboard"), this);
    auto* clipForm = new QFormLayout(clipBox);
    clipForm->setContentsMargins(ui::kGroupMargin, ui::kGroupMargin, ui::kGroupMargin,
                                 ui::kGroupMargin);
    clipForm->setVerticalSpacing(ui::kSpacing);
    clipForm->setHorizontalSpacing(ui::kFormHSpacing);
    m_clipboardClear = new QSpinBox(clipBox);
    m_clipboardClear->setRange(5, 600);
    m_clipboardClear->setSuffix(tr(" seconds"));
    m_clipboardClear->setValue(prefs.clipboardClearSeconds);
    clipForm->addRow(tr("Clear a copied password after:"), m_clipboardClear);
    layout->addWidget(clipBox);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

Preferences PreferencesDialog::preferences() const {
    Preferences p;
    p.searchFullEntry = m_findFull->isChecked();
    p.pwMode = m_charDds->isChecked() ? PasswordGenerator::Mode::UcLcDds
                                      : PasswordGenerator::Mode::UcLcDd;
    p.minSiteLength = m_minSite->value();
    p.suggestedSiteLength = m_suggestedSite->value();
    p.minMasterLength = m_minMaster->value();
    p.clipboardClearSeconds = m_clipboardClear->value();
    return p;
}
// Copyright Ben Paul Wise. All Rights Reserved.
