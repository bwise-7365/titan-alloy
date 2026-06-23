// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <QWidget>

class QEvent;
class QMouseEvent;

namespace guicommon {

// Base for the game board widgets (irrgo, mancala, latrunculi). It factors out the
// manual-play feedback state and mouse-tracking that were otherwise duplicated in
// each widget: the hovered cell, the last-move cell, and a "searching" flag. It is
// game-agnostic -- it deals only in integer cell ids -- so each widget supplies the
// pixel->cell map via cellAt() and keeps its own painting and click handling (the
// move grammars and rendering differ too much to share).
class BoardWidgetBase : public QWidget {
    Q_OBJECT
public:
    explicit BoardWidgetBase(QWidget* parent = nullptr);

    void setLastMove(int cell);   // mark the most recently played cell (-1 = none)
    void setSearching(bool searching);  // suppress hover / dim while the engine thinks
    void clearHover();

signals:
    void hoverChanged(int cell);  // -1 when no cell is hovered

protected:
    // Pixel position -> cell id, or -1 if none. Implemented per game (the former
    // nodeAt / pitAt / squareAt).
    virtual int cellAt(const QPointF& pos) const = 0;

    // Clear the hover and last-move markers; call this from a derived setGame().
    void resetFeedback();

    int  hoverCell() const { return hoverCell_; }
    int  lastMoveCell() const { return lastMoveCell_; }
    bool isSearching() const { return searching_; }

    void mouseMoveEvent(QMouseEvent* e) override;
    void leaveEvent(QEvent* e) override;

private:
    int  hoverCell_    = -1;
    int  lastMoveCell_ = -1;
    bool searching_    = false;
};

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
