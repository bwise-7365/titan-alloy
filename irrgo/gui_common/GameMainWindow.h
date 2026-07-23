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

    // Lets a derived window supply one auto-play ply itself instead of searching for it:
    // return true and set `mv`, and startPlay() plays that move and carries on with the
    // turn run exactly as if a search had produced it. Latrunculi uses this for the
    // random placements in its opening policy. Default: never pre-empt the search.
    virtual bool autoPlayMoveOverride(AbsGame::MoveId&) { return false; }

    // Play one NegaMax/MCTS move, then auto-play up to `turns` plies, re-entering
    // itself after each completed move (identical to the old per-window loop).
    void startPlay(SearchController::Params params, int turns);
    // Shared tail of a completed auto-play ply, whether the move came from a search or
    // from autoPlayMoveOverride(): count the turn and either continue or end the run.
    void continuePlay(SearchController::Params params, int turns);

    // ── Human vs computer ────────────────────────────────────────────────────
    // Enter "play against the computer" mode: the human plays side `humanSide` (0 or 1)
    // and the computer answers with a search using `params` whenever it is to move. Call
    // from a derived window's Computer "Go!" slot; it cancels any in-flight search/auto-
    // play run first, then lets the computer open if it is the side to move.
    void beginVersus(SearchController::Params params, int humanSide);
    // Leave human-vs-computer mode (on Manual, an auto-play run, or a New Game). Safe to
    // call when not in it; does not touch the board or any in-flight search.
    void endVersus() { versusActive_ = false; versusHumanSide_ = -1; }
    bool versusActive() const { return versusActive_; }
    // After the derived window applies ANY move (a human board move or a computer move),
    // call this: in versus mode, if it is now the computer's turn, it searches and plays,
    // chaining through any further computer plies (e.g. Mancala's extra turns). No-op
    // unless versus mode is on and it is the computer to move.
    void maybeComputerMove();

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

    // Human-vs-computer state. versusHumanSide_ is the player index the human controls;
    // -1 (and versusActive_ false) means the mode is off.
    SearchController::Params versusParams_;
    int  versusHumanSide_ = -1;
    bool versusActive_    = false;
};

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
