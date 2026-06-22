// Copyright Ben Paul Wise. All Rights Reserved.
#include "RybColorWheel.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>

#include "ColorConvert.h"
#include "palette/conversion.h"

namespace palette_widgets {

namespace {

constexpr double kPi = std::numbers::pi;

// Wheel geometry and marker rendering.
constexpr double kWheelMarginPx = 8.0;          // inset from the widget's short side
constexpr double kWheelHueStepDeg = 3.0;        // coloured-wedge width
constexpr double kFullCircleDeg = 360.0;
constexpr double kQtTopOffsetDeg = 90.0;        // rotates hue 0 to the top (12 o'clock)
constexpr double kQtAnglePerDegree = 16.0;      // Qt pie angles are in 1/16 of a degree
constexpr double kMarkerRadiusFraction = 0.80;  // marker ring radius / wheel radius
constexpr double kMarkerRadiusPx = 9.0;
constexpr double kMarkerPenWidthPx = 2.0;
constexpr double kLabelPenWidthPx = 1.0;

// Screen position of a wheel hue: 0 deg at the top, increasing clockwise.
QPointF huePoint(double cx, double cy, double radius, double hueDeg) {
    const double rad = hueDeg * kPi / 180.0;
    return QPointF(cx + radius * std::sin(rad), cy - radius * std::cos(rad));
}

} // namespace

RybColorWheel::RybColorWheel(QWidget* parent) : QWidget(parent) {
    setMinimumSize(140, 140);
}

void RybColorWheel::setPalette(const QColor& background, const QColor& piece1,
                               const QColor& piece2) {
    background_ = background;
    piece1_ = piece1;
    piece2_ = piece2;
    update();
}

void RybColorWheel::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    const double side = std::min(width(), height()) - kWheelMarginPx;
    if (side <= 0.0) {
        return;
    }
    const double cx = width() / 2.0;
    const double cy = height() / 2.0;
    const double radius = side / 2.0;
    const QRectF box(cx - radius, cy - radius, side, side);

    // The coloured wheel depends only on geometry, not the palette. Regenerate it
    // only when the device-pixel size changes; otherwise reuse the cached pixmap
    // and just repaint the (palette-dependent) markers below.
    const qreal dpr = devicePixelRatioF();
    if (wheelCache_.size() != size() * dpr) {
        wheelCache_ = QPixmap(size() * dpr);
        wheelCache_.setDevicePixelRatio(dpr);
        wheelCache_.fill(Qt::transparent);

        QPainter wp(&wheelCache_);
        wp.setRenderHint(QPainter::Antialiasing, true);
        wp.setPen(Qt::NoPen);
        const double step = kWheelHueStepDeg;
        for (double hue = 0.0; hue < kFullCircleDeg; hue += step) {
            const palette::Srgb s = palette::srgbAtHsvRyb(
                palette::HsvRyb{hue + step / 2.0, 1.0, 1.0});
            wp.setBrush(toQColor(s));
            const int qtStart = static_cast<int>((kQtTopOffsetDeg - hue - step) * kQtAnglePerDegree);
            const int qtSpan = static_cast<int>(step * kQtAnglePerDegree);
            wp.drawPie(box, qtStart, qtSpan);
        }
    }
    painter.drawPixmap(0, 0, wheelCache_);

    // Overlay the three palette markers at their RYB-wheel hue angles.
    const std::array<QColor, 3> colors{background_, piece1_, piece2_};
    const std::array<const char*, 3> labels{"B", "1", "2"};
    const double markerR = radius * kMarkerRadiusFraction;
    for (int i = 0; i < 3; ++i) {
        const double hue = palette::hueOfSrgb(toSrgb(colors[i]));
        const QPointF p = huePoint(cx, cy, markerR, hue);
        painter.setBrush(colors[i]);
        painter.setPen(QPen(Qt::white, kMarkerPenWidthPx));
        painter.drawEllipse(p, kMarkerRadiusPx, kMarkerRadiusPx);
        painter.setPen(QPen(Qt::black, kLabelPenWidthPx));
        painter.drawText(QRectF(p.x() - kMarkerRadiusPx, p.y() - kMarkerRadiusPx,
                                2.0 * kMarkerRadiusPx, 2.0 * kMarkerRadiusPx),
                         Qt::AlignCenter, labels[i]);
    }
}

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
