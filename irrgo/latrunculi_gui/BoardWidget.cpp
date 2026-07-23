// Copyright Ben Paul Wise. All Rights Reserved.
#include "BoardWidget.h"

#include <QByteArray>
//#include <QMouseEvent>
#include <QPainter>
#include <QResizeEvent>
#include <QSvgRenderer>
#include <algorithm>

namespace gb = games::board;

BoardWidget::BoardWidget(QWidget* parent) : guicommon::BoardWidgetBase(parent) {
    style_  = gb::default_svg_style();
    render_ = gb::default_render_config();  // fixed noise seed -> stable look
}

void BoardWidget::setGame(const Latrunculi::Game* game) {
    game_ = game;
    clearSelection();
    clearSuggestion();
    resetFeedback();  // clear the hover + last-move markers (base)
    rebuild();
    update();
}

void BoardWidget::setSideColors(const QColor& a, const QColor& b) {
    colorA_ = a;
    colorB_ = b;
    rebuild();
    update();
}

void BoardWidget::setBackgroundColor(const QColor& c) {
    style_.background = c.name().toStdString();
    rebuild();
    update();
}

void BoardWidget::clearSelection() {
    pendingRemove_ = -1;
    selectedFrom_  = -1;
    destinations_.clear();
}

void BoardWidget::clearSuggestion() {
    suggestion_     = AbsGame::kPass;
    suggestionFrom_ = -1;
    suggestionTo_   = -1;
    update();
}

void BoardWidget::setSuggestion(AbsGame::MoveId mv) {
    suggestion_     = mv;
    suggestionFrom_ = -1;
    suggestionTo_   = -1;
    if (game_ && mv >= 0 && game_->phase() == Latrunculi::Phase::Movement) {
        int rem = -1, from = -1, to = -1;
        game_->decodeMovement(mv, rem, from, to);
        suggestionFrom_ = from;
        suggestionTo_   = to;
    }
    update();
}

// ── Rendering ─────────────────────────────────────────────────────────────────

void BoardWidget::rebuild() {
    cache_ = QPixmap();
    if (!game_) {
        return;
    }
    const int cols = game_->columns();
    const int rows = game_->rows();

    look_.grid.rows    = rows;
    look_.grid.columns = cols;
    gb::apply_draw_defaults(look_);

    // Choose a pixel scale so the whole diagram (board + margins) fits the widget.
    const double unitsW = cols + 2.0 * style_.margin_units;
    const double unitsH = rows + 2.0 * style_.margin_units;
    double scale = std::min(width() / unitsW, height() / unitsH);
    if (scale < 1.0) {
        scale = 1.0;  // guard against a degenerate (near-zero) widget size
    }
    render_.square_size = scale;

    // Build the placed-piece list from the current position.
    std::vector<gb::PlacedPiece> pieces;
    pieces.reserve(static_cast<std::size_t>(game_->squareCount()));
    for (int s = 0; s < game_->squareCount(); ++s) {
        const int owner = game_->ownerAt(s);
        if (owner < 0) {
            continue;
        }
        const Latrunculi::Cell c = game_->cellAt(s);
        const bool bound =
            (c == Latrunculi::Cell::P0Bound || c == Latrunculi::Cell::P1Bound);
        const std::string fill =
            (owner == 0) ? colorA_.name().toStdString() : colorB_.name().toStdString();
        pieces.push_back(gb::PlacedPiece{s, fill, bound, owner});
    }

    // Immobilisation "X" is drawn in the opponent's color (shows who immobilised it).
    look_.mark_color_p0 = colorB_.name().toStdString();  // X on an A disc = B's color
    look_.mark_color_p1 = colorA_.name().toStdString();  // X on a  B disc = A's color
    const std::string svg = gb::generate_position_svg(look_, pieces, render_, style_);
    QSvgRenderer renderer(QByteArray::fromStdString(svg));
    const QSize svgSize = renderer.defaultSize();
    QPixmap pm(svgSize);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing, true);
    renderer.render(&p);
    p.end();
    cache_ = pm;

    scale_  = scale;
    margin_ = style_.margin_units * scale;
    offX_   = (width()  - svgSize.width())  / 2.0;
    offY_   = (height() - svgSize.height()) / 2.0;
}

