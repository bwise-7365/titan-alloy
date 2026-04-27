// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "DVR.h"
#include "Game.h"
#include <QColor>
#include <QPixmap>
#include <QString>
#include <QWidget>
#include <vector>

class QTimer;

class BoardWidget : public QWidget {
    Q_OBJECT
public:
    explicit BoardWidget(QWidget* parent = nullptr);

    void setGame(const IrrGo::Game* game);
    void setBgColor(QColor c);

    // Highlight a suggested move with a coloured border; -1 clears it.
    void setSuggestion(int nodeId, bool isBlack);
    void clearSuggestion();
    void setBoardInfo(const QString& info);   // right-corner label ("rows x cols: seed")

    void setSearching(bool s);
    void setShowBlackDvr(bool show);
    void setShowWhiteDvr(bool show);
    void setDvrRadius(int r);
    void showNeighborhoodSize();
    void hideNeighborhoodSize();
    void showLabels();
    void hideLabels();

    // Mark the most recently placed stone with a contrasting dot; -1 clears it.
    void setLastMove(int nodeId);
    void setUseTexture(bool on);

signals:
    void moveRequested(int nodeId);
    void clearSuggestionRequested();  // any left-click anywhere in the board area

protected:
    void paintEvent(QPaintEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void leaveEvent(QEvent*) override;
    void resizeEvent(QResizeEvent*) override;

private:
    void updateTransform();
    int  nodeAt(QPointF pos) const;

    // Shared helper: draws a stone-sized disc with a coloured border.
    // nodeId is used to select the texture variant; -1 → variant 0.
    void paintStoneBordered(QPainter& p, QPointF pt,
                            bool isBlack, QColor borderColor, int nodeId = -1) const;

    void loadTextures();
    void rescaleTextures();

    const IrrGo::Game* game_      = nullptr;
    QColor      bgColor_   { "#DBAD6B" };
    QColor      lineColor_ { "#373C65" };
    QString     boardInfoRight_;            // right corner label, set via setBoardInfo()

    bool    searching_       = false;
    int     hoverNode_       = -1;
    int     tentativeNode_   = -1;
    int     suggestedNode_   = -1;
    bool    suggestIsBlack_  = true;
    int     lastMoveNode_    = -1;
    bool    showBlackDvr_         = false;
    bool    showWhiteDvr_         = false;
    int     dvrRadius_            = 4;
    bool    showNeighborhoodSize_ = false;
    bool    showLabels_           = false;
    QTimer* confirmTimer_    = nullptr;

    // Texture stone images: source (full-res) and scaled to current stoneR_
    bool useTexture_ = false;
    std::vector<QPixmap> blackSrc_,    whiteSrc_;
    std::vector<QPixmap> blackScaled_, whiteScaled_;
    QPixmap fabricSrc_;
    QPixmap fabricScaled_;

    // Cached transform — updated on resize / graph change
    float minX_=0, minY_=0, rangeX_=1, rangeY_=1;
    float scale_=1, offX_=0, offY_=0, stoneR_=10;
};
// Copyright Ben Paul Wise. All Rights Reserved.
