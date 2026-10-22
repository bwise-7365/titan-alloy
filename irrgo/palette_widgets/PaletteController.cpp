// Copyright Ben Paul Wise. All Rights Reserved.

#include "PaletteController.h"
#include "palette/modes.h"

namespace palette_widgets {

PaletteController::PaletteController(QObject* parent) : QObject(parent) {}

void PaletteController::setMode(Mode mode) {
    mode_ = mode;
}

void PaletteController::setHarmony(palette::Harmony harmony) {
    harmony_ = harmony;
}

void PaletteController::setLuminanceSpread(bool enabled) {
    constraints_.allowPieceLuminanceSpread = enabled;
}

void PaletteController::setBackgroundColor(const palette::Srgb& color) {
    background_ = color;
}

void PaletteController::setPiece1Color(const palette::Srgb& color) {
    piece1_ = color;
}

void PaletteController::setPiece2Color(const palette::Srgb& color) {
    piece2_ = color;
}

void PaletteController::recompute() {
    palette::Palette result;
    switch (mode_) {
        case Mode::Background: {
            result = palette::fromBackground(background_, harmony_, constraints_);
            break;
        }
        case Mode::OnePiece: {
            result = palette::fromOnePiece(piece1_, harmony_, constraints_);
            break;
        }
        case Mode::TwoPieces: {
            result = palette::fromTwoPieces(piece1_, piece2_, constraints_);
            break;
        }
    }
    emit paletteReady(result);
}

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
