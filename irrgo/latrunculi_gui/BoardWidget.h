// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include "BoardWidgetBase.h"
#include "Game.h"            // Latrunculi::Game, Cell, Phase
#include "draw_params.h"     // RenderConfig, SvgStyle
#include "irregular_grid.h"  // BoardSpec, generate_position_svg, PlacedPiece

#include <QColor>
#include <QPixmap>
#include <string>
#include <vector>

// Draws a Latrunculi position by reusing the irregular_grids SVG library
// (generate_position_svg) rasterised through QSvgRenderer into a cached pixmap,
// and turns mouse clicks into engine moves. The board is an observer: it never
// mutates the game, it only emits moveRequested(); the MainWindow validates and
// applies, then calls setGame() again to refresh.
//
// Move UX (movement phase): if the side to move has captured enemy discs, the
// FIRST click selects an enemy Bound disc to remove (mandatory); then click an
// own Free disc (the origin) and finally a highlighted destination. In the
// placement phase a single click on an empty square places a disc.
class BoardWidget : public guicommon::BoardWidgetBase {
    Q_OBJECT
public:
    explicit BoardWidget(QWidget* parent = nullptr);

    void setGame(const Latrunculi::Game* game);  // resets selection + rebuilds
    void setSideColors(const QColor& a, const QColor& b);
    void setBackgroundColor(const QColor& c);
    void setSuggestion(AbsGame::MoveId mv);
    void clearSuggestion();
    void clearSelection();  // drop any in-progress two-click selection

    QSize sizeHint() const override { return QSize(850, 680); }
    QSize minimumSizeHint() const override { return QSize(360, 360); }

signals:
    void moveRequested(AbsGame::MoveId mv);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    int  cellAt(const QPointF& pos) const override;  // widget pixel -> square, or -1

private:
    void rebuild();                            // regenerate the SVG pixmap + geometry
    QPointF squareCenter(int square) const;    // square -> widget pixel centre
    std::vector<int> legalDestinations(int from) const;
    bool currentHasCaptives() const;           // side to move has enemy Bound discs
    void handlePlacementClick(int square);
    void handleMovementClick(int square);

    const Latrunculi::Game* game_ = nullptr;
    QColor colorA_{0xFA, 0xE5, 0xBE};  // pale beige  (side_a default)
    QColor colorB_{0x85, 0x25, 0x32};  // brick-red   (side_b default)

    // Cached render and the geometry it was built with.
    QPixmap cache_;
    double  scale_  = 0.0;  // pixels per square
    double  margin_ = 0.0;  // gutter in pixels (margin_units * scale_)
    double  offX_   = 0.0;  // pixmap top-left within the widget (centring)
    double  offY_   = 0.0;

    games::board::BoardSpec    look_;
    games::board::RenderConfig render_;
    games::board::SvgStyle     style_;

    // Two-click move state (movement phase).
    int pendingRemove_ = -1;        // chosen enemy Bound disc to remove (-1 = none)
    int selectedFrom_  = -1;        // chosen own Free disc
    std::vector<int> destinations_; // legal `to` squares for selectedFrom_

    // Suggestion overlay (decoded from a MoveId).
    AbsGame::MoveId suggestion_ = AbsGame::kPass;
    int suggestionFrom_ = -1;
    int suggestionTo_   = -1;
};
// Copyright Ben Paul Wise. All Rights Reserved.
