// Copyright Ben Paul Wise. All Rights Reserved.
#include "BoardPreview.h"

#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>

namespace palette_widgets {

BoardPreview::BoardPreview(QWidget* parent) : QWidget(parent) {
    setMinimumSize(150, 100); // 3:2
}

void BoardPreview::setColors(const QColor& background, const QColor& piece1,
                             const QColor& piece2) {
    background_ = background;
    piece1_ = piece1;
    piece2_ = piece2;
    update();
}

void BoardPreview::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    // Largest 3:2 rectangle that fits, centered, with a small margin.
    const double margin = 6.0;
    const double availW = width() - 2.0 * margin;
    const double availH = height() - 2.0 * margin;
    if (availW <= 0.0 || availH <= 0.0) {
        return;
    }
    double boardW = availW;
    double boardH = boardW * 2.0 / 3.0;
    if (boardH > availH) {
        boardH = availH;
        boardW = boardH * 3.0 / 2.0;
    }
    const double boardX = (width() - boardW) / 2.0;
    const double boardY = (height() - boardH) / 2.0;
    const QRectF board(boardX, boardY, boardW, boardH);

    // Background (the board).
    painter.setPen(QPen(QColor("#555555"), 1.0));
    painter.setBrush(background_);
    painter.drawRect(board);

    // Two piece circles, sized as on a 3:2 board (radius ~0.28 of the height),
    // spaced left/right so they don't overlap.
    const double radius = boardH * 0.28;
    const QPointF c1(boardX + boardW * 0.30, boardY + boardH * 0.5);
    const QPointF c2(boardX + boardW * 0.70, boardY + boardH * 0.5);

    painter.setPen(QPen(QColor("#333333"), 1.0));
    painter.setBrush(piece1_);
    painter.drawEllipse(c1, radius, radius);
    painter.setBrush(piece2_);
    painter.drawEllipse(c2, radius, radius);
}

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
