// Copyright Ben Paul Wise. All Rights Reserved.
#include "BoardWidget.h"
#include "RectangularGraph.h"
#include <QMouseEvent>
#include <QPainter>
#include <QDebug>
#include <QString>
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
    loadTextures();
}

void BoardWidget::loadTextures() {
    for (int i = 1; i <= 7; ++i) {
        QPixmap pm(QString(":/stones/black%1.png").arg(i, 2, 10, QChar('0')));
        if (!pm.isNull()) blackSrc_.push_back(pm);
    }
    for (int i = 1; i <= 6; ++i) {
        QPixmap pm(QString(":/stones/white%1.png").arg(i, 2, 10, QChar('0')));
        if (!pm.isNull()) whiteSrc_.push_back(pm);
    }
    qDebug() << "BoardWidget::loadTextures:" << blackSrc_.size() << "black," << whiteSrc_.size() << "white";
}

void BoardWidget::rescaleTextures() {
    if (!useTexture_ || blackSrc_.empty() || whiteSrc_.empty()) return;
    int sz = std::max(1, static_cast<int>(stoneR_ * 2.0f));
    blackScaled_.clear();
    for (const auto& pm : blackSrc_)
        blackScaled_.push_back(
            pm.scaled(sz, sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    whiteScaled_.clear();
    for (const auto& pm : whiteSrc_)
        whiteScaled_.push_back(
            pm.scaled(sz, sz, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void BoardWidget::setUseTexture(bool on) {

    qDebug() << "setUseTexture" << on << "blackScaled size after rescale:"
             << (on ? blackScaled_.size() : 0);
    useTexture_ = on;
    if (on) rescaleTextures();
    else { blackScaled_.clear(); whiteScaled_.clear(); }
    update();
}

void BoardWidget::setGame(const IrrGo::Game* game) {
    game_         = game;
    hoverNode_    = -1;
    tentativeNode_= -1;
    suggestedNode_= -1;
    lastMoveNode_ = -1;
    showBlackDvr_         = false;
    showWhiteDvr_         = false;
    showNeighborhoodSize_ = false;
    showLabels_           = false;
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

void BoardWidget::setSearching(bool s) {
    searching_ = s;
    if (s) { hoverNode_ = -1; update(); }
}

void BoardWidget::setShowBlackDvr(bool show) {
    showBlackDvr_ = show;
    update();
}

void BoardWidget::setShowWhiteDvr(bool show) {
    showWhiteDvr_ = show;
    update();
}

void BoardWidget::setDvrRadius(int r) {
    dvrRadius_ = r;
    update();
}

void BoardWidget::showNeighborhoodSize() {
    showNeighborhoodSize_ = true;
    update();
}

void BoardWidget::hideNeighborhoodSize() {
    showNeighborhoodSize_ = false;
    update();
}

void BoardWidget::showLabels() {
    showLabels_ = true;
    update();
}

void BoardWidget::hideLabels() {
    showLabels_ = false;
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
    rescaleTextures();
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
                                     bool isBlack, QColor borderColor, int nodeId) const {
    const auto& scaled = isBlack ? blackScaled_ : whiteScaled_;
    if (useTexture_ && !scaled.empty()) {
        int idx = (nodeId >= 0 ? nodeId : 0) % static_cast<int>(scaled.size());
        p.drawPixmap(QPointF(pt.x() - stoneR_, pt.y() - stoneR_), scaled[idx]);
        // Overlay border ring only — no fill
        float penW = stoneR_ * 2.0f * 0.1f;
        float ellR = stoneR_ - penW * 0.5f;
        p.setPen(QPen(borderColor, penW));
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(pt, ellR, ellR);
        return;
    }
    float penW = stoneR_ * 2.0f * 0.1f;
    float ellR = stoneR_ - penW * 0.5f;
    p.setPen(QPen(borderColor, penW));
    p.setBrush(isBlack ? QColor(0, 0, 0) : QColor(255, 255, 255));
    p.drawEllipse(pt, ellR, ellR);
}

// ── Mouse events ──────────────────────────────────────────────────────────────

void BoardWidget::mouseMoveEvent(QMouseEvent* e) {
    if (searching_) return;
    int n = nodeAt(e->position());
    if (n != hoverNode_) { hoverNode_ = n; update(); }
}

void BoardWidget::mousePressEvent(QMouseEvent* e) {
    if (searching_) return;
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
    const auto* rg = dynamic_cast<const RectangularGraph*>(&game_->graph());

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

    // Non-rectangular hover: incident edges in medium purple
    if (!rg && hoverNode_ >= 0) {
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
        if (!rg) {
            for (const auto& nd : nodes)
                p.drawEllipse(toWidget(nd.x, nd.y), dotR, dotR);
        } else {
            int rows = rg->rows(), cols = rg->cols();
            int lr = starCoord(rows),  hr = rows - (1 + lr);
            int lc = starCoord(cols),  hc = cols - (1 + lc);
            for (int r : {lr, hr})
                for (int c : {lc, hc}) {
                    const auto& nd = nodes[rg->nodeId(r, c)];
                    p.drawEllipse(toWidget(nd.x, nd.y), dotR, dotR);
                }
            // If large enough, draw center + 4 side star points
            if (rows * cols >= 100) {
                int cr = rows / 2, cc = cols / 2;
                p.drawEllipse(toWidget(nodes[rg->nodeId(cr, cc)].x,
                                       nodes[rg->nodeId(cr, cc)].y), dotR, dotR);
                for (int r : {lr, hr})
                    p.drawEllipse(toWidget(nodes[rg->nodeId(r,  cc)].x,
                                           nodes[rg->nodeId(r,  cc)].y), dotR, dotR);
                for (int c : {lc, hc})
                    p.drawEllipse(toWidget(nodes[rg->nodeId(cr, c)].x,
                                           nodes[rg->nodeId(cr, c)].y), dotR, dotR);
            }
        }
    }

    if (showLabels_) {
        QFont f = p.font();
        float textScale = 0.6f;
        f.setPixelSize(qBound(4, qRound(stoneR_ * textScale), 9));
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        for (const auto& nd : nodes) {
            QPointF pt = toWidget(nd.x, nd.y);
            QRectF textRect(pt.x() - stoneR_, pt.y() - stoneR_,
                            stoneR_ * 2.0f, stoneR_ * 2.0f);
            p.drawText(textRect, Qt::AlignCenter, QString::fromStdString(nd.label));
        }
    } else if (showNeighborhoodSize_) {
        // For each node, count neighbours within Manhattan distance <= dvrRadius_
        int radius = dvrRadius_;
        QFont f = p.font();
        f.setPixelSize(qBound(8, qRound(stoneR_ * 0.95f), 18));
        f.setBold(true);
        p.setFont(f);
        p.setPen(Qt::white);
        for (const auto& nd : nodes) {
            int count = 0;
            for (const auto& other : nodes)
                if (qAbs(other.row - nd.row) + qAbs(other.col - nd.col) <= radius)
                    ++count;
            QPointF pt = toWidget(nd.x, nd.y);
            QRectF textRect(pt.x() - stoneR_, pt.y() - stoneR_,
                            stoneR_ * 2.0f, stoneR_ * 2.0f);
            p.drawText(textRect, Qt::AlignCenter, QString::number(count));
        }
    } else {
        // DVR overlay — drawn before stones so occupied intersections stay clean
        if (showBlackDvr_ || showWhiteDvr_) {
            int radius = dvrRadius_;
            float dotR = stoneR_ * 0.35f;
            p.setPen(Qt::NoPen);
            if (showBlackDvr_) {
                DVR blackDvr(*game_, Color::Black, radius);
                p.setBrush(Qt::black);
                for (int id : blackDvr.nodes())
                    p.drawEllipse(toWidget(nodes[id].x, nodes[id].y), dotR, dotR);
            }
            if (showWhiteDvr_) {
                DVR whiteDvr(*game_, Color::White, radius);
                p.setBrush(Qt::white);
                for (int id : whiteDvr.nodes())
                    p.drawEllipse(toWidget(nodes[id].x, nodes[id].y), dotR, dotR);
            }
        }

        // Stones
        for (const auto& nd : nodes) {
            Color c = game_->colorAt(nd.id);
            if (c == Color::Empty) continue;
            QPointF pt = toWidget(nd.x, nd.y);
            const auto& scaled = (c == Color::Black) ? blackScaled_ : whiteScaled_;
            if (useTexture_ && !scaled.empty()) {
                int idx = nd.id % static_cast<int>(scaled.size());
                p.drawPixmap(QPointF(pt.x() - stoneR_, pt.y() - stoneR_), scaled[idx]);
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(c == Color::Black ? Qt::black : Qt::white);
                p.drawEllipse(pt, stoneR_, stoneR_);
            }
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
                               suggestIsBlack_, kGreen, suggestedNode_);
        }

        // Tentative stone: player colour with grey cross-hatch
        if (tentativeNode_ >= 0) {
            QPointF pt = toWidget(nodes[tentativeNode_].x, nodes[tentativeNode_].y);
            bool isBlack = (game_->toMove() == Player::Black);
            const auto& scaled = isBlack ? blackScaled_ : whiteScaled_;
            if (useTexture_ && !scaled.empty()) {
                int idx = tentativeNode_ % static_cast<int>(scaled.size());
                p.drawPixmap(QPointF(pt.x() - stoneR_, pt.y() - stoneR_), scaled[idx]);
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(isBlack ? Qt::black : Qt::white);
                p.drawEllipse(pt, stoneR_, stoneR_);
            }
            p.setPen(Qt::NoPen);
            p.setBrush(QBrush(QColor(130, 130, 130), Qt::DiagCrossPattern));
            p.drawEllipse(pt, stoneR_, stoneR_);
        }

        // Hover effect
        if (hoverNode_ >= 0 && hoverNode_ != tentativeNode_) {
            QPointF pt = toWidget(nodes[hoverNode_].x, nodes[hoverNode_].y);
            if (game_->colorAt(hoverNode_) == Color::Empty) {
                paintStoneBordered(p, pt, game_->toMove() == Player::Black, kOrange, hoverNode_);
            } else {
                p.setPen(Qt::NoPen);
                p.setBrush(kRed);
                p.drawEllipse(pt, stoneR_ * 0.5f, stoneR_ * 0.5f);
            }
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
