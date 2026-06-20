// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <QObject>

#include "palette/color.hpp"
#include "palette/types.hpp"

// Thin bridge between the Qt UI and the pure palette_core. This is the ONLY
// class that calls palette::fromBackground / fromOnePiece / fromTwoPieces. It
// speaks palette:: value types; QColor conversion stays in the MainWindow.
namespace palette_widgets {

class PaletteController : public QObject {
    Q_OBJECT
public:
    enum class Mode { Background, OnePiece, TwoPieces };

    explicit PaletteController(QObject* parent = nullptr);

public slots:
    void setMode(Mode mode);
    void setHarmony(palette::Harmony harmony);
    void setLuminanceSpread(bool enabled);
    void setBackgroundColor(const palette::Srgb& color);
    void setPiece1Color(const palette::Srgb& color);
    void setPiece2Color(const palette::Srgb& color);
    void recompute();

signals:
    void paletteReady(const palette::Palette& result);

private:
    Mode mode_ = Mode::Background;
    palette::Harmony harmony_ = palette::Harmony::SplitComplement;
    palette::Constraints constraints_{};
    palette::Srgb background_{1.0, 1.0, 0.8667}; // beige RGB(255,255,221)
    palette::Srgb piece1_{1.0, 0.0, 0.0};
    palette::Srgb piece2_{0.0, 0.0, 1.0};
};

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
