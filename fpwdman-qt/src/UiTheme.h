// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef UITHEME_H
#define UITHEME_H

#include <QString>

// The single home for the application's colors and the global style sheet built
// from them. Change a color here and it changes everywhere -- no hex constants
// are scattered through the widget code.
//
// The look is a darker-grey "frame" (windows, dialogs, menus, and the text that
// sits on them) wrapping pale, high-contrast input surfaces (the Find box, the
// site list, and the entry fields), which keep their light background so typed
// text stays easy to read.
// 4A4A4A
namespace ui {
struct Theme {
    static inline const QString centralObjectName = QStringLiteral("centralContainer");

    // Assemble the application-wide style sheet from the palette (light or dark).
    static QString styleSheet(bool isDark = false) {
        const QString frameBackground = isDark ? QStringLiteral("#2B2B2B") : QStringLiteral("#D0D0E0");
        const QString frameText = isDark ? QStringLiteral("#F0F0F0") : QStringLiteral("#101010");
        const QString fieldBackground = isDark ? QStringLiteral("#1E1E1E") : QStringLiteral("#FFFFDD");
        const QString fieldText = isDark ? QStringLiteral("#E0E0E0") : QStringLiteral("#000000");
        const QString fieldBorder = isDark ? QStringLiteral("#555555") : QStringLiteral("#555555");
        const QString accent = QStringLiteral("#3399FF");
        const QString accentText = QStringLiteral("#FFFFFF");

        QString s;
        s += QStringLiteral("QMainWindow, QDialog, QMenuBar, QWidget#%1 { background-color: %2; }")
                 .arg(centralObjectName, frameBackground);
        s += QStringLiteral("QLabel, QCheckBox, QRadioButton, QGroupBox { color: %1; background: transparent; }")
                 .arg(frameText);
        s += QStringLiteral("QMenuBar::item { color: %1; }").arg(frameText);
        s += QStringLiteral("QMenuBar::item:selected { background-color: %1; color: %2; }")
                 .arg(accent, accentText);
        s += QStringLiteral("QMenu { background-color: %1; color: %2; padding: 4px; }")
                 .arg(frameBackground, frameText);
        s += QStringLiteral("QMenu::item { padding: 4px 24px 4px 10px; }");
        s += QStringLiteral("QMenu::item:selected { background-color: %1; color: %2; }")
                 .arg(accent, accentText);
        s += QStringLiteral("QMenu::separator { height: 1px; background: %1; margin: 4px 6px; }")
                 .arg(fieldBorder);
        s += QStringLiteral(
                 "QPlainTextEdit, QLineEdit, QListWidget {"
                 " background-color: %1; color: %2;"
                 " font-family: 'Helvetica', 'Arial', 'Monaco', 'Courier New', monospace, sans-serif;"
                 " border: 2px solid %3;"
                 " selection-background-color: %4; selection-color: %5; }")
                 .arg(fieldBackground, fieldText, fieldBorder, accent, accentText);
        s += QStringLiteral("QPushButton { background-color: %1; color: %2; border: 1px solid %3; padding: 3px 8px; border-radius: 3px; }"
                            "QPushButton:hover { background-color: %4; color: %5; }")
                 .arg(isDark ? QStringLiteral("#3C3F41") : QStringLiteral("#E5E5F0"), frameText, fieldBorder, accent, accentText);
        s += QStringLiteral("QToolTip { background-color: %1; color: %2; border: 1px solid %3; }")
                 .arg(frameBackground, frameText, fieldBorder);
        return s;
    }

    static QString badgeStyle(const QString& bgHex, const QString& fgHex = QStringLiteral("#FFFFFF")) {
        return QStringLiteral("background-color: %1; color: %2; font-weight: bold; "
                              "border-radius: 4px; padding: 2px 6px; font-size: 11px;")
                 .arg(bgHex, fgHex);
    }
};
} // namespace ui

#endif // UITHEME_H
// Copyright Ben Paul Wise. All Rights Reserved.
