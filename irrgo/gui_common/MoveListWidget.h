// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <QListWidget>
#include <QStringList>

namespace guicommon {

// A clickable move log: one row per ply, monospaced. Clicking ply r jumps the
// replay to the position after that move (emits plyClicked). setCurrentPly()
// highlights the row for the current cursor without emitting. The owner fills it
// with game-specific move text via setMoves().
class MoveListWidget : public QListWidget {
    Q_OBJECT
public:
    explicit MoveListWidget(QWidget* parent = nullptr);

    void setMoves(const QStringList& moves);  // one row per ply, in order
    void setCurrentPly(int ply);              // highlight the row for ply k (0 = none)

signals:
    void plyClicked(int ply);  // row r clicked -> the position after ply r+1
};

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
