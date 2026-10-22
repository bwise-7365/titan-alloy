// Copyright Ben Paul Wise. All Rights Reserved.

#include "GameStats.h"

#include <algorithm>
#include <cstddef>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>

namespace Latrunculi {

namespace {

const char* reasonName(WinReason r) {
    switch (r) {
        case WinReason::Reduction:      return "Reduction";
        case WinReason::Immobilization: return "Immobilize";
        case WinReason::QuietGame:      return "QuietGame";
        case WinReason::None:           return "None";
    }
    throw std::invalid_argument("Latrunculi stats: unknown WinReason");
}

// Mean of a projection over a batch, as a double. An empty batch has no mean, and
// returning 0.0 for one would be a fabricated statistic, so the caller guards instead.
template <typename Fn>
double mean(const std::vector<GameStats>& all, Fn value) {
    double total = 0.0;
    for (const GameStats& s : all) {
        total += static_cast<double>(value(s));
    }
    return total / static_cast<double>(all.size());
}

template <typename Fn>
int countIf(const std::vector<GameStats>& all, Fn pred) {
    int n = 0;
    for (const GameStats& s : all) {
        if (pred(s)) {
            ++n;
        }
    }
    return n;
}

}  // namespace

GameStats analyseGame(const Game& finished) {
    if (!finished.isOver()) {
        throw std::invalid_argument("Latrunculi stats: the game is not over");
    }

    GameStats s;
    s.winner = finished.winner();
    s.reason = finished.winReason();
    s.freeP0 = finished.freeDiscs(0);
    s.freeP1 = finished.freeDiscs(1);
    s.boundP0 = finished.boundDiscs(0);
    s.boundP1 = finished.boundDiscs(1);
    s.finalMargin = s.freeP0 - s.freeP1;
    s.plies = static_cast<int>(finished.history().size());

    // Replay from an empty board under the same rules. Free-disc counts before and after
    // each ply give captures; the recorded Move gives removals. Replaying is what keeps
    // this a pure function of the finished game rather than bookkeeping smeared through
    // the play loop.
    Game replay(finished.rows(), finished.columns(), finished.perSide(),
                finished.moveStyle());
    int previousMargin = 0;
    int quietRun = 0;
    bool sawFirstMargin = false;

    for (const Move& m : finished.history()) {
        const bool placement = (m.from < 0);
        const AbsGame::MoveId mid = placement
            ? replay.placementMove(m.to)
            : replay.movementMove(m.from, m.to, m.removed);

        const int freeOppBefore = replay.freeDiscs(1 - replay.currentPlayer());
        if (!replay.applyMove(mid)) {
            throw std::invalid_argument(
                "Latrunculi stats: recorded move " + std::to_string(m.turn) +
                " does not replay legally");
        }
        // currentPlayer has flipped, so the side that just moved is the new opponent.
        const int freeOppAfter = replay.freeDiscs(replay.currentPlayer());

        if (placement) {
            ++s.placementPlies;
            continue;
        }
        ++s.movementPlies;

        const bool captured = freeOppAfter < freeOppBefore;
        const bool removed = m.removed >= 0;
        if (captured) {
            ++s.captures;
        }
        if (removed) {
            ++s.removals;
        }
        if (captured || removed) {
            s.lastActivePly = m.turn;
            quietRun = 0;
        } else {
            ++quietRun;
            s.longestQuietRun = std::max(s.longestQuietRun, quietRun);
        }

        // Lead tracking over the movement phase. A margin of 0 is neither side's lead,
        // so it is not itself a change; only a genuine sign flip counts.
        const int margin = replay.freeDiscs(0) - replay.freeDiscs(1);
        s.peakMargin = std::max(s.peakMargin, std::abs(margin));
        if (margin != 0) {
            if (sawFirstMargin && ((margin > 0) != (previousMargin > 0))) {
                ++s.leadChanges;
            }
            previousMargin = margin;
            sawFirstMargin = true;
        }
    }
    return s;
}

std::string statsHeader() {
    std::ostringstream o;
    o << std::left << std::setw(5) << "game"
      << std::right
      << std::setw(7) << "plies"
      << std::setw(7) << "moveP"
      << std::setw(6) << "capt"
      << std::setw(6) << "remv"
      << std::setw(8) << "lastAct"
      << std::setw(8) << "tail"
      << std::setw(7) << "quiet"
      << std::setw(7) << "lead"
      << std::setw(7) << "peak"
      << std::setw(8) << "margin"
      << std::setw(4) << "win"
      << "  reason";
    return o.str();
}

std::string formatStatsRow(const GameStats& s, int gameIndex) {
    // "tail" is the number of plies after the last capture or removal: the dead run at
    // the end of the game, which is the symptom this instrumentation exists to measure.
    const int tail = s.plies - s.lastActivePly;
    std::ostringstream o;
    o << std::left << std::setw(5) << gameIndex
      << std::right
      << std::setw(7) << s.plies
      << std::setw(7) << s.movementPlies
      << std::setw(6) << s.captures
      << std::setw(6) << s.removals
      << std::setw(8) << s.lastActivePly
      << std::setw(8) << tail
      << std::setw(7) << s.longestQuietRun
      << std::setw(7) << s.leadChanges
      << std::setw(7) << s.peakMargin
      << std::setw(8) << s.finalMargin
      << std::setw(4) << (s.winner == 0 ? "A" : "B")
      << "  " << reasonName(s.reason);
    return o.str();
}

std::string formatStatsSummary(const std::vector<GameStats>& all) {
    if (all.empty()) {
        throw std::invalid_argument("Latrunculi stats: no games to summarise");
    }
    const double n = static_cast<double>(all.size());
    const int quiet = countIf(all, [](const GameStats& s) {
        return s.reason == WinReason::QuietGame;
    });
    const int reduction = countIf(all, [](const GameStats& s) {
        return s.reason == WinReason::Reduction;
    });
    const int immobilize = countIf(all, [](const GameStats& s) {
        return s.reason == WinReason::Immobilization;
    });
    const int wonByA = countIf(all, [](const GameStats& s) { return s.winner == 0; });
    const int anyLeadChange = countIf(all, [](const GameStats& s) {
        return s.leadChanges > 0;
    });

    std::ostringstream o;
    o << std::fixed << std::setprecision(1);
    o << "games                " << all.size() << '\n';
    o << "mean plies           " << mean(all, [](const GameStats& s) { return s.plies; })
      << "   (movement " << mean(all, [](const GameStats& s) { return s.movementPlies; })
      << ")\n";
    o << "mean captures        " << mean(all, [](const GameStats& s) { return s.captures; })
      << '\n';
    o << "mean removals        " << mean(all, [](const GameStats& s) { return s.removals; })
      << '\n';
    o << "mean dead tail       "
      << mean(all, [](const GameStats& s) { return s.plies - s.lastActivePly; })
      << "   plies after the last capture or removal\n";
    o << "mean longest quiet   "
      << mean(all, [](const GameStats& s) { return s.longestQuietRun; }) << '\n';
    o << "mean lead changes    "
      << mean(all, [](const GameStats& s) { return s.leadChanges; })
      << "   (" << anyLeadChange << " of " << all.size() << " games had any)\n";
    o << "mean peak margin     "
      << mean(all, [](const GameStats& s) { return s.peakMargin; }) << '\n';
    o << "mean final margin    "
      << mean(all, [](const GameStats& s) { return std::abs(s.finalMargin); })
      << "   (absolute)\n";
    o << std::setprecision(0);
    o << "ended by quiet game  " << quiet << "  ("
      << (100.0 * quiet / n) << "%)\n";
    o << "ended by reduction   " << reduction << "  ("
      << (100.0 * reduction / n) << "%)\n";
    o << "ended by immobilize  " << immobilize << "  ("
      << (100.0 * immobilize / n) << "%)\n";
    o << "won by A / B         " << wonByA << " / " << (all.size() - wonByA) << '\n';
    return o.str();
}

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.
