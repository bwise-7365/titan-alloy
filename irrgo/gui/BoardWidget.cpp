// Copyright Ben Paul Wise. All Rights Reserved.
#include "BoardWidget.h"
#include "IrregularGraph.h"
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <algorithm>
#include <limits>

static const QColor kOrange    { 255, 140,   0 };
static const QColor kMedPurple { 147, 112, 219 };
static const QColor kRed       { 220,   0,   0, 180 };
static constexpr float kStoneRPhys = 0.875f / 2.0f;

BoardWidget::BoardWidget(QWidget* parent) : QWidget(parent) {
    setMinimumSize(400, 400);
    setMouseTracking(true);
    confirmTimer_ = new QTimer(this);
    confirmTimer_->setSingleShot(true);
    confirmTimer_->setInterval(3000);
    connect(confirmTimer_, &QTimer::timeout, this, [this]() {
        tentativeNode_ = -1;
        update();
    });
}

void BoardWidget::setGame(const Game* game) {
    game_         = game;
    hoverNode_    = -1;
    tentativeNode_= -1;
    confirmTimer_->stop();
    updateTransform();
    update();
}

void BoardWidget::setBgColor(QColor c) {
    bgColor_ = c;
    update();
}

void BoardWidget::resizeEvent(QResizeEvent*) {
    updateTransform();
}

void BoardWidget::leaveEvent(QEvent*) {
    hoverNode_ = -1;
    update();
}

void BoardWidget::updateTransform() {
    if (!game_ || game_->graph().nodeCount() == 0) return;
    const auto& nodes = game_->graph().nodes();

    minX_ = minY_ =  std::numeric_limits<float>::max();
    float maxX    =  std::numeric_limits<float>::lowest();
    float maxY    =  std::numeric_limits<float>::lowest();
    for (const auto& nd : nodes) {
        minX_ = std::min(minX_, nd.x);  minY_ = std::min(minY_, nd.y);
        maxX  = std::max(maxX,  nd.x);  maxY  = std::max(maxY,  nd.y);
    }
    rangeX_ = (maxX - minX_ < 1e-4f) ? 1.0f : maxX - minX_;
    rangeY_ = (maxY - minY_ < 1e-4f) ? 1.0f : maxY - minY_;

    // Pad by one stone radius on each side so stones at edges aren't clipped
    float paddedRX = rangeX_ + 4.0f * kStoneRPhys;  // 2× stone radius margin each side
    float paddedRY = rangeY_ + 4.0f * kStoneRPhys;
    const float kExtra = 10.0f;
    scale_ = std::min((width()  - 2.0f * kExtra) / paddedRX,
                      (height() - 2.0f * kExtra) / paddedRY);
    if (scale_ < 1.0f) scale_ = 1.0f;
    stoneR_ = kStoneRPhys * scale_;

    float usedW = paddedRX * scale_;
    float usedH = paddedRY * scale_;
    offX_ = (width()  - usedW) / 2.0f + 2.0f * stoneR_;
    offY_ = (height() - usedH) / 2.0f + 2.0f * stoneR_;
}

int BoardWidget::nodeAt(QPointF pos) const {
    if (!game_) return -1;
    float r2   = stoneR_ * stoneR_;
    int   found = -1;
    float best  = r2 + 1.0f;
    for (const auto& nd : game_->graph().nodes()) {
        float px = offX_ + (nd.x - minX_) * scale_;
        float py = offY_ + (nd.y - minY_) * scale_;
        float dx = static_cast<float>(pos.x()) - px;
        float dy = static_cast<float>(pos.y()) - py;
        float d2 = dx*dx + dy*dy;
        if (d2 < best) { best = d2; found = nd.id; }
    }
    return (best <= r2) ? found : -1;
}

void BoardWidget::mouseMoveEvent(QMouseEvent* e) {
    int n = nodeAt(e->position());
    if (n != hoverNode_) { hoverNode_ = n; update(); }
}

void BoardWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton || !game_) return;
    int n = nodeAt(e->position());
    if (n < 0 || game_->colorAt(n) != Color::Empty) return;

    if (n == tentativeNode_) {
        confirmTimer_->stop();
        tentativeNode_ = -1;
        emit moveRequested(n);
    } else {
        tentativeNode_ = n;
        confirmTimer_->start();
    }
    update();
}

void BoardWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), bgColor_);

    if (!game_ || game_->graph().nodeCount() == 0) return;

    const auto& nodes = game_->graph().nodes();
    bool isIrr = dynamic_cast<const IrregularGraph*>(&game_->graph()) != nullptr;

    auto toWidget = [&](float x, float y) -> QPointF {
        return { offX_ + (x - minX_) * scale_,
                 offY_ + (y - minY_) * scale_ };
    };

    // Board frame
    QRectF fr(offX_ - 2.0f * stoneR_, offY_ - 2.0f * stoneR_,
              rangeX_ * scale_ + 4.0f * stoneR_,
              rangeY_ * scale_ + 4.0f * stoneR_);
    p.setPen(QPen(Qt::black, 4.0));
    p.setBrush(Qt::NoBrush);
    p.drawRect(fr);

    // Edges (normal)
    p.setPen(QPen(lineColor_, 1.5));
    for (const auto& nd : nodes)
        for (int nb : nd.neighbors)
            if (nb > nd.id)
                p.drawLine(toWidget(nd.x, nd.y),
                           toWidget(nodes[nb].x, nodes[nb].y));

    // Irregular hover: incident edges highlighted in medium purple (drawn over normal)
    if (isIrr && hoverNode_ >= 0) {
        p.setPen(QPen(kMedPurple, 2.5));
        const auto& hn = nodes[hoverNode_];
        for (int nb : hn.neighbors)
            p.drawLine(toWidget(hn.x, hn.y), toWidget(nodes[nb].x, nodes[nb].y));
    }

    // Node dots
    float dotR = std::max(2.0f, stoneR_ * 0.15f);
    p.setPen(Qt::NoPen);
    p.setBrush(lineColor_);
    for (const auto& nd : nodes)
        p.drawEllipse(toWidget(nd.x, nd.y), dotR, dotR);

    // Stones
    for (const auto& nd : nodes) {
        Color c = game_->colorAt(nd.id);
        if (c == Color::Empty) continue;
        p.setPen(Qt::NoPen);
        p.setBrush(c == Color::Black ? Qt::black : Qt::white);
        p.drawEllipse(toWidget(nd.x, nd.y), stoneR_, stoneR_);
    }

    // Tentative stone: player color with grey cross-hatch overlay
    if (tentativeNode_ >= 0) {
        QPointF pt = toWidget(nodes[tentativeNode_].x, nodes[tentativeNode_].y);
        bool isBlack = (game_->currentPlayer() == Player::Black);
        p.setPen(Qt::NoPen);
        p.setBrush(isBlack ? Qt::black : Qt::white);
        p.drawEllipse(pt, stoneR_, stoneR_);
        p.setBrush(QBrush(QColor(130, 130, 130), Qt::DiagCrossPattern));
        p.drawEllipse(pt, stoneR_, stoneR_);
    }

    // Hover effect on any node other than the tentative one
    if (hoverNode_ >= 0 && hoverNode_ != tentativeNode_) {
        QPointF pt = toWidget(nodes[hoverNode_].x, nodes[hoverNode_].y);
        if (game_->colorAt(hoverNode_) == Color::Empty) {
            // Preview stone: player color + orange border (border = 1/10 diameter)
            float penW = stoneR_ * 2.0f * 0.1f;
            float ellR = stoneR_ - penW * 0.5f;
            bool isBlack = (game_->currentPlayer() == Player::Black);
            p.setPen(QPen(kOrange, penW));
            p.setBrush(isBlack ? QColor(0, 0, 0) : QColor(255, 255, 255));
            p.drawEllipse(pt, ellR, ellR);
        } else {
            // Occupied: small red disc to signal illegal (half stone diameter)
            p.setPen(Qt::NoPen);
            p.setBrush(kRed);
            p.drawEllipse(pt, stoneR_ * 0.5f, stoneR_ * 0.5f);
        }
    }
}
