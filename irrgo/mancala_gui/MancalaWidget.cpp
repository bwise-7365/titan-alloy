// Copyright Ben Paul Wise. All Rights Reserved.
#include "MancalaWidget.h"
#include <QMouseEvent>
#include <QPainter>

// ── Layout constants ──────────────────────────────────────────────────────────
namespace {
constexpr int kPitRadius = 28;   // fixed circle radius (pixels)
constexpr int kPitGap    = 6;    // gap between adjacent circles
constexpr int kStoreW    = 56;   // store width
constexpr int kRowGap    = 10;   // vertical gap between the two pit rows
constexpr int kMargin    = 12;   // border margin
constexpr int kLabelH    = 20;   // bottom status-label area height
}

MancalaWidget::MancalaWidget(QWidget* parent)
    : guicommon::BoardWidgetBase(parent)
{
}

void MancalaWidget::setGame(const Mancala::Game* game) {
    game_      = game;
    resetFeedback();   // clears the hover + last-move markers (base)
    suggested_ = -1;
    rebuildGeometry();
    update();
}

void MancalaWidget::setBgColor(QColor c) {
    bgColor_ = c;
    update();
}

void MancalaWidget::setSuggestion(int pitIndex) {
    suggested_ = pitIndex;
    update();
}

void MancalaWidget::clearSuggestion() {
    suggested_ = -1;
    update();
}

// ── Geometry ──────────────────────────────────────────────────────────────────

QSize MancalaWidget::preferredSize() const {
    int N    = game_ ? game_->numPits() : 6;
    int diam = 2 * kPitRadius;
    int w    = 2*kMargin + 2*kStoreW + N*diam + (N-1)*kPitGap;
    int h    = 2*kMargin + 2*diam  + kRowGap + kLabelH;
    return {w, h};
}

void MancalaWidget::rebuildGeometry() {
    int N = game_ ? game_->numPits() : 6;
    pitRects_.assign(game_ ? game_->totalSlots() : 14, QRectF{});

    constexpr int diam  = 2 * kPitRadius;
    int pitX0  = kMargin + kStoreW;
    int rowY0  = kMargin;
    int rowY1  = kMargin + diam + kRowGap;
    int storeH = 2*diam + kRowGap;

    // P0 pits (bottom row): indices 0 .. N-1, left to right.
    for (int i = 0; i < N; ++i)
        pitRects_[i] = QRectF(pitX0 + i*(diam + kPitGap), rowY1, diam, diam);

    // P1 pits (top row): indices 2N, 2N-1, ... N+1 — left to right.
    for (int i = 0; i < N; ++i)
        pitRects_[2*N - i] = QRectF(pitX0 + i*(diam + kPitGap), rowY0, diam, diam);

    // Stores span both rows.
    storeRect1_ = QRectF(kMargin,                               kMargin, kStoreW, storeH);
    storeRect0_ = QRectF(pitX0 + N*diam + (N-1)*kPitGap, kMargin, kStoreW, storeH);
    if (game_) {
        pitRects_[game_->p0Store()] = storeRect0_;
        pitRects_[game_->p1Store()] = storeRect1_;
    }
}

QRectF MancalaWidget::pitRect(int index) const {
    if (index < 0 || index >= static_cast<int>(pitRects_.size())) return {};
    return pitRects_[index];
}

int MancalaWidget::cellAt(const QPointF& pos) const {
    if (!game_) return -1;
    int N = game_->numPits();
    for (int i = 0; i < N; ++i)
        if (pitRects_[i].contains(pos)) return i;
    for (int i = N+1; i <= 2*N; ++i)
        if (pitRects_[i].contains(pos)) return i;
    return -1;
}

// ── Drawing ───────────────────────────────────────────────────────────────────

