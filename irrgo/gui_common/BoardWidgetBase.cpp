// Copyright Ben Paul Wise. All Rights Reserved.
#include "BoardWidgetBase.h"

#include <QMouseEvent>

namespace guicommon {

BoardWidgetBase::BoardWidgetBase(QWidget* parent) : QWidget(parent) {
    setMouseTracking(true);
}

void BoardWidgetBase::setLastMove(int cell) {
    lastMoveCell_ = cell;
    update();
}

void BoardWidgetBase::setSearching(bool searching) {
    searching_ = searching;
    if (searching_) {
        hoverCell_ = -1;  // no hover feedback while the engine is thinking
    }
    update();
}

void BoardWidgetBase::clearHover() {
    if (hoverCell_ != -1) {
        hoverCell_ = -1;
        emit hoverChanged(-1);
        update();
    }
}

void BoardWidgetBase::resetFeedback() {
    hoverCell_    = -1;
    lastMoveCell_ = -1;
}

void BoardWidgetBase::mouseMoveEvent(QMouseEvent* e) {
    if (searching_) {
        return;  // no hover feedback while the engine is thinking
    }
    const int cell = cellAt(e->position());
    if (cell != hoverCell_) {
        hoverCell_ = cell;
        emit hoverChanged(cell);
        update();
    }
}

void BoardWidgetBase::leaveEvent(QEvent*) {
    clearHover();
}

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
