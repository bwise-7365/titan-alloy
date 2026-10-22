// Copyright Ben Paul Wise. All Rights Reserved.
#include "SearchController.h"

#include "Searcher.h"

#include <QProgressBar>
#include <QTimer>
#include <thread>

namespace guicommon {

namespace {

// The wall clock the search will actually run out, in milliseconds. Both algorithms have
// one: MCTS spends its whole budget, and NegaMax deepens until the clock stops it (its
// depth is only a ceiling, see Params::kNegamaxDepthCeiling). So the bar always has a
// real fraction to draw.
int searchBudgetMsOf(const SearchController::Params& p) {
    if (p.algo == SearchController::Algorithm::Mcts) {
        return p.seconds * 1000;
    }
    return p.negamaxTimeMs;
}

}  // namespace

SearchController::Params SearchController::Params::negamaxTimed(int seconds) {
    Params p;
    p.algo          = Algorithm::NegaMax;
    p.depth         = kNegamaxDepthCeiling;
    p.negamaxTimeMs = seconds * 1000;
    return p;
}

SearchController::Params SearchController::Params::mctsTimed(int seconds) {
    Params p;
    p.algo    = Algorithm::Mcts;
    p.seconds = seconds;
    return p;
}

SearchController::SearchController(QObject* parent) : QObject(parent) {
    searchBarTimer_ = new QTimer(this);
    connect(searchBarTimer_, &QTimer::timeout, this, [this]() {
        if (searchBudgetMs_ > 0) {
            int pct = qMin(100, static_cast<int>(searchElapsed_.elapsed()) * 100
                                / searchBudgetMs_);
            searchProgress_->setValue(pct);
        } else {
            searchProgress_->setValue((searchProgress_->value() + 3) % 101);
        }
    });
}

void SearchController::setProgressBars(QProgressBar* search, QProgressBar* turn) {
    searchProgress_ = search;
    turnProgress_   = turn;
}

void SearchController::startSearchIndicator(int budgetMs) {
    searchBudgetMs_ = budgetMs;
    searchProgress_->setValue(0);
    searchProgress_->show();
    if (searchBudgetMs_ > 0) {
        searchElapsed_.start();
        // Aim for about a hundred steps across the whole budget, so a short search still
        // animates instead of jumping in quarters, and a long one does not wake the UI
        // thread more often than the bar has pixels to show for it.
        searchBarTimer_->setInterval(qBound(30, searchBudgetMs_ / 100, 250));
    } else {
        searchBarTimer_->setInterval(30);
    }
    searchBarTimer_->start();
}

void SearchController::stopSearchIndicator() {
    searchBarTimer_->stop();
    searchProgress_->hide();
}

void SearchController::cancelSearch() {
    ++searchGen_;
    playTurnsRemaining_ = 0;
    playTurnsTotal_     = 0;
    if (turnProgress_) {
        turnProgress_->hide();
    }
    if (isSearching_) {
        isSearching_ = false;
        stopSearchIndicator();
        emit searchingChanged(false);
    }
}

void SearchController::launch(std::unique_ptr<AbsGame::Game> clone, Params params,
                              std::function<void(AbsGame::MoveId, unsigned)> onComplete) {
    isSearching_ = true;
    emit searchingChanged(true);
    startSearchIndicator(searchBudgetMsOf(params));

    unsigned gen = searchGen_;
    // shared_ptr so the (copyable) thread lambda can own the cloned game.
    std::shared_ptr<AbsGame::Game> game(std::move(clone));
    std::thread([this, gen, params, game, onComplete]() {
        AbsGame::MoveId mv = (params.algo == Algorithm::Mcts)
            ? AbsGame::Searcher::mcts(*game, params.seconds)
            : AbsGame::Searcher::bestMove(*game, params.depth, params.negamaxTimeMs);
        QMetaObject::invokeMethod(this, [this, mv, gen, onComplete]() {
            if (gen != searchGen_) {
                return;
            }
            isSearching_ = false;
            stopSearchIndicator();
            emit searchingChanged(false);
            onComplete(mv, gen);
        }, Qt::QueuedConnection);
    }).detach();
}

void SearchController::beginTurnRun(int turns) {
    if (playTurnsRemaining_ <= 0) {
        playTurnsRemaining_ = turns;
        playTurnsTotal_     = turns;
        if (turnProgress_) {
            turnProgress_->setValue(0);
            turnProgress_->show();
        }
    }
}

void SearchController::advanceTurn() {
    --playTurnsRemaining_;
    if (playTurnsTotal_ > 0 && turnProgress_) {
        turnProgress_->setValue((playTurnsTotal_ - playTurnsRemaining_) * 100
                                / playTurnsTotal_);
    }
}

void SearchController::endTurnRun() {
    if (turnProgress_) {
        turnProgress_->hide();
    }
    playTurnsTotal_ = 0;
}

}  // namespace guicommon
// Copyright Ben Paul Wise. All Rights Reserved.
