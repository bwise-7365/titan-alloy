// Copyright Ben Paul Wise. All Rights Reserved.
//
// Instrumented batch self-play for Latrunculi. Plays N games under one rule set and one
// time budget and prints a per-game metric table plus an aggregate summary, so rule and
// evaluation changes can be compared numerically instead of by watching games. Metric
// definitions live in GameStats.h.
//
// Deliberately free of Qt and of the per-ply PNG rendering that selfplay.cpp does: this
// is meant to be run in bulk, and rendering would dominate the wall clock.
//
// Usage: named key=value arguments in any order. (Positional arguments were replaced once
// there were more knobs than anyone could remember the order of.)
//
//   latrunculi_bench games=50 ms=1000 seed=777001 style=slide payoff=convex komi=1.5
//
//     games      number of games to play                     (default 10)
//     ms         search budget per searched ply, in ms        (default 200)
//     seed       base RNG seed; 0 means clock-derived         (default 0)
//     style      slide | stepleap                             (default slide)
//     payoff     convex | gradient                            (default convex)
//     komi       half-integer credited to player 1            (default kDefaultKomi)
//     placement  policy | random                              (default policy)
//                policy = the shared PlacementPolicy (one random placement, then runs of
//                searched ones). random = every placement random, which removes any
//                advantage either side gains from SEARCHING the opening and so isolates
//                the first-move advantage proper.
//     rows, columns, perside                                  (default 8, 10, 20)
//
// Game g uses seed base+g, so a run is reproducible from the printed base seed.

#include "Game.h"
#include "GameStats.h"
#include "PlacementPolicy.h"
#include "Searcher.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>
#include <vector>

namespace {

// Search depth ceiling. Iterative deepening is bounded by the clock, so this only has to
// be past anything the budget can reach.
constexpr int kMaxDepth = 64;

// How the placement phase is played.
enum class PlacementMode { Policy, Random };

struct Options {
    int games = 10;
    int msPerPly = 200;
    std::uint64_t seed = 0;
    Latrunculi::MoveStyle style = Latrunculi::MoveStyle::Slide;
    Latrunculi::PayoffStyle payoff = Latrunculi::PayoffStyle::ConvexMargin;
    double komi = Latrunculi::kDefaultKomi;
    PlacementMode placement = PlacementMode::Policy;
    int rows = Latrunculi::kDefaultRows;
    int columns = Latrunculi::kDefaultColumns;
    int perSide = Latrunculi::kDefaultPerSide;
};

// Parses key=value arguments. An unknown key, a malformed value or an unrecognised
// enumerand is an error rather than something to fall back from: silently benchmarking a
// configuration the caller did not ask for produces numbers that look valid and are not,
// and a typo in a batch script would otherwise go unnoticed until the results confused
// somebody weeks later.
Options parseArgs(int argc, char** argv) {
    Options o;
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        const std::size_t eq = arg.find('=');
        if (eq == std::string::npos) {
            throw std::invalid_argument("bench: expected key=value, got \"" + arg + "\"");
        }
        const std::string key = arg.substr(0, eq);
        const std::string value = arg.substr(eq + 1);

        const auto positiveInt = [&key, &value]() {
            const int v = std::atoi(value.c_str());
            if (v <= 0) {
                throw std::invalid_argument("bench: " + key +
                                            " must be a positive integer, got \"" + value + "\"");
            }
            return v;
        };
        const auto oneOf = [&key, &value](const char* a, const char* b) {
            if (value == a) { return true; }
            if (value == b) { return false; }
            throw std::invalid_argument("bench: " + key + " must be \"" + a + "\" or \"" +
                                        b + "\", got \"" + value + "\"");
        };

        if (key == "games")          { o.games = positiveInt(); }
        else if (key == "ms")        { o.msPerPly = positiveInt(); }
        else if (key == "seed")      { o.seed = std::strtoull(value.c_str(), nullptr, 10); }
        else if (key == "rows")      { o.rows = positiveInt(); }
        else if (key == "columns")   { o.columns = positiveInt(); }
        else if (key == "perside")   { o.perSide = positiveInt(); }
        else if (key == "komi")      { o.komi = std::atof(value.c_str());
                                       Latrunculi::validateKomi(o.komi); }
        else if (key == "style")     { o.style = oneOf("slide", "stepleap")
                                           ? Latrunculi::MoveStyle::Slide
                                           : Latrunculi::MoveStyle::StepLeap; }
        else if (key == "payoff")    { o.payoff = oneOf("convex", "gradient")
                                           ? Latrunculi::PayoffStyle::ConvexMargin
                                           : Latrunculi::PayoffStyle::Gradient; }
        else if (key == "placement") { o.placement = oneOf("policy", "random")
                                           ? PlacementMode::Policy
                                           : PlacementMode::Random; }
        else {
            throw std::invalid_argument("bench: unknown option \"" + key + "\"");
        }
    }
    return o;
}

