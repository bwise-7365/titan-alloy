// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"
#include "SearchController.h"

#include <QMainWindow>

namespace guicommon {

class PlaybackBar;
class MoveListWidget;

// Base for the game main windows (irrgo, mancala, latrunculi). It owns the
// SearchController and provides the generic Play orchestration (the auto-play
// turn loop) that was previously copy-pasted into each window. Each derived
// window still builds its own widgets and menus, then implements a few hooks.
//
// Suggestion searches are launched by the derived windows directly via
// search().launch(...), because the result formatting is entirely game-specific.
class GameMainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit GameMainWindow(QWidget* parent = nullptr);

protected:
    SearchController& search() { return *search_; }

    // ── Hooks each game implements ───────────────────────────────────────────
    virtual AbsGame::Game* currentGame() = 0;            // nullptr if no game
    virtual void applyComputedMove(AbsGame::MoveId mv) = 0;
    virtual void updateControls() = 0;
    // An extra reason a search must not start (e.g. an ongoing setup animation).
    virtual bool extraSearchBlock() const { return false; }

    // Play one NegaMax/MCTS move, then auto-play up to `turns` plies, re-entering
    // itself after each completed move (identical to the old per-window loop).
    void startPlay(SearchController::Params params, int turns);

    // ── Playback / review (opt-in) ───────────────────────────────────────────
    // A game that supports Load-replay creates a PlaybackBar + MoveListWidget,
    // registers them here, fills the list via the widget, and overrides the two
    // hooks below. Games that don't (e.g. mancala) leave all of this untouched.
    void registerPlayback(PlaybackBar* bar, MoveListWidget* list);
    void syncPlaybackToEnd();          // after a move/load: cursor = M, refresh UI
    int  playbackCursor() const { return reviewCursor_; }

    virtual int  playbackPlyCount() const { return 0; }  // length of the timeline (M)
    virtual void rebuildToPly(int ply) { (void)ply; }    // reconstruct the game at ply

    void gotoPly(int ply);             // reconstruct + sync the bar/list to `ply`

private:
    SearchController* search_ = nullptr;
    PlaybackBar*      playbackBar_  = nullptr;
    MoveListWidget*   moveListView_ = nullptr;
    int               reviewCursor_ = 0;
};

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
