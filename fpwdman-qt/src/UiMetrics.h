// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef UIMETRICS_H
#define UIMETRICS_H

#include <QMargins>

// Shared spacing vocabulary so every dialog and the main window share one uniform,
// snug rhythm. Values are pixels.
namespace ui {
constexpr int kMargin = 8;       // outer content margin of a dialog / central widget
constexpr int kSpacing = 6;      // vertical gap between stacked widgets and form rows
constexpr int kFormHSpacing = 8; // horizontal gap between a form label and its field
constexpr int kGroupMargin = 8;  // internal margin inside a QGroupBox's layout

// The uniform outer margin as a QMargins, for setContentsMargins() call sites.
inline QMargins snugMargins() { return QMargins(kMargin, kMargin, kMargin, kMargin); }
} // namespace ui

#endif // UIMETRICS_H
// Copyright Ben Paul Wise. All Rights Reserved.