void BoardWidget::resizeEvent(QResizeEvent* e) {
    rebuild();
    QWidget::resizeEvent(e);
}

QPointF BoardWidget::squareCenter(int square) const {
    const int cols = game_->columns();
    const int col = square % cols;
    const int row = square / cols;
    return QPointF(offX_ + margin_ + (col + 0.5) * scale_,
                   offY_ + margin_ + (row + 0.5) * scale_);
}

int BoardWidget::cellAt(const QPointF& pos) const {
    if (!game_ || scale_ <= 0.0) {
        return -1;
    }
    const double x = pos.x() - offX_ - margin_;
    const double y = pos.y() - offY_ - margin_;
    if (x < 0.0 || y < 0.0) {
        return -1;
    }
    const int col = static_cast<int>(x / scale_);
    const int row = static_cast<int>(y / scale_);
    if (col < 0 || col >= game_->columns() || row < 0 || row >= game_->rows()) {
        return -1;
    }
    return row * game_->columns() + col;
}

void BoardWidget::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.fillRect(rect(), latgui::kBoardSurround);  // dark surround behind the board
    if (cache_.isNull()) {
        return;
    }
    p.drawPixmap(QPointF(offX_, offY_), cache_);

    auto ringAt = [&](int square, const QColor& col, double widthPx) {
        const QPointF c = squareCenter(square);
        const double r = latgui::kRingRadiusFrac * scale_;
        QPen pen(col);
        pen.setWidthF(widthPx);
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawEllipse(c, r, r);
    };

    // Suggestion (drawn under the interactive selection).
    if (suggestionFrom_ >= 0) {
        ringAt(suggestionFrom_, latgui::kSuggestionRing, latgui::kThinPenPx);
    }
    if (suggestionTo_ >= 0) {
        ringAt(suggestionTo_, latgui::kSuggestionRing, latgui::kThinPenPx);
    }
    if (suggestionFrom_ < 0 && suggestion_ >= 0) {  // placement suggestion (one square)
        ringAt(suggestion_, latgui::kSuggestionRing, latgui::kThinPenPx);
    }

    // Mandatory removal cue: while a captive must be removed but none is chosen yet, ring
    // every enemy Bound disc so it is clear the first click has to land on one of them (an
    // own-disc click is ignored until then). Once one is picked, only that one stays ringed.
    if (!isSearching() && !game_->isOver()
        && game_->phase() == Latrunculi::Phase::Movement
        && currentHasCaptives() && pendingRemove_ < 0) {
        const int cur = game_->currentPlayer();
        const Latrunculi::Cell enemyBound =
            (cur == 0) ? Latrunculi::Cell::P1Bound : Latrunculi::Cell::P0Bound;
        const int squares = game_->rows() * game_->columns();
        for (int sq = 0; sq < squares; ++sq) {
            if (game_->cellAt(sq) == enemyBound) {
                ringAt(sq, latgui::kCaptiveRing, latgui::kThickPenPx);
            }
        }
    }

    // Interactive selection.
    if (pendingRemove_ >= 0) {
        ringAt(pendingRemove_, latgui::kCaptiveRing, latgui::kThickPenPx);  // captive to remove
    }
    if (selectedFrom_ >= 0) {
        ringAt(selectedFrom_, latgui::kSelectionBlue, latgui::kThickPenPx);  // selected origin
    }
    QColor destFill = latgui::kSelectionBlue;
    destFill.setAlpha(latgui::kOverlayAlpha);
    for (int d : destinations_) {
        const QPointF c = squareCenter(d);
        p.setPen(Qt::NoPen);
        p.setBrush(destFill);
        p.drawEllipse(c, latgui::kDestinationDotFrac * scale_, latgui::kDestinationDotFrac * scale_);
    }

    // Last-move dot: a small mark on the disc that last moved / was placed,
    // contrasting with its color (white on a dark piece, near-black on a light one).
    const int lm = lastMoveCell();
    if (lm >= 0 && game_->ownerAt(lm) >= 0) {
        const QColor piece = (game_->ownerAt(lm) == 0) ? colorA_ : colorB_;
        const QColor dot = (piece.lightnessF() < latgui::kPieceDarkThreshold)
                               ? latgui::kLastMoveDotLight : latgui::kLastMoveDotDark;
        p.setPen(Qt::NoPen);
        p.setBrush(dot);
        p.drawEllipse(squareCenter(lm), latgui::kLastMoveDotFrac * scale_,
                      latgui::kLastMoveDotFrac * scale_);
    }

    // Hover ghost: a translucent disc in the to-move side's color with a bright
    // outline, previewing where a click would act.
    const int hv = hoverCell();
    if (!isSearching() && hv >= 0 && !game_->isOver()) {
        bool show = false;
        if (game_->phase() == Latrunculi::Phase::Placement) {
            show = game_->isLegalMove(game_->placementMove(hv));  // only legal squares
        } else if (selectedFrom_ >= 0) {
            show = std::find(destinations_.begin(), destinations_.end(), hv)
                   != destinations_.end();  // movement: preview the landing
        }
        if (show) {
            QColor fill = (game_->currentPlayer() == 0) ? colorA_ : colorB_;
            fill.setAlpha(latgui::kOverlayAlpha);
            QPen pen(latgui::kHoverOutline);
            pen.setWidthF(latgui::kThinPenPx);
            p.setPen(pen);
            p.setBrush(fill);
            p.drawEllipse(squareCenter(hv), latgui::kHoverDiscFrac * scale_,
                          latgui::kHoverDiscFrac * scale_);
        }
    }
}

