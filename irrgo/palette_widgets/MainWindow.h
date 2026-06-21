// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <QColor>
#include <QMainWindow>

#include "palette/types.h"

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QRadioButton;

namespace palette_widgets {

class BoardPreview;
class RybColorWheel;
class PaletteController;

// Compact selector window (DESIGN.md §6). Holds the QColor<->Srgb boundary and
// wires the UI to a PaletteController; contains no palette algorithm logic.
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

private slots:
    void onModeChanged();
    void onPickBackground();
    void onPickPiece1();
    void onPickPiece2();
    void onHarmonyChanged(int index);
    void onLuminanceSpreadToggled(bool enabled);
    void onPaletteReady(const palette::Palette& result);

private:
    void buildUi();
    void updateInputVisibility();
    void setSwatch(QLabel* swatch, const QColor& color);

    PaletteController* controller_ = nullptr;

    QRadioButton* modeBackground_ = nullptr;
    QRadioButton* modeOnePiece_ = nullptr;
    QRadioButton* modeTwoPieces_ = nullptr;

    QPushButton* pickBackground_ = nullptr;
    QPushButton* pickPiece1_ = nullptr;
    QPushButton* pickPiece2_ = nullptr;
    QLabel* inputBackground_ = nullptr;
    QLabel* inputPiece1_ = nullptr;
    QLabel* inputPiece2_ = nullptr;

    QComboBox* harmonyCombo_ = nullptr;
    QCheckBox* spreadCheck_ = nullptr;

    BoardPreview* preview_ = nullptr;
    RybColorWheel* wheel_ = nullptr;

    QLabel* resultBackground_ = nullptr;
    QLabel* resultPiece1_ = nullptr;
    QLabel* resultPiece2_ = nullptr;
    QLabel* diagnostics_ = nullptr;
    QLabel* warnings_ = nullptr;

    // Current user-fixed input colors (defaults: beige board + red/blue pieces).
    QColor backgroundColor_{255, 255, 221};
    QColor piece1Color_{255, 0, 0};
    QColor piece2Color_{0, 0, 255};
};

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