// Plays one complete game and returns it. Placement follows the shared PlacementPolicy
// (a random opening placement, then runs of searched ones); movement is always searched.
Latrunculi::Game playGame(const Options& o, std::uint64_t seed) {
    using namespace Latrunculi;
    Game game(o.rows, o.columns, o.perSide, o.style, o.payoff, o.komi);
    PlacementPolicy placement(seed);

    while (!game.isTerminal()) {
        const std::vector<AbsGame::MoveId> moves = game.getLegalMoves();
        if (moves.empty()) {
            throw std::runtime_error("bench: no legal moves in a non-terminal position");
        }

        AbsGame::MoveId mv = moves.front();
        bool searched = true;
        if (game.phase() == Phase::Placement) {
            // In Random mode nextIsRandom is not consulted at all -- every placement is
            // random for both sides, so neither gains anything from searching the opening.
            const bool random = (o.placement == PlacementMode::Random) ||
                                placement.nextIsRandom(game.currentPlayer());
            if (random) {
                mv = placement.pickRandomPlacement(game, moves);
                searched = false;
            }
        }
        if (searched) {
            const AbsGame::MoveId best =
                AbsGame::Searcher::bestMove(game, kMaxDepth, o.msPerPly);
            if (best == AbsGame::kPass || !game.isLegalMove(best)) {
                throw std::runtime_error("bench: search returned no usable move");
            }
            mv = best;
        }
        if (!game.applyMove(mv)) {
            throw std::runtime_error("bench: a chosen move was rejected");
        }
    }
    return game;
}

}  // namespace

int main(int argc, char** argv) {
    using namespace Latrunculi;
    try {
        const Options o = parseArgs(argc, argv);
        const std::uint64_t base = AbsGame::makeSeed(o.seed);

        std::cout << "Latrunculi bench: " << o.games << " games, "
                  << o.rows << "x" << o.columns << ", " << o.perSide << " per side, "
                  << (o.style == MoveStyle::Slide ? "slide" : "step/leap") << ", "
                  << (o.payoff == PayoffStyle::ConvexMargin ? "convex" : "gradient")
                  << " payoff, komi " << o.komi << ", "
                  << (o.placement == PlacementMode::Policy ? "policy" : "random")
                  << " placement, "
                  << o.msPerPly << " ms per searched ply\n"
                  << "base seed: " << base << "  (game g uses seed base+g)\n\n"
                  << statsHeader() << '\n';

        const auto started = std::chrono::steady_clock::now();
        std::vector<GameStats> all;
        all.reserve(static_cast<std::size_t>(o.games));
        for (int g = 0; g < o.games; ++g) {
            const Game finished = playGame(o, base + static_cast<std::uint64_t>(g));
            all.push_back(analyseGame(finished));
            std::cout << formatStatsRow(all.back(), g) << std::endl;
        }
        const auto elapsed = std::chrono::steady_clock::now() - started;
        const double seconds =
            std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count() / 1000.0;

        std::cout << '\n' << formatStatsSummary(all)
                  << "wall clock           " << seconds << " s\n";
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.
