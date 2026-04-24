// Copyright Ben Paul Wise. All Rights Reserved.
#include "BoardWidget.h"
#include "IrregularGraph.h"
#include "RectangularGraph.h"
#include <QMouseEvent>
#include <QPainter>
#include <QTimer>
#include <algorithm>
#include <limits>

using namespace IrrGo;

static int starCoord(int n) { return n < 11 ? 2 : 3; }

static const QColor kOrange    { 255, 140,   0 };
static const QColor kGreen     {   0, 200,   0 };
static const QColor kMedPurple { 147, 112, 219 };
static const QColor kDarkBlue  { 0, 0, 128 };
static const QColor kRed       { 220,   0,   0, 180 };
static constexpr float kStoneRPhys      = 0.875f / 2.0f;
static constexpr float kGridLineThickness = 2.5f; // normally 1.5f;

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

void BoardWidget::setGame(const IrrGo::Game* game) {
    game_         = game;
    hoverNode_    = -1;
    tentativeNode_= -1;
    suggestedNode_= -1;
    lastMoveNode_ = -1;
    confirmTimer_->stop();
    updateTransform();
    update();
}

void BoardWidget::setSuggestion(int nodeId, bool isBlack) {
    suggestedNode_  = nodeId;
    suggestIsBlack_ = isBlack;
    update();
}

void BoardWidget::clearSuggestion() {
    suggestedNode_ = -1;
    update();
}

void BoardWidget::setBoardInfo(const QString& info) {
    boardInfoRight_ = info;
    update();
}

