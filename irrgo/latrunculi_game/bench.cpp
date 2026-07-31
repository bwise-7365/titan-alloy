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
//   latrunculi_bench ms=5000 games=10 --mb 0 0             <- the standard smoke run
//   latrunculi_bench ms=5000 games=10 --mb 2 0             <- the leak check
//   latrunculi_bench ms=5000 games=10 --mb 2 8231          <- break on a named block
//   latrunculi_bench help                                  <- full option list
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
//     threads    games played in parallel, one game per thread (default 1)
//     xml        y | n -- save each finished game as XML      (default n)
//     xmldir     directory for the XML files                  (default ".")
//
//     --mb <n> <m>  memory block tracking: level n, first suspect block m. Spelled and
//                   behaved exactly as in abzar's demo.cpp, including taking both
//                   values together -- an FSMB is meaningless without the level it
//                   applies to, so they are never separate options. The tracker is
//                   always compiled in, so no rebuild is needed to use it; run with
//                   `help` for the level descriptions. Default: no tracking.
//
// Game g uses seed base+g, so a run is reproducible from the printed base seed. That
// holds at any thread count: the seed follows the game index, not the worker, so
// threads= changes only how fast the run finishes and the order the rows appear in.
//
// THREADS AND MEMORY TRACKING TOGETHER: MB2 takes one global mutex per allocation on its
// tracked path, so at --mb level 1 or above the workers serialise on it and threads> 1
// buys almost nothing. Use threads for throughput at level 0, and a single thread when
// hunting a leak.
//
// READING THE MEMORY REPORT (--mb level 2 is the level worth using): the run is arranged so
// that everything a game allocates is destroyed before tracking stops, but the first use
// of an iostream or a locale facet allocates buffers that live until process exit and so
// appear in the report. Those are a fixed cost, not a leak. The reliable test is to run
// the same command twice with different game counts -- say games=1 and games=10 -- and
// compare: a real leak scales with the number of games, the iostream floor does not.

#include "Game.h"
#include "GameStats.h"
#include "GameXml.h"
#include "MemTrack.h"
#include "PlacementPolicy.h"
#include "Searcher.h"

#include <atomic>
#include <cerrno>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
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
    int threads = 1;
    unsigned mbLevel = 0;
    std::uint64_t fsmb = 0;
    bool saveXml = false;
    std::string xmlDir = ".";
    bool showHelp = false;
};

std::string usage() {
    return
        "Usage: latrunculi_bench [key=value ...] [--mb <level> <fsmb>]\n"
        "    games      number of games to play                     (default 10)\n"
        "    ms         search budget per searched ply, in ms       (default 200)\n"
        "    seed       base RNG seed; 0 means clock-derived        (default 0)\n"
        "    style      slide | stepleap                            (default slide)\n"
        "    payoff     convex | gradient                           (default convex)\n"
        "    komi       half-integer credited to player 1           (default 0.5)\n"
        "    placement  policy | random                             (default policy)\n"
        "    rows, columns, perside                                 (default 8, 10, 20)\n"
        "    threads    games played in parallel, one game per thread (default 1)\n"
        "    xml        y | n -- save each finished game as XML     (default n)\n"
        "    xmldir     directory for the XML files                 (default \".\")\n"
        + AbsGame::MemTrack::levelHelp();
}

[[noreturn]] void badValue(const std::string& what, const std::string& value,
                           const char* wanted) {
    throw std::invalid_argument("bench: " + what + " must be " + wanted + ", got \"" +
                                value + "\"");
}

// Numeric options go through these rather than std::atoi / std::atof, which report
// nothing: they return 0 for text that is not a number at all, and stop at the first
// character they cannot use. "ms=5x" would quietly become 5, and "seed=abc" would
// quietly become 0 -- which this driver reads as "derive a seed from the clock", so a
// typo would silently cost the run its reproducibility. These reject any value that is
// not wholly a number.
std::uint64_t wholeOf(const std::string& what, const std::string& value) {
    // Checked before strtoull because that function accepts a leading '-' and wraps it
    // around, so "-1" would otherwise arrive as 18446744073709551615.
    if (value.empty() || (value.find_first_not_of("0123456789") != std::string::npos)) {
        badValue(what, value, "a whole number");
    }
    errno = 0;
    const std::uint64_t v = std::strtoull(value.c_str(), nullptr, 10);
    if (errno == ERANGE) {
        badValue(what, value, "a whole number in range");
    }
    return v;
}

int positiveIntOf(const std::string& what, const std::string& value) {
    const std::uint64_t v = wholeOf(what, value);
    if ((v == 0) || (v > static_cast<std::uint64_t>(std::numeric_limits<int>::max()))) {
        badValue(what, value, "a positive integer");
    }
    return static_cast<int>(v);
}

double numberOf(const std::string& what, const std::string& value) {
    errno = 0;
    char* end = nullptr;
    const double v = std::strtod(value.c_str(), &end);
    if (value.empty() || (end != value.c_str() + value.size()) || (errno == ERANGE)) {
        badValue(what, value, "a number");
    }
    return v;
}

