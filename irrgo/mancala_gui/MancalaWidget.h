// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "BoardWidgetBase.h"
#include "Game.h"
#include <QColor>
#include <QRectF>
#include <vector>

// Renders the Mancala (Kalah) board and emits moveRequested when the user
// clicks a legal pit.  The widget is read-only with respect to game state:
// it observes a const Game* and the owner calls update() after mutations.
class MancalaWidget : public guicommon::BoardWidgetBase {
    Q_OBJECT
public:
    explicit MancalaWidget(QWidget* parent = nullptr);

    void setGame(const Mancala::Game* game);
    void setBgColor(QColor c);

    // Returns the exact pixel size needed to display N circular pits with fixed radius.
    QSize preferredSize() const;

    // Highlight a pit as the AI-suggested move; pitIndex -1 clears it.
    void setSuggestion(int pitIndex);
    void clearSuggestion();

signals:
    void moveRequested(int pitIndex);

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void resizeEvent(QResizeEvent*) override;
    int  cellAt(const QPointF& pos) const override;  // pixel -> pit index (or -1)

private:
    void  rebuildGeometry();

    // Returns the screen rect for a given pit index.
    QRectF pitRect(int index) const;

    // Draws a single pit oval with stone count.
    void drawPit(QPainter& p, int index) const;

    // Draws a store (taller oval) for the given player.
    void drawStore(QPainter& p, int player) const;

    const Mancala::Game* game_      = nullptr;
    QColor               bgColor_   { "#DBAD6B" };
    int                  suggested_ = -1;

    // Cached geometry — rebuilt on resize / game change.
    // Indexed by pit slot (0 .. totalSlots-1); stores included at their indices.
    std::vector<QRectF> pitRects_;
    QRectF              storeRect0_; // Player 0 store (right side)
    QRectF              storeRect1_; // Player 1 store (left side)
};
// Copyright Ben Paul Wise. All Rights Reserved.