void BoardWidget::setLastMove(int nodeId) {
    lastMoveNode_ = nodeId;
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

    float paddedRX = rangeX_ + 4.0f * kStoneRPhys;
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

// ── Shared bordered-stone helper ──────────────────────────────────────────────

void BoardWidget::paintStoneBordered(QPainter& p, QPointF pt,
                                     bool isBlack, QColor borderColor) const {
    float penW = stoneR_ * 2.0f * 0.1f;
    float ellR = stoneR_ - penW * 0.5f;
    p.setPen(QPen(borderColor, penW));
    p.setBrush(isBlack ? QColor(0, 0, 0) : QColor(255, 255, 255));
    p.drawEllipse(pt, ellR, ellR);
}

// ── Mouse events ──────────────────────────────────────────────────────────────

void BoardWidget::mouseMoveEvent(QMouseEvent* e) {
    int n = nodeAt(e->position());
    if (n != hoverNode_) { hoverNode_ = n; update(); }
}

void BoardWidget::mousePressEvent(QMouseEvent* e) {
    if (e->button() != Qt::LeftButton || !game_) return;
    emit clearSuggestionRequested();
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

// ── Paint ─────────────────────────────────────────────────────────────────────

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
    p.setPen(QPen(lineColor_, kGridLineThickness));
    for (const auto& nd : nodes)
        for (int nb : nd.neighbors)
            if (nb > nd.id)
                p.drawLine(toWidget(nd.x, nd.y),
                           toWidget(nodes[nb].x, nodes[nb].y));

    // Irregular hover: incident edges in medium purple
    if (isIrr && hoverNode_ >= 0) {
        p.setPen(QPen(kMedPurple, 2.5)); // normally kMedPurple kDarkBlue
        const auto& hn = nodes[hoverNode_];
        for (int nb : hn.neighbors)
            p.drawLine(toWidget(hn.x, hn.y), toWidget(nodes[nb].x, nodes[nb].y));
    }

    // Node dots: all nodes for irregular; corner star points for rectangular
    {
        float dotR = std::max(2.0f, stoneR_ * 0.3f);
        p.setPen(Qt::NoPen);
        p.setBrush(lineColor_);
        if (isIrr) {
            for (const auto& nd : nodes)
                p.drawEllipse(toWidget(nd.x, nd.y), dotR, dotR);
        } else {
            const auto* rg = static_cast<const RectangularGraph*>(&game_->graph());
            int rows = rg->rows(), cols = rg->cols();
            int lr = starCoord(rows),  hr = rows - (1 + lr);
            int lc = starCoord(cols),  hc = cols - (1 + lc);
            for (int r : {lr, hr})
                for (int c : {lc, hc}) {
                    const auto& nd = nodes[rg->nodeId(r, c)];
                    p.drawEllipse(toWidget(nd.x, nd.y), dotR, dotR);
                }
            if (rows * cols >= 100) {
                const auto& nd = nodes[rg->nodeId(rows / 2, cols / 2)];
                p.drawEllipse(toWidget(nd.x, nd.y), dotR, dotR);
            }
        }
    }

    // Stones
    for (const auto& nd : nodes) {
        Color c = game_->colorAt(nd.id);
        if (c == Color::Empty) continue;
        p.setPen(Qt::NoPen);
        p.setBrush(c == Color::Black ? Qt::black : Qt::white);
        p.drawEllipse(toWidget(nd.x, nd.y), stoneR_, stoneR_);
    }

    // Last-move marker: small contrasting dot on the most recently placed stone
    if (lastMoveNode_ >= 0 && lastMoveNode_ < static_cast<int>(nodes.size())) {
        Color lmc = game_->colorAt(lastMoveNode_);
        if (lmc != Color::Empty) {
            float dotR = stoneR_ * 0.28f;
            p.setPen(Qt::NoPen);
            p.setBrush(lmc == Color::Black ? Qt::white : lineColor_);
            p.drawEllipse(toWidget(nodes[lastMoveNode_].x, nodes[lastMoveNode_].y), dotR, dotR);
        }
    }

    // Suggested move — green bordered disc (drawn over any stone on that node)
    if (suggestedNode_ >= 0 && suggestedNode_ < static_cast<int>(nodes.size())) {
        paintStoneBordered(p, toWidget(nodes[suggestedNode_].x, nodes[suggestedNode_].y),
                           suggestIsBlack_, kGreen);
    }

    // Tentative stone: player colour with grey cross-hatch
    if (tentativeNode_ >= 0) {
        QPointF pt = toWidget(nodes[tentativeNode_].x, nodes[tentativeNode_].y);
        bool isBlack = (game_->toMove() == Player::Black);
        p.setPen(Qt::NoPen);
        p.setBrush(isBlack ? Qt::black : Qt::white);
        p.drawEllipse(pt, stoneR_, stoneR_);
        p.setBrush(QBrush(QColor(130, 130, 130), Qt::DiagCrossPattern));
        p.drawEllipse(pt, stoneR_, stoneR_);
    }

    // Hover effect
    if (hoverNode_ >= 0 && hoverNode_ != tentativeNode_) {
        QPointF pt = toWidget(nodes[hoverNode_].x, nodes[hoverNode_].y);
        if (game_->colorAt(hoverNode_) == Color::Empty) {
            // Orange-bordered preview — shares paintStoneBordered
            paintStoneBordered(p, pt, game_->toMove() == Player::Black, kOrange);
        } else {
            // Occupied: small red disc
            p.setPen(Qt::NoPen);
            p.setBrush(kRed);
            p.drawEllipse(pt, stoneR_ * 0.5f, stoneR_ * 0.5f);
        }
    }

    // Corner labels — below the grid, inside the board frame
    {
        float bandTop = offY_ + rangeY_ * scale_;   // bottom of lowest grid row
        float bandH   = static_cast<float>(fr.bottom()) - bandTop;
        int   px      = qBound(8, qRound(stoneR_ * 0.8f), 16);
        QFont f       = p.font();
        f.setPixelSize(px);
        f.setBold(true);
        p.setFont(f);
        p.setPen(lineColor_);

        float margin = stoneR_ * 0.35f;
        QRectF leftR (fr.left() + margin,                      bandTop,
                      fr.width() * 0.5f - margin,              bandH);
        QRectF rightR(fr.left() + fr.width() * 0.5f,           bandTop,
                      fr.width() * 0.5f - margin,              bandH);

        p.drawText(leftR,  Qt::AlignLeft  | Qt::AlignVCenter, "IrrGo");
        if (!boardInfoRight_.isEmpty())
            p.drawText(rightR, Qt::AlignRight | Qt::AlignVCenter, boardInfoRight_);
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.