// Parses key=value arguments. An unknown key, a malformed value or an unrecognised
// enumerand is an error rather than something to fall back from: silently benchmarking a
// configuration the caller did not ask for produces numbers that look valid and are not,
// and a typo in a batch script would otherwise go unnoticed until the results confused
// somebody weeks later.
Options parseArgs(int argc, char** argv) {
    Options opt;

    // Fetch the argument following the flag at argv[idx], advancing idx. Guards against
    // reading argv[argc], as abzar's demo.cpp does for the same flag.
    const auto nextArg = [argc, argv](int& idx) -> std::string {
        if (idx + 1 >= argc) {
            throw std::invalid_argument(std::string("bench: missing value after ") +
                                        argv[idx]);
        }
        return argv[++idx];
    };

    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if ((arg == "help") || (arg == "--help") || (arg == "-h")) {
            opt.showHelp = true;
            return opt;
        }
        // Memory block tracking, spelled as in abzar's demo.cpp: --mb <level> <fsmb>.
        // Both values are taken together because an FSMB means nothing without the
        // level it applies to, so there is no separate option for it.
        if (arg == "--mb") {
            opt.mbLevel = static_cast<unsigned>(
                wholeOf("--mb tracking level", nextArg(i)));
            opt.fsmb = wholeOf("--mb first suspect block", nextArg(i));
            continue;
        }
        const std::size_t eq = arg.find('=');
        if (eq == std::string::npos) {
            throw std::invalid_argument("bench: expected key=value, got \"" + arg + "\"");
        }
        const std::string key = arg.substr(0, eq);
        const std::string value = arg.substr(eq + 1);

        const auto oneOf = [&key, &value](const char* a, const char* b) {
            if (value == a) { return true; }
            if (value == b) { return false; }
            throw std::invalid_argument("bench: " + key + " must be \"" + a + "\" or \"" +
                                        b + "\", got \"" + value + "\"");
        };

        if (key == "games")          { opt.games = positiveIntOf(key, value); }
        else if (key == "ms")        { opt.msPerPly = positiveIntOf(key, value); }
        else if (key == "seed")      { opt.seed = wholeOf(key, value); }
        else if (key == "rows")      { opt.rows = positiveIntOf(key, value); }
        else if (key == "columns")   { opt.columns = positiveIntOf(key, value); }
        else if (key == "perside")   { opt.perSide = positiveIntOf(key, value); }
        else if (key == "threads")   { opt.threads = positiveIntOf(key, value); }
        else if (key == "komi")      { opt.komi = numberOf(key, value);
                                       Latrunculi::validateKomi(opt.komi); }
        else if (key == "style")     { opt.style = oneOf("slide", "stepleap")
                                           ? Latrunculi::MoveStyle::Slide
                                           : Latrunculi::MoveStyle::StepLeap; }
        else if (key == "payoff")    { opt.payoff = oneOf("convex", "gradient")
                                           ? Latrunculi::PayoffStyle::ConvexMargin
                                           : Latrunculi::PayoffStyle::Gradient; }
        else if (key == "placement") { opt.placement = oneOf("policy", "random")
                                           ? PlacementMode::Policy
                                           : PlacementMode::Random; }
        else if (key == "xml")       { opt.saveXml = oneOf("y", "n"); }
        else if (key == "xmldir")    { opt.xmlDir = value; }
        else {
            throw std::invalid_argument("bench: unknown option \"" + key + "\"");
        }
    }
    return opt;
}

// "<dir>/latrunculi-0007.xml". Zero-padded to four digits so a 1000-game run sorts
// correctly in a file listing and in whatever reads it afterwards.
std::string xmlPathFor(const std::string& dir, int gameIndex) {
    std::ostringstream name;
    name << "latrunculi-" << std::setw(4) << std::setfill('0') << gameIndex << ".xml";
    return (std::filesystem::path(dir) / name.str()).string();
}

// Writes one finished game. Any failure throws: the point of the run is the corpus, and
// a batch that quietly dropped games would be discovered only during analysis.
void saveGameXml(const Latrunculi::Game& game, const std::string& dir, int gameIndex) {
    const std::string path = xmlPathFor(dir, gameIndex);
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("bench: could not open \"" + path + "\" for writing");
    }
    Latrunculi::writeGameXml(file, game);
    file.close();
    if (!file) {
        throw std::runtime_error("bench: failed while writing \"" + path + "\"");
    }
}

