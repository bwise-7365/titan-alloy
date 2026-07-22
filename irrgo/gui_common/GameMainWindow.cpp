// Copyright Ben Paul Wise. All Rights Reserved.
#include "GameMainWindow.h"

#include "MoveListWidget.h"
#include "PlaybackBar.h"

#include <algorithm>

namespace guicommon {

GameMainWindow::GameMainWindow(QWidget* parent) : QMainWindow(parent) {
    search_ = new SearchController(this);
    connect(search_, &SearchController::searchingChanged, this, [this](bool) {
        updateControls();
    });
}

void GameMainWindow::startPlay(SearchController::Params params, int turns) {
    if (!currentGame() || currentGame()->isTerminal()
        || search().isSearching() || extraSearchBlock()) {
        return;
    }
    search().beginTurnRun(turns);

    // A derived window may supply this ply itself rather than have it searched (see
    // autoPlayMoveOverride). Latrunculi's placement policy plays a random placement now
    // and then, and at most two of those can fall back to back, so recursing straight
    // into the next ply here cannot run away.
    AbsGame::MoveId preset = AbsGame::kPass;
    if (autoPlayMoveOverride(preset)) {
        applyComputedMove(preset);
        continuePlay(params, turns);
        return;
    }

    search().launch(currentGame()->clone(), params,
                    [this, params, turns](AbsGame::MoveId mv, unsigned) {
        applyComputedMove(mv);
        continuePlay(params, turns);
    });
}

void GameMainWindow::continuePlay(SearchController::Params params, int turns) {
    search().advanceTurn();
    if (currentGame() && !currentGame()->isTerminal() && search().turnRunActive()) {
        startPlay(params, turns);
    } else {
        search().endTurnRun();
    }
}

// ── Playback / review ─────────────────────────────────────────────────────────

void GameMainWindow::registerPlayback(PlaybackBar* bar, MoveListWidget* list) {
    playbackBar_  = bar;
    moveListView_ = list;
    connect(bar,  &PlaybackBar::seek,          this, &GameMainWindow::gotoPly);
    connect(list, &MoveListWidget::plyClicked, this, &GameMainWindow::gotoPly);
}

void GameMainWindow::gotoPly(int ply) {
    search().cancelSearch();    // navigating away abandons any in-flight search
    const int k = std::clamp(ply, 0, playbackPlyCount());
    rebuildToPly(k);            // derived reconstructs the game at ply k + refreshes
    reviewCursor_ = k;
    if (playbackBar_)  { playbackBar_->setCursor(k); }
    if (moveListView_) { moveListView_->setCurrentPly(k); }
}

void GameMainWindow::syncPlaybackToEnd() {
    const int m = playbackPlyCount();
    reviewCursor_ = m;
    if (playbackBar_)  { playbackBar_->setPlyCount(m); playbackBar_->setCursor(m); }
    if (moveListView_) { moveListView_->setCurrentPly(m); }
}

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
