// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "AbsGame.h"

#include <QElapsedTimer>
#include <QObject>
#include <functional>
#include <memory>

class QProgressBar;
class QTimer;

namespace guicommon {

// Owns the threaded-search machinery shared by the game GUIs: the two status
// bars' animation, the in-flight-search "generation" guard, and the auto-play
// turn counter. A search runs on a detached std::thread against a cloned game;
// its result is marshalled back to the UI thread and discarded if a newer search
// (or a board reset) has bumped the generation since it launched.
//
// This is a verbatim extraction of the logic previously copy-pasted into each
// game's MainWindow, so the GUIs behave identically after migrating onto it.
class SearchController : public QObject {
    Q_OBJECT
public:
    enum class Algorithm { NegaMax, Mcts };

    struct Params {
        Algorithm algo = Algorithm::NegaMax;
        int depth = 1;              // NegaMax search depth
        int seconds = 0;            // MCTS time budget
        int negamaxTimeMs = 10000;  // NegaMax wall-clock cap (matches the old code)
        // Which of the two NegaMax budgets actually bounds the search, and therefore
        // what the progress bar can honestly show. A depth-budgeted search (irrgo,
        // mancala) finishes when it finishes: negamaxTimeMs is only a backstop, and
        // drawing a fraction of it would be a progress bar that lies. A time-budgeted
        // one (latrunculi, whose depth is just an iterative-deepening ceiling) runs the
        // clock out every time, so elapsed/budget is exactly its progress.
        //
        // The caller has to say which it is: `depth` and `negamaxTimeMs` are both always
        // set to something, so there is nothing here to infer it from.
        bool negamaxTimeBudgeted = false;
    };

    explicit SearchController(QObject* parent = nullptr);

    // The bars the controller animates and shows/hides. Ownership stays with the
    // caller (they live in the window's board-area layout). `turn` may be null.
    void setProgressBars(QProgressBar* search, QProgressBar* turn);

    // budgetMs == 0 -> indeterminate sweep animation, for a search whose duration is not
    //                  known in advance (a depth-budgeted NegaMax).
    // budgetMs  > 0 -> the true elapsed fraction, refreshed every 250 ms, for a search
    //                  that runs a wall clock out (MCTS, and time-budgeted NegaMax).
    // Milliseconds rather than seconds because NegaMax's budget is in milliseconds;
    // taking seconds here would have truncated any budget under a second to a sweep.
    void startSearchIndicator(int budgetMs = 0);
    void stopSearchIndicator();

    // Invalidate any in-flight search (bumps the generation), clear the auto-play
    // run, and hide the bars. Safe to call when not searching.
    void cancelSearch();

    bool isSearching() const { return isSearching_; }

    // Launch an async search on `clone` (ownership taken). `onComplete` runs on
    // the UI thread only if the generation is still current; isSearching_ is
    // cleared, the indicator stopped, and searchingChanged(false) emitted before
    // it is invoked.
    void launch(std::unique_ptr<AbsGame::Game> clone, Params params,
                std::function<void(AbsGame::MoveId, unsigned gen)> onComplete);

    // Auto-play turn bookkeeping (drives the turn progress bar).
    void beginTurnRun(int turns);  // no-op while a run is already active
    void advanceTurn();
    bool turnRunActive() const { return playTurnsRemaining_ > 0; }
    void endTurnRun();

signals:
    // Emitted when isSearching_ flips; windows connect it to updateControls().
    void searchingChanged(bool searching);

private:
    QProgressBar* searchProgress_ = nullptr;  // not owned
    QProgressBar* turnProgress_   = nullptr;  // not owned
    QTimer*       searchBarTimer_ = nullptr;  // owned (parented to this)
    QElapsedTimer searchElapsed_;             // wall-clock timer for MCTS progress
    int           searchBudgetMs_ = 0;        // 0 = sweep mode; >0 = timed mode
    bool          isSearching_    = false;
    unsigned      searchGen_      = 0;         // bumped to discard stale results
    int           playTurnsRemaining_ = 0;     // counts down during auto-play
    int           playTurnsTotal_     = 0;     // total turns for the current run
};

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