// Plays one complete game and returns it. Placement follows the shared PlacementPolicy
// (a random opening placement, then runs of searched ones); movement is always searched.
Latrunculi::Game playGame(const Options& opt, std::uint64_t seed) {
    using namespace Latrunculi;
    Game game(opt.rows, opt.columns, opt.perSide, opt.style, opt.payoff, opt.komi);
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
            const bool random = (opt.placement == PlacementMode::Random) ||
                                placement.nextIsRandom(game.currentPlayer());
            if (random) {
                mv = placement.pickRandomPlacement(game, moves);
                searched = false;
            }
        }
        if (searched) {
            const AbsGame::MoveId best =
                AbsGame::Searcher::bestMove(game, kMaxDepth, opt.msPerPly);
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

// Plays games [0, opt.games) across opt.threads workers, filling stats[g] for each.
//
// This is safe because a game shares nothing with any other. Every Game, PlacementPolicy
// and search is thread-local by construction, the engines hold no mutable global state,
// and the one static in the whole path -- Latrunculi's Zobrist table in Game.cpp -- is a
// function-local `static const` whose one-time initialisation the language already
// serialises, and which is read-only afterwards. Game g always uses seed base+g whichever
// worker draws it, so a run's results do not depend on the thread count; only the order
// the rows appear in does.
//
// Two things do need guarding, and are: std::cout (interleaved << chains from several
// threads would garble the table) and the first exception out of any worker, which is
// re-thrown on the calling thread rather than left to terminate the process.
void runGames(const Options& opt, std::uint64_t base, std::vector<Latrunculi::GameStats>& stats) {
    std::atomic<int> nextGame{0};
    std::mutex consoleMtx;
    std::mutex errorMtx;
    std::exception_ptr firstError;

    const auto worker = [&]() {
        for (;;) {
            const int g = nextGame.fetch_add(1);
            if (g >= opt.games) {
                return;
            }
            try {
                const Latrunculi::Game finished =
                    playGame(opt, base + static_cast<std::uint64_t>(g));
                if (opt.saveXml) {
                    saveGameXml(finished, opt.xmlDir, g);
                }
                stats[static_cast<std::size_t>(g)] = Latrunculi::analyseGame(finished);
                const std::string row =
                    Latrunculi::formatStatsRow(stats[static_cast<std::size_t>(g)], g);
                const std::lock_guard<std::mutex> lock(consoleMtx);
                std::cout << row << std::endl;
            } catch (...) {
                // Stop drawing work and keep the first failure; a later one is almost
                // always a consequence of it. Reported by the caller, which owns the
                // process's exit status.
                const std::lock_guard<std::mutex> lock(errorMtx);
                if (!firstError) {
                    firstError = std::current_exception();
                }
                nextGame.store(opt.games);
                return;
            }
        }
    };

    if (opt.threads <= 1) {
        // Run on the calling thread. Not merely an optimisation: it keeps the default
        // single-threaded run free of any thread machinery, so a memory-tracked run
        // reports on the engine alone.
        worker();
    } else {
        std::vector<std::thread> pool;
        pool.reserve(static_cast<std::size_t>(opt.threads));
        for (int t = 0; t < opt.threads; ++t) {
            pool.emplace_back(worker);
        }
        for (std::thread& t : pool) {
            t.join();
        }
    }

    if (firstError) {
        std::rethrow_exception(firstError);
    }
}

}  // namespace

int main(int argc, char** argv) {
    using namespace Latrunculi;
    try {
        const Options opt = parseArgs(argc, argv);
        if (opt.showHelp) {
            std::cout << usage();
            return 0;
        }
        const std::uint64_t base = AbsGame::makeSeed(opt.seed);

        if (opt.saveXml) {
            std::filesystem::create_directories(opt.xmlDir);
        }

        // The banner is printed BEFORE tracking starts on purpose: the first use of an
        // iostream allocates buffers that live until process exit, and counting those as
        // leaks would bury the per-game signal the run exists to find.
        std::cout << "Latrunculi bench: " << opt.games << " games, "
                  << opt.rows << "x" << opt.columns << ", " << opt.perSide << " per side, "
                  << (opt.style == MoveStyle::Slide ? "slide" : "step/leap") << ", "
                  << (opt.payoff == PayoffStyle::ConvexMargin ? "convex" : "gradient")
                  << " payoff, komi " << opt.komi << ", "
                  << (opt.placement == PlacementMode::Policy ? "policy" : "random")
                  << " placement, "
                  << opt.msPerPly << " ms per searched ply, "
                  << opt.threads << (opt.threads == 1 ? " thread\n" : " threads\n")
                  << "base seed: " << base << "  (game g uses seed base+g)\n"
                  << "XML: " << (opt.saveXml ? opt.xmlDir : std::string("not saved"))
                  << "   memory tracking level: " << opt.mbLevel;
        if (opt.fsmb > 0) {
            std::cout << ", first suspect block " << opt.fsmb;
        }
        std::cout << "\n\n" << statsHeader() << '\n';

        AbsGame::MemTrack::start(opt.mbLevel, opt.fsmb);

        // Everything the run allocates is created and destroyed inside this scope, so
        // the tracker's report describes the engine and not the harness.
        {
            const auto started = std::chrono::steady_clock::now();
            std::vector<GameStats> all(static_cast<std::size_t>(opt.games));
            runGames(opt, base, all);
            const auto elapsed = std::chrono::steady_clock::now() - started;
            const double seconds =
                std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()
                / 1000.0;

            std::cout << '\n' << formatStatsSummary(all)
                      << "wall clock           " << seconds << " s\n";
        }

        AbsGame::MemTrack::stop();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.
