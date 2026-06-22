// Copyright Ben Paul Wise. All Rights Reserved.
#include "BoardPreview.h"

#include <QBrush>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>

namespace palette_widgets {

namespace {
constexpr double kMarginPx = 6.0;             // inset around the board
constexpr double kBoardAspect = 3.0 / 2.0;    // board width : height
constexpr double kPieceRadiusFraction = 0.28; // of board height
constexpr double kLeftPieceXFraction = 0.30;
constexpr double kRightPieceXFraction = 0.70;
constexpr double kPieceYFraction = 0.5;       // vertically centred
constexpr double kBorderPenWidthPx = 1.0;
const QColor kBoardBorderColor{"#555555"};
const QColor kPieceBorderColor{"#333333"};
}  // namespace

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
    const double margin = kMarginPx;
    const double availW = width() - 2.0 * margin;
    const double availH = height() - 2.0 * margin;
    if (availW <= 0.0 || availH <= 0.0) {
        return;
    }
    double boardW = availW;
    double boardH = boardW / kBoardAspect;
    if (boardH > availH) {
        boardH = availH;
        boardW = boardH * kBoardAspect;
    }
    const double boardX = (width() - boardW) / 2.0;
    const double boardY = (height() - boardH) / 2.0;
    const QRectF board(boardX, boardY, boardW, boardH);

    // Background (the board).
    painter.setPen(QPen(kBoardBorderColor, kBorderPenWidthPx));
    painter.setBrush(background_);
    painter.drawRect(board);

    // Two piece circles, sized as on a 3:2 board (radius ~0.28 of the height),
    // spaced left/right so they don't overlap.
    const double radius = boardH * kPieceRadiusFraction;
    const QPointF c1(boardX + boardW * kLeftPieceXFraction, boardY + boardH * kPieceYFraction);
    const QPointF c2(boardX + boardW * kRightPieceXFraction, boardY + boardH * kPieceYFraction);

    painter.setPen(QPen(kPieceBorderColor, kBorderPenWidthPx));
    painter.setBrush(piece1_);
    painter.drawEllipse(c1, radius, radius);
    painter.setBrush(piece2_);
    painter.drawEllipse(c2, radius, radius);
}

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
