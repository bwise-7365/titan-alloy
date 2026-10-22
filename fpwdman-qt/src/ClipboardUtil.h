// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef CLIPBOARDUTIL_H
#define CLIPBOARDUTIL_H

#include <QApplication>
#include <QClipboard>
#include <QGuiApplication>
#include <QString>
#include <QTimer>

// -----------------------------------------------------------------------------
// Sensitive-clipboard helper. A password manager should not leave a copied
// password sitting on the system clipboard indefinitely, so copying goes through
// copySensitive(), which schedules an auto-clear. Every clear -- timed or on
// exit -- only fires if the clipboard still holds exactly what we put there, so
// if the user has since copied something else we leave their newer content alone.
//
// The timed clear runs on the Qt event loop and so cannot help if the app exits
// first; clearIfOurs() closes that gap by clearing synchronously on shutdown. To
// recognize "our" content across both paths we remember the last text we copied.
// -----------------------------------------------------------------------------

namespace cliputil {

constexpr int kDefaultClearMs = 30000; // 30 seconds

// The last sensitive text we placed on the clipboard, for the still-ours checks.
// A function-local static keeps this header-only with a single shared instance.
inline QString& lastCopied() {
    static QString s;
    return s;
}

inline void copySensitive(const QString& text, int clearMs = kDefaultClearMs) {
    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(text);
    lastCopied() = text;
    if (clearMs <= 0 || text.isEmpty())
        return;
    // qApp owns the timer, so a pending clear is dropped cleanly on app exit.
    QTimer::singleShot(clearMs, qApp, [text]() {
        QClipboard* cb = QGuiApplication::clipboard();
        if (cb->text() == text) {
            cb->clear();
            if (lastCopied() == text)
                lastCopied().clear();
        }
    });
}

// Clear the clipboard now if it still holds the password we last copied. Call on
// shutdown so a copied password never outlives the app on the system clipboard.
inline void clearIfOurs() {
    const QString ours = lastCopied();
    lastCopied().clear();
    if (ours.isEmpty())
        return;
    QClipboard* cb = QGuiApplication::clipboard();
    if (cb->text() == ours)
        cb->clear();
}

} // namespace cliputil

#endif // CLIPBOARDUTIL_H
// Copyright Ben Paul Wise. All Rights Reserved.
