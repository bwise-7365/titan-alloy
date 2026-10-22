// Copyright Ben Paul Wise. All Rights Reserved.

#include "BenchBoundary.h"

#include "Eval.h"
#include "PlacementEval.h"

#include <cmath>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>
#include <string>

namespace Latrunculi::Bench {

namespace {

// The board as the evaluation functions want it: one row-major vector of cells.
std::vector<Cell> cellsOf(const Game& game) {
    std::vector<Cell> cells;
    cells.reserve(static_cast<std::size_t>(game.squareCount()));
    for (int square = 0; square < game.squareCount(); ++square) {
        cells.push_back(game.cellAt(square));
    }
    return cells;
}

// Fill the at-the-flip fields of `b` from `game`, which must be standing at the flip.
void measureFlip(const Game& game, BoundaryStats& b) {
    const std::vector<Cell> cells = cellsOf(game);
    for (int player = 0; player < 2; ++player) {
        const PlacementTerms placement = placementTerms(
            cells, game.rows(), game.columns(), player, game.moveStyle());
        const PositionalTerms positional = positionalTerms(
            cells, game.rows(), game.columns(), player, game.moveStyle());
        b.exposed[player] = placement.oneMoveCapturable;
        b.notchExposure[player] = placement.notchExposure;
        b.vulnerableAxes[player] = placement.vulnerableAxes;
        b.threats[player] = positional.threats;
    }
    b.reachedMovement = true;
}

}  // anonymous namespace

BoundaryStats analyseBoundary(const Game& game) {
    BoundaryStats b;
    Game replay(game.rows(), game.columns(), game.perSide(), game.moveStyle());

    for (const Move& m : game.history()) {
        // The flip is detected on the replay's own phase, before the ply is applied:
        // the first movement ply in the history finds the replay already in Movement.
        if (!b.reachedMovement && replay.phase() == Phase::Movement) {
            measureFlip(replay, b);
        }
        const bool placement = (m.from < 0);
        const AbsGame::MoveId mid = placement
            ? replay.placementMove(m.to)
            : replay.movementMove(m.from, m.to, m.removed);

        const int mover = replay.currentPlayer();
        const int freeOppBefore = replay.freeDiscs(1 - mover);
        if (!replay.applyMove(mid)) {
            throw std::invalid_argument(
                "Latrunculi boundary: recorded move " + std::to_string(m.turn) +
                " does not replay legally");
        }
        if (placement) {
            continue;
        }
        if (b.earlyPlies >= kEarlyMovementPlies) {
            break;  // the window is full; the rest of the game is not this report's
        }
        ++b.earlyPlies;
        // A capture binds one or more enemy Free discs; counted as an event per ply,
        // exactly as GameStats does.
        if (replay.freeDiscs(1 - mover) < freeOppBefore) {
            ++b.captures[mover];
        }
    }
    // A history that ends exactly at the flip (capped at 2*perSide plies) has a
    // measurable boundary but no early plies.
    if (!b.reachedMovement && replay.phase() == Phase::Movement) {
        measureFlip(replay, b);
    }
    return b;
}

std::string boundaryColumns(const BoundaryStats& b) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    if (!b.reachedMovement) {
        for (int k = 0; k < 6; ++k) {
            out << std::setw(6) << "-";
        }
        return out.str();
    }
    out << std::setw(6) << b.exposed[0] << std::setw(6) << b.exposed[1]
        << std::setw(6) << b.threats[0] << std::setw(6) << b.threats[1]
        << std::setw(6) << b.captures[0] << std::setw(6) << b.captures[1];
    return out.str();
}

std::string boundaryHeader() {
    std::ostringstream out;
    out << std::setw(6) << "exp0" << std::setw(6) << "exp1"
        << std::setw(6) << "thr0" << std::setw(6) << "thr1"
        << std::setw(6) << "cap0" << std::setw(6) << "cap1";
    return out.str();
}

namespace {

// Mean and standard error of one per-side field over the measured games.
struct MeanSe {
    double mean = 0.0;
    double se = 0.0;
};

template <typename Pick>
MeanSe meanSe(const std::vector<BoundaryStats>& all, Pick pick) {
    double sum = 0.0;
    double sumSq = 0.0;
    int n = 0;
    for (const BoundaryStats& b : all) {
        if (!b.reachedMovement) {
            continue;
        }
        const double v = static_cast<double>(pick(b));
        sum += v;
        sumSq += v * v;
        ++n;
    }
    MeanSe r;
    if (n == 0) {
        return r;
    }
    r.mean = sum / n;
    if (n > 1) {
        const double variance = (sumSq - n * r.mean * r.mean) / (n - 1);
        r.se = std::sqrt(variance > 0.0 ? variance : 0.0) / std::sqrt(static_cast<double>(n));
    }
    return r;
}

std::string pair(const char* label, const MeanSe& p0, const MeanSe& p1) {
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::left << std::setw(21) << label << std::right << std::fixed
        << std::setprecision(2)
        << "P0 " << p0.mean << " +/- " << p0.se
        << "   P1 " << p1.mean << " +/- " << p1.se << '\n';
    return out.str();
}

}  // anonymous namespace

std::string formatBoundarySummary(const std::vector<BoundaryStats>& all) {
    int measured = 0;
    int fullWindows = 0;
    for (const BoundaryStats& b : all) {
        if (b.reachedMovement) {
            ++measured;
            if (b.earlyPlies == kEarlyMovementPlies) {
                ++fullWindows;
            }
        }
    }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "-- phase boundary (" << measured << " of " << all.size()
        << " games reached movement; " << fullWindows << " had the full "
        << kEarlyMovementPlies << "-ply early window; mean +/- standard error) --\n";
    if (measured == 0) {
        return out.str();
    }
    const auto side = [&all](int p, auto field) {
        return meanSe(all, [p, field](const BoundaryStats& b) { return (b.*field)[p]; });
    };
    out << pair("exposed at flip", side(0, &BoundaryStats::exposed),
                side(1, &BoundaryStats::exposed))
        << pair("threats at flip", side(0, &BoundaryStats::threats),
                side(1, &BoundaryStats::threats))
        << pair("notch exposure", side(0, &BoundaryStats::notchExposure),
                side(1, &BoundaryStats::notchExposure))
        << pair("vulnerable axes", side(0, &BoundaryStats::vulnerableAxes),
                side(1, &BoundaryStats::vulnerableAxes))
        << pair("early captures by", side(0, &BoundaryStats::captures),
                side(1, &BoundaryStats::captures));
    return out.str();
}

}  // namespace Latrunculi::Bench
// Copyright Ben Paul Wise. All Rights Reserved.
