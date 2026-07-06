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
    // --- The frame: windows, dialogs, menu bar/menus, and label text ---------
    static inline const QString frameBackground = QStringLiteral("#BBBBBB"); // medium-dark grey
    static inline const QString frameText = QStringLiteral("#101010");       // near-black, for on-frame text

    // --- Editable input surfaces (deliberately NOT grey) ---------------------
    static inline const QString fieldBackground = QStringLiteral("#FFFFDD"); // pale yellow
    static inline const QString fieldText = QStringLiteral("#000000");
    static inline const QString fieldBorder = QStringLiteral("#555555");

    // --- Selection / hover accent, shared by lists and menus -----------------
    static inline const QString accent = QStringLiteral("#3399FF");
    static inline const QString accentText = QStringLiteral("#FFFFFF");

    // The object name given to the main window's central container so the frame
    // color reliably reaches the window body (a plain child QWidget would
    // otherwise repaint over the styled QMainWindow background).
    static inline const QString centralObjectName = QStringLiteral("centralContainer");

    // Assemble the application-wide style sheet from the colors above.
    static QString styleSheet() {
        QString s;

        // The frame: top-level windows/dialogs, the central container, and the
        // menu bar all take the darker grey.
        s += QStringLiteral("QMainWindow, QDialog, QMenuBar, QWidget#%1 "
                            "{ background-color: %2; }")
                 .arg(centralObjectName, frameBackground);

        // Text that sits directly on the frame (labels and toggles are
        // transparent, so they show the grey behind them) must be light.
        s += QStringLiteral("QLabel, QCheckBox, QRadioButton, QGroupBox "
                            "{ color: %1; background: transparent; }")
                 .arg(frameText);

        // Menu bar entries: light text, accent highlight when active.
        s += QStringLiteral("QMenuBar::item { color: %1; }").arg(frameText);
        s += QStringLiteral("QMenuBar::item:selected { background-color: %1; color: %2; }")
                 .arg(accent, accentText);

        // Dropdown menus: grey to match the frame; keep the tightened gutter and
        // the accent highlight on the selected row.
        s += QStringLiteral("QMenu { background-color: %1; color: %2; padding: 4px; }")
                 .arg(frameBackground, frameText);
        s += QStringLiteral("QMenu::item { padding: 4px 24px 4px 10px; }");
        s += QStringLiteral("QMenu::item:selected { background-color: %1; color: %2; }")
                 .arg(accent, accentText);
        s += QStringLiteral("QMenu::separator { height: 1px; background: %1; margin: 4px 6px; }")
                 .arg(fieldBorder);

        // The pale input surfaces, unchanged from the original look.
        s += QStringLiteral(
                 "QPlainTextEdit, QLineEdit, QListWidget {"
                 " background-color: %1; color: %2;"
                 " font-family: 'Helvetica', 'Arial', 'Monaco', 'Courier New', monospace, sans-serif;"
                 " border: 2px solid %3;"
                 " selection-background-color: %4; selection-color: %5; }")
                 .arg(fieldBackground, fieldText, fieldBorder, accent, accentText);

        // Tooltips read as small grey chips on the dark frame.
        s += QStringLiteral("QToolTip { background-color: %1; color: %2; border: 1px solid %3; }")
                 .arg(frameBackground, frameText, fieldBorder);

        return s;
    }
};
} // namespace ui

#endif // UITHEME_H
// Copyright Ben Paul Wise. All Rights Reserved.
