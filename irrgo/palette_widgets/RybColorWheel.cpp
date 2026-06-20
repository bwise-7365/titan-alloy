// Copyright Ben Paul Wise. All Rights Reserved.
#include "RybColorWheel.h"

#include <algorithm>
#include <array>
#include <cmath>

#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>

#include "ColorConvert.h"
#include "palette/conversion.hpp"

namespace palette_widgets {

namespace {

constexpr double kPi = 3.14159265358979323846;

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

    const double side = std::min(width(), height()) - 8.0;
    if (side <= 0.0) {
        return;
    }
    const double cx = width() / 2.0;
    const double cy = height() / 2.0;
    const double radius = side / 2.0;
    const QRectF box(cx - radius, cy - radius, side, side);

    // Paint the wheel as colored wedges (hue 0 at top, clockwise).
    painter.setPen(Qt::NoPen);
    const double step = 3.0;
    for (double hue = 0.0; hue < 360.0; hue += step) {
        const palette::Srgb s = palette::srgbAtHsvRyb(
            palette::HsvRyb{hue + step / 2.0, 1.0, 1.0});
        painter.setBrush(toQColor(s));
        const int qtStart = static_cast<int>((90.0 - hue - step) * 16.0);
        const int qtSpan = static_cast<int>(step * 16.0);
        painter.drawPie(box, qtStart, qtSpan);
    }

    // Overlay the three palette markers at their RYB-wheel hue angles.
    const std::array<QColor, 3> colors{background_, piece1_, piece2_};
    const std::array<const char*, 3> labels{"B", "1", "2"};
    const double markerR = radius * 0.80;
    for (int i = 0; i < 3; ++i) {
        const double hue = palette::hueOfSrgb(toSrgb(colors[i]));
        const QPointF p = huePoint(cx, cy, markerR, hue);
        painter.setBrush(colors[i]);
        painter.setPen(QPen(Qt::white, 2.0));
        painter.drawEllipse(p, 9.0, 9.0);
        painter.setPen(QPen(Qt::black, 1.0));
        painter.drawText(QRectF(p.x() - 9.0, p.y() - 9.0, 18.0, 18.0),
                         Qt::AlignCenter, labels[i]);
    }
}

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
