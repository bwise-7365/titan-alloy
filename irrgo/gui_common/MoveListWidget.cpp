// Copyright Ben Paul Wise. All Rights Reserved.
#include "MoveListWidget.h"

#include <QFont>
#include <QStringList>

namespace guicommon {

MoveListWidget::MoveListWidget(QWidget* parent) : QListWidget(parent) {
    QFont f("Monospace");
    f.setStyleHint(QFont::TypeWriter);
    setFont(f);
    setSelectionMode(QAbstractItemView::SingleSelection);
    setUniformItemSizes(true);
    // Only a real user click navigates; programmatic setCurrentRow() (in
    // setCurrentPly) does not emit itemClicked, so there is no feedback loop.
    connect(this, &QListWidget::itemClicked, this, [this](QListWidgetItem* it) {
        emit plyClicked(row(it) + 1);
    });
}

void MoveListWidget::setMoves(const QStringList& moves) {
    clear();
    addItems(moves);
}

void MoveListWidget::setCurrentPly(int ply) {
    if (ply <= 0 || ply > count()) {
        setCurrentRow(-1);  // clears the highlight
        return;
    }
    setCurrentRow(ply - 1);              // ply k highlights the k-th move (row k-1)
    if (auto* it = item(ply - 1)) {
        scrollToItem(it);
    }
}

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