void MancalaWidget::drawPit(QPainter& p, int index) const {
    QRectF r = pitRects_[index];
    if (r.isNull()) return;

    int  stones   = game_ ? game_->pit(index) : 0;
    int  cp       = game_ ? game_->currentPlayer() : 0;
    int  N        = game_ ? game_->numPits() : 6;
    bool isOwn    = (cp == 0) ? (index >= 0 && index < N)
                               : (index > N && index <= 2*N);
    bool terminal = game_ && game_->isTerminal();

    // Pit fill colour.  P1 pits share the greenish hue of the P1 store.
    bool   isP1Pit = (index > N && index <= 2*N);
    QColor base    = isP1Pit ? QColor("#8BC8B0") : bgColor_;
    QColor fill;
    if (terminal || isSearching())    fill = base.darker(115);
    else if (isOwn && stones > 0)     fill = base.lighter(130);
    else                              fill = base.darker(110);

    // Border colour: suggestion > last move > hover > default.
    QColor border;
    double borderWidth = 1.5;
    if (index == suggested_) {
        border = QColor("#00AAFF");  borderWidth = 3.0;
    } else if (index == lastMoveCell()) {
        border = (cp == 1) ? Qt::white : Qt::black;  borderWidth = 3.0;
    } else if (index == hoverCell() && isOwn && stones > 0 && !terminal && !isSearching()) {
        border = QColor("#FFD700");  borderWidth = 2.5;
    } else {
        border = bgColor_.darker(140);
    }

    p.setPen(QPen(border, borderWidth));
    p.setBrush(fill);
    QRectF inner = r.adjusted(3, 3, -3, -3);
    p.drawEllipse(inner);

    QColor textColor = (fill.lightnessF() > 0.5) ? Qt::black : Qt::white;
    p.setPen(textColor);
    QFont font = p.font();
    font.setPointSizeF(qMax(7.0, qMin(inner.height() * 0.38, 18.0)));
    font.setBold(stones > 0);
    p.setFont(font);
    p.drawText(inner, Qt::AlignCenter, QString::number(stones));
}

void MancalaWidget::drawStore(QPainter& p, int player) const {
    QRectF r   = (player == 0) ? storeRect0_ : storeRect1_;
    int stones = game_ ? game_->storeOf(player) : 0;

    QColor fill = (player == 0) ? QColor("#C8A86B").lighter(110)
                                : QColor("#8BC8B0").lighter(110);
    if (isSearching()) fill = fill.darker(120);

    p.setPen(QPen(bgColor_.darker(150), 2.0));
    p.setBrush(fill);
    QRectF inner = r.adjusted(3, 3, -3, -3);
    p.drawRoundedRect(inner, 10, 10);

    QFont font = p.font();
    font.setPointSizeF(qMax(7.0, qMin(inner.width() * 0.35, 16.0)));
    font.setBold(true);
    p.setFont(font);
    p.setPen(Qt::black);

    QRectF labelRect(inner.x(), inner.y(), inner.width(), inner.height() * 0.4);
    QRectF countRect(inner.x(), inner.y() + inner.height() * 0.4,
                     inner.width(), inner.height() * 0.6);
    p.drawText(labelRect, Qt::AlignCenter, QString("P%1").arg(player));
    font.setPointSizeF(qMax(9.0, qMin(inner.width() * 0.48, 22.0)));
    p.setFont(font);
    p.drawText(countRect, Qt::AlignCenter, QString::number(stones));
}

void MancalaWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), bgColor_);

    double W = width(), H = height();
    int N = game_ ? game_->numPits() : 6;
    constexpr int diam = 2 * kPitRadius;
    QRectF board(kMargin + kStoreW - 4,            kMargin - 4,
                 N*(diam + kPitGap) - kPitGap + 8, 2*diam + kRowGap + 8);
    p.setPen(QPen(bgColor_.darker(160), 2));
    p.setBrush(bgColor_.darker(105));
    p.drawRoundedRect(board, 6, 6);

    if (!game_) return;
    drawStore(p, 0);
    drawStore(p, 1);
    for (int i = 0;   i <  N;       ++i) drawPit(p, i);
    for (int i = N+1; i <= 2*N;     ++i) drawPit(p, i);

    if (!game_->isTerminal()) {
        int cp = game_->currentPlayer();
        QString label = isSearching() ? "Thinking..."
                      : (cp == 0   ? "Player 0 (South) to move"
                                   : "Player 1 (North) to move");
        p.setPen(bgColor_.darker(200));
        QFont f = p.font();
        f.setPointSizeF(10);
        p.setFont(f);
        p.drawText(QRectF(0, H - 18, W, 16), Qt::AlignCenter, label);
    } else {
        int s0 = game_->storeOf(0), s1 = game_->storeOf(1);
        QString result = (s0 > s1) ? "Player 0 wins!"
                       : (s1 > s0) ? "Player 1 wins!" : "Draw!";
        result += QString("  (%1 – %2)").arg(s0).arg(s1);
        p.setPen(Qt::darkRed);
        QFont f = p.font();
        f.setPointSizeF(11);
        f.setBold(true);
        p.setFont(f);
        p.drawText(QRectF(0, H - 20, W, 18), Qt::AlignCenter, result);
    }
}

// ── Mouse ─────────────────────────────────────────────────────────────────────

void MancalaWidget::resizeEvent(QResizeEvent*) {
    rebuildGeometry();
}

void MancalaWidget::mousePressEvent(QMouseEvent* ev) {
    if (ev->button() != Qt::LeftButton) return;
    int idx = cellAt(ev->position());
    if (idx >= 0) emit moveRequested(idx);
}
// Copyright Ben Paul Wise. All Rights Reserved.
