// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PREFERENCESDIALOG_H
#define PREFERENCESDIALOG_H

#include <QDialog>

#include "PasswordGenerator.h"

class QRadioButton;
class QSpinBox;

// User preferences, ported from the old ControlPanelUI.
struct Preferences {
    bool searchFullEntry = true;                                  // find scope
    PasswordGenerator::Mode pwMode = PasswordGenerator::Mode::UcLcDd; // charset
    int minSiteLength = 8;
    int suggestedSiteLength = 16;
    int minMasterLength = 8;
    int clipboardClearSeconds = 30; // auto-clear a copied password after this long
};

class PreferencesDialog : public QDialog {
    Q_OBJECT

public:
    explicit PreferencesDialog(const Preferences& prefs, QWidget* parent = nullptr);
    Preferences preferences() const;

private:
    QRadioButton* m_findTitleOnly;
    QRadioButton* m_findFull;
    QRadioButton* m_charDd;  // UC,LC,D (digit-doubled)
    QRadioButton* m_charDds; // UC,LC,D,S (adds symbols)
    QSpinBox* m_minSite;
    QSpinBox* m_suggestedSite;
    QSpinBox* m_minMaster;
    QSpinBox* m_clipboardClear;
};

#endif // PREFERENCESDIALOG_H
// Copyright Ben Paul Wise. All Rights Reserved.
