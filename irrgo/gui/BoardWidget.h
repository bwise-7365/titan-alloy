// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "Game.h"
#include <QColor>
#include <QWidget>

class QTimer;

class BoardWidget : public QWidget {
    Q_OBJECT
public:
    explicit BoardWidget(QWidget* parent = nullptr);

    void setGame(const Game* game);
    void setBgColor(QColor c);

signals:
    void moveRequested(int nodeId);

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    void updateTransform();
    int  nodeAt(QPointF pos) const;

    const Game* game_      = nullptr;
    QColor      bgColor_   { "#DBAD6B" };
    QColor      lineColor_ { "#373C65" };

    int     hoverNode_     = -1;
    int     tentativeNode_ = -1;
    QTimer* confirmTimer_  = nullptr;

    // Cached transform — updated on resize / graph change
    float minX_=0, minY_=0, rangeX_=1, rangeY_=1;
    float scale_=1, offX_=0, offY_=0, stoneR_=10;
};