// ── Interaction ───────────────────────────────────────────────────────────────

bool BoardWidget::currentHasCaptives() const {
    // Enemy captives = the opponent's immobilised (Bound) discs.
    return game_->boundDiscs(1 - game_->currentPlayer()) > 0;
}

std::vector<int> BoardWidget::legalDestinations(int from) const {
    std::vector<int> dests;
    if (!game_ || game_->phase() != Latrunculi::Phase::Movement) {
        return dests;
    }
    for (AbsGame::MoveId mv : game_->getLegalMoves()) {
        int rem = -1, f = -1, t = -1;
        game_->decodeMovement(mv, rem, f, t);
        if (f == from && rem == pendingRemove_) {
            dests.push_back(t);
        }
    }
    return dests;
}

void BoardWidget::mousePressEvent(QMouseEvent* e) {
    if (!game_ || isSearching() || game_->isOver() || e->button() != Qt::LeftButton) {
        return;
    }
    const int sq = cellAt(e->position());
    if (sq < 0) {
        clearSelection();
        update();
        return;
    }
    if (game_->phase() == Latrunculi::Phase::Placement) {
        handlePlacementClick(sq);
    } else {
        handleMovementClick(sq);
    }
}

void BoardWidget::handlePlacementClick(int square) {
    const AbsGame::MoveId mv = game_->placementMove(square);
    if (game_->isLegalMove(mv)) {
        emit moveRequested(mv);
    }
}

void BoardWidget::handleMovementClick(int square) {
    const int cur = game_->currentPlayer();
    const Latrunculi::Cell c = game_->cellAt(square);
    const bool mustRemove = currentHasCaptives();
    const bool enemyBound = (cur == 0 && c == Latrunculi::Cell::P1Bound)
                         || (cur == 1 && c == Latrunculi::Cell::P0Bound);
    const bool ownFree = (cur == 0 && c == Latrunculi::Cell::P0Free)
                      || (cur == 1 && c == Latrunculi::Cell::P1Free);

    // Mandatory removal: clicking an enemy Bound disc (re)selects the captive.
    if (mustRemove && enemyBound) {
        pendingRemove_ = square;
        selectedFrom_  = -1;
        destinations_.clear();
        update();
        return;
    }
    // If removal is required but not yet chosen, from/to clicks are ignored.
    if (mustRemove && pendingRemove_ < 0) {
        return;
    }
    // Select an own Free disc as the origin.
    if (ownFree) {
        selectedFrom_ = square;
        destinations_ = legalDestinations(square);
        update();
        return;
    }
    // Complete the move on a highlighted destination.
    if (selectedFrom_ >= 0) {
        for (int d : destinations_) {
            if (d == square) {
                emit moveRequested(
                    game_->movementMove(selectedFrom_, square, pendingRemove_));
                return;  // MainWindow refreshes via setGame(), resetting selection
            }
        }
    }
    // Anything else clears the origin selection (the captive choice is kept).
    selectedFrom_ = -1;
    destinations_.clear();
    update();
}
// Copyright Ben Paul Wise. All Rights Reserved.
