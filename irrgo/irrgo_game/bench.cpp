// Copyright Ben Paul Wise. All Rights Reserved.
//
// Batch self-play for IrrGo under MCTS. Plays N games on freshly generated boards and
// prints a per-game result line plus an aggregate summary; optionally writes each
// finished game as XML (doc/irrgo.xsd) for later analysis.
//
// Deliberately free of Qt, for two reasons: this is meant to be run in bulk, and the MB2
// memory tracker cannot coexist with Qt (see absgame/MemTrack.h).
//
// Usage: named key=value arguments in any order, matching latrunculi_bench's convention.
//
//   irrgo_bench secs=5 games=10 --mb 0 0              <- the standard smoke run
//   irrgo_bench secs=5 games=10 --mb 2 0              <- the leak check
//   irrgo_bench secs=5 games=10 --mb 2 8231           <- break on a named block
//   irrgo_bench games=1000 secs=5 xml=y xmldir=corpus
//   irrgo_bench help                                  <- full option list
//
//     games      number of games to play                       (default 10)
//     secs       MCTS budget per move, in seconds               (default 5)
//     seed       base RNG seed; 0 means clock-derived           (default 0)
//     rows, cols board size                                     (default 9, 13)
//     irregular  y | n -- hand-scratched graph or square grid   (default y)
//     maxdeg     max edges per node, irregular boards only      (default 4)
//     komi       points added to White                          (default 1.5)
//     maxplies   ply cap per game; exceeding it is an error     (default 0 = 8x nodes)
//     threads    games played in parallel, one game per thread  (default 1)
//     xml        y | n -- save each finished game as XML        (default n)
//     xmldir     directory for the XML files                    (default ".")
//
//     --mb <n> <m>  memory block tracking: level n, first suspect block m. Spelled
//                   and behaved exactly as in abzar's demo.cpp, including taking both
//                   values together -- an FSMB is meaningless without the level it
//                   applies to, so they are never separate options. The tracker is
//                   always compiled in, so no rebuild is needed to use it; run with
//                   `help` for the level descriptions. Default: no tracking.
//
// Game g uses seed base+g, which on an irregular board generates that game's BOARD: every
// game in a run gets its own graph and each XML file is self-describing. The seed follows
// the game index rather than the worker, so threads= changes only how fast the run
// finishes and the order the rows appear in.
//
// THE BOARD IS REPRODUCIBLE; THE GAME IS NOT. Searcher::mcts seeds its own generator from
// std::random_device on every call, so replaying a seed reproduces the position but not
// the play. The search is also budgeted by wall clock, so its iteration count moves with
// machine speed and load -- two runs of the same binary on the same board diverge. Do not
// read a difference between two runs as a difference between two builds.
//
// TERM/PLY AND TERM%: how much real evidence the Monte Carlo part of MCTS actually got.
// The estimate at a node is an average over playout outcomes, so it is worth what it
// claims only when playouts reach outcomes; one stopped by the depth ceiling is scored by
// staticEval() on an unfinished position instead. term/ply is terminal playouts per move
// decision -- how much finished evidence each move rested on -- and term% is the fraction
// of playouts that finished, which separates "the budget bought few playouts" from "the
// playouts did not reach the end".
//
// Both are measured over the first nodeCount/2 plies only, and divided by the plies
// actually played in that window (so a game shorter than the window is not diluted). The
// restriction matters: the opening has the most options and the fewest finished playouts,
// while an endgame with three legal moves left terminates nearly every playout, so a
// whole-game average would report health for a search that saw nothing during the part of
// the game that decides it.
//
// THREADS AND MEMORY TRACKING TOGETHER: MB2 takes one global mutex per allocation on its
// tracked path, so at --mb level 1 or above the workers serialise on it and threads> 1
// buys almost nothing. Use threads for throughput at level 0, and a single thread when
// hunting a leak. Note also that each worker holds its own MCTS tree, so peak memory
// scales with the thread count.
//
// READING THE MEMORY REPORT (--mb level 2 is the level worth using): the run is arranged so
// that everything a game allocates is destroyed before tracking stops, but the first use
// of an iostream or a locale facet allocates buffers that live until process exit and so
// appear in the report. Those are a fixed cost, not a leak. The reliable test is to run
// the same command twice with different game counts -- say games=1 and games=10 -- and
// compare: a real leak scales with the number of games, the iostream floor does not.

#include "Game.h"
#include "GameXml.h"
#include "Graph.h"
#include "IrregularGraph.h"
#include "MemTrack.h"
#include "RectangularGraph.h"
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
#include <locale>
#include <memory>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

namespace {

struct Options {
    int games = 10;
    int secsPerMove = 5;
    std::uint64_t seed = 0;
    int rows = 9;
    int cols = 13;
    bool irregular = true;
    int maxDegree = 4;
    double komi = 1.5;
    int maxPlies = 0;  // 0 = derive from the node count
    int threads = 1;
    unsigned mbLevel = 0;
    std::uint64_t fsmb = 0;
    bool saveXml = false;
    std::string xmlDir = ".";
    bool showHelp = false;
};

std::string usage() {
    return
        "Usage: irrgo_bench [key=value ...] [--mb <level> <fsmb>]\n"
        "    games      number of games to play                       (default 10)\n"
        "    secs       MCTS budget per move, in seconds               (default 5)\n"
        "    seed       base RNG seed; 0 means clock-derived           (default 0)\n"
        "    rows, cols board size                                     (default 9, 13)\n"
        "    irregular  y | n -- hand-scratched graph or square grid   (default y)\n"
        "    maxdeg     max edges per node, irregular boards only      (default 4)\n"
        "    komi       points added to White                          (default 1.5)\n"
        "    maxplies   ply cap per game; exceeding it is an error     (default 0 = 8x nodes)\n"
        "    threads    games played in parallel, one game per thread  (default 1)\n"
        "    xml        y | n -- save each finished game as XML        (default n)\n"
        "    xmldir     directory for the XML files                    (default \".\")\n"
        + AbsGame::MemTrack::levelHelp();
}

[[noreturn]] void badValue(const std::string& what, const std::string& value,
                           const char* wanted) {
    throw std::invalid_argument("bench: " + what + " must be " + wanted + ", got \"" +
                                value + "\"");
}

// Numeric options go through these rather than std::atoi / std::atof, which report
// nothing: they return 0 for text that is not a number at all, and stop at the first
// character they cannot use. "secs=5x" would quietly become 5, and "seed=abc" would
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

int nonNegativeIntOf(const std::string& what, const std::string& value) {
    const std::uint64_t v = wholeOf(what, value);
    if (v > static_cast<std::uint64_t>(std::numeric_limits<int>::max())) {
        badValue(what, value, "a non-negative integer in range");
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
// enumerand is an error rather than something to fall back from: silently running a
// configuration the caller did not ask for produces a corpus that looks valid and is not,
// and a typo in a batch script would otherwise go unnoticed until the analysis confused
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

        const auto yesNo = [&key, &value]() {
            if (value == "y") { return true; }
            if (value == "n") { return false; }
            throw std::invalid_argument("bench: " + key + " must be \"y\" or \"n\", got \"" +
                                        value + "\"");
        };

        if (key == "games")          { opt.games = positiveIntOf(key, value); }
        else if (key == "secs")      { opt.secsPerMove = positiveIntOf(key, value); }
        else if (key == "seed")      { opt.seed = wholeOf(key, value); }
        else if (key == "rows")      { opt.rows = positiveIntOf(key, value); }
        else if (key == "cols")      { opt.cols = positiveIntOf(key, value); }
        else if (key == "irregular") { opt.irregular = yesNo(); }
        else if (key == "maxdeg")    { opt.maxDegree = positiveIntOf(key, value); }
        else if (key == "komi")      { opt.komi = numberOf(key, value); }
        else if (key == "maxplies")  { opt.maxPlies = nonNegativeIntOf(key, value); }
        else if (key == "threads")   { opt.threads = positiveIntOf(key, value); }
        else if (key == "xml")       { opt.saveXml = yesNo(); }
        else if (key == "xmldir")    { opt.xmlDir = value; }
        else {
            throw std::invalid_argument("bench: unknown option \"" + key + "\"");
        }
    }
    return opt;
}

std::unique_ptr<IrrGo::Graph> makeGraph(const Options& opt, std::uint64_t seed) {
    if (opt.irregular) {
        return std::make_unique<IrrGo::IrregularGraph>(opt.rows, opt.cols, opt.maxDegree, seed);
    }
    return std::make_unique<IrrGo::RectangularGraph>(opt.rows, opt.cols);
}

// "<dir>/irrgo-0007.xml". Zero-padded to four digits so a 1000-game run sorts correctly
// in a file listing and in whatever reads it afterwards.
std::string xmlPathFor(const std::string& dir, int gameIndex) {
    std::ostringstream name;
    name << "irrgo-" << std::setw(4) << std::setfill('0') << gameIndex << ".xml";
    return (std::filesystem::path(dir) / name.str()).string();
}

// Writes one finished game. Any failure throws: the point of the run is the corpus, and
// a batch that quietly dropped games would be discovered only during analysis.
void saveGameXml(const IrrGo::Game& game, const IrrGo::Graph& graph,
                 const std::string& dir, int gameIndex) {
    const std::string path = xmlPathFor(dir, gameIndex);
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("bench: could not open \"" + path + "\" for writing");
    }
    // setupCount 0: a bench game places no stones before play, so the whole history is
    // the move list and <Position type="setup"> is empty.
    IrrGo::writeGameXml(file, game, graph, 0);
    file.close();
    if (!file) {
        throw std::runtime_error("bench: failed while writing \"" + path + "\"");
    }
}

// How much real evidence MCTS gathered over a game's OPENING, which is the only part of a
// game where the measure compares across positions.
//
// The Monte Carlo estimate is an average over playout outcomes, so it is only worth what
// it claims when playouts reach outcomes. Counting terminals per ply says how much of
// that a move decision actually rested on.
//
// Measured over the first nodeCount/2 plies rather than the whole game, because the two
// ends of a game are not comparable: the opening has the most options and the fewest
// finished playouts, while an endgame with three legal moves left terminates almost every
// playout. Averaging over a long endgame tail would report a healthy number for a search
// that saw nothing during the part of the game that decides it.
struct WindowStats {
    long long plies = 0;      // = min(nodeCount/2, game length), the divisor
    long long terminals = 0;  // playouts in the window that reached a true terminal
    long long rollouts = 0;   // playouts in the window, terminal or not
};

// Plays one complete game on `graph`. Every move comes from MCTS, including the passes
// that end the game: getLegalMoves() always offers kPass, so the search chooses it once
// nothing on the board is worth more.
IrrGo::Game playGame(const Options& opt, const IrrGo::Graph& graph, WindowStats& window) {
    IrrGo::Game game(graph, opt.komi);

    // Half the intersections, truncated: 200 nodes measure over 100 plies, 117 over 58.
    const int windowPlies = graph.nodeCount() / 2;

    // A cap on run-away games. Two consecutive passes end a game, and MCTS reliably finds
    // them, but a rule or evaluation change could stop it doing so -- and a 1000-game
    // batch that wedged on game 400 with no message would waste a day. Exceeding the cap
    // is reported as the failure it is rather than truncating the game into the corpus.
    const int cap = (opt.maxPlies > 0) ? opt.maxPlies : (8 * graph.nodeCount());

    int plies = 0;
    while (!game.isTerminal()) {
        if (plies >= cap) {
            throw std::runtime_error("bench: game exceeded the " + std::to_string(cap) +
                                     "-ply cap without reaching two consecutive passes");
        }
        // The stats are only collected inside the window; past it the search runs
        // exactly as before and the out-parameter is not asked for.
        const bool inWindow = (plies < windowPlies);
        AbsGame::SearchStats searchStats;
        const AbsGame::MoveId mv =
            AbsGame::Searcher::mcts(game, opt.secsPerMove, inWindow ? &searchStats : nullptr);
        if (!game.applyMove(mv)) {
            throw std::runtime_error("bench: the search returned a move the game rejected");
        }
        if (inWindow) {
            window.plies += 1;
            window.terminals += searchStats.terminalRollouts;
            window.rollouts += searchStats.rollouts;
        }
        ++plies;
    }
    return game;
}

// A ratio, or "-" when there is nothing to divide by. A zero-length window is not a rate
// of zero, and printing it as one would read as "MCTS saw no terminals" when the truth is
// that nothing was measured.
std::string ratioOr(double numerator, double denominator, int places) {
    if (denominator <= 0.0) {
        return "-";
    }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(places) << (numerator / denominator);
    return out.str();
}

std::string resultRow(int gameIndex, const IrrGo::Game& game, const WindowStats& window,
                      double seconds) {
    const IrrGo::GameResult r = game.score();
    std::ostringstream row;
    row.imbue(std::locale::classic());
    row << std::setw(5) << gameIndex
        << std::setw(8) << game.moveHistory().size()
        << std::setw(10) << std::fixed << std::setprecision(1) << r.blackScore
        << std::setw(10) << r.whiteScore
        << std::setw(8) << (r.winner == IrrGo::Player::Black ? "Black" : "White")
        << std::setw(10) << std::setprecision(1) << seconds
        << std::setw(12) << ratioOr(static_cast<double>(window.terminals),
                                    static_cast<double>(window.plies), 1)
        << std::setw(9) << ratioOr(100.0 * static_cast<double>(window.terminals),
                                   static_cast<double>(window.rollouts), 2);
    return row.str();
}

std::string resultHeader() {
    std::ostringstream head;
    head << std::setw(5) << "game" << std::setw(8) << "plies"
         << std::setw(10) << "black" << std::setw(10) << "white"
         << std::setw(8) << "winner" << std::setw(10) << "sec"
         << std::setw(12) << "term/ply" << std::setw(9) << "term%";
    return head.str();
}

// What one finished game contributes to the run summary.
struct GameOutcome {
    long long plies = 0;
    bool blackWon = false;
    WindowStats window;
};

// Plays games [0, opt.games) across opt.threads workers, filling outcome[g] for each.
//
// This is safe because a game shares nothing with any other. Every Graph, Game and search
// is thread-local by construction, and the engines hold no mutable global state: IrrGo's
// Zobrist tables are per-instance members (Game::initZobrist), and the searcher's one
// piece of shared state -- the old Searcher::terminalCount -- is gone. Game g always uses
// seed base+g whichever worker draws it, so a run's results do not depend on the thread
// count; only the order the rows appear in does.
//
// Two things do need guarding, and are: std::cout (interleaved << chains from several
// threads would garble the table) and the first exception out of any worker, which is
// re-thrown on the calling thread rather than left to terminate the process.
void runGames(const Options& opt, std::uint64_t base, std::vector<GameOutcome>& outcome) {
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
                const auto gameStarted = std::chrono::steady_clock::now();
                const std::unique_ptr<IrrGo::Graph> graph =
                    makeGraph(opt, base + static_cast<std::uint64_t>(g));
                WindowStats window;
                const IrrGo::Game finished = playGame(opt, *graph, window);
                const auto gameElapsed = std::chrono::steady_clock::now() - gameStarted;
                const double gameSeconds =
                    static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
                        gameElapsed).count()) / 1000.0;

                if (opt.saveXml) {
                    saveGameXml(finished, *graph, opt.xmlDir, g);
                }
                GameOutcome& slot = outcome[static_cast<std::size_t>(g)];
                slot.plies = static_cast<long long>(finished.moveHistory().size());
                slot.blackWon = (finished.score().winner == IrrGo::Player::Black);
                slot.window = window;

                const std::string row = resultRow(g, finished, window, gameSeconds);
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
        std::cout << "IrrGo bench: " << opt.games << " games, "
                  << opt.rows << "x" << opt.cols << ", "
                  << (opt.irregular ? "irregular" : "rectangular") << " board";
        if (opt.irregular) {
            std::cout << " (max degree " << opt.maxDegree << ")";
        }
        std::cout << ", komi " << opt.komi << ", MCTS "
                  << opt.secsPerMove << " s per move, "
                  << opt.threads << (opt.threads == 1 ? " thread\n" : " threads\n")
                  << "base seed: " << base << "  (game g uses seed base+g"
                  << (opt.irregular ? ", which also generates its board)" : ")") << '\n'
                  << "XML: " << (opt.saveXml ? opt.xmlDir : std::string("not saved"))
                  << "   memory tracking level: " << opt.mbLevel;
        if (opt.fsmb > 0) {
            std::cout << ", first suspect block " << opt.fsmb;
        }
        std::cout << "\n\n" << resultHeader() << '\n';

        AbsGame::MemTrack::start(opt.mbLevel, opt.fsmb);

        // Everything the run allocates is created and destroyed inside this scope, so
        // the tracker's report describes the engine and not the harness.
        {
            const auto started = std::chrono::steady_clock::now();
            std::vector<GameOutcome> outcome(static_cast<std::size_t>(opt.games));
            runGames(opt, base, outcome);

            int blackWins = 0;
            long long totalPlies = 0;
            WindowStats runWindow;
            for (const GameOutcome& result : outcome) {
                totalPlies += result.plies;
                if (result.blackWon) {
                    ++blackWins;
                }
                // Pooled rather than averaged over per-game rates: every ply in the run
                // then carries the same weight, so one short game cannot swing the figure.
                runWindow.plies += result.window.plies;
                runWindow.terminals += result.window.terminals;
                runWindow.rollouts += result.window.rollouts;
            }

            const auto elapsed = std::chrono::steady_clock::now() - started;
            const double seconds =
                static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(
                    elapsed).count()) / 1000.0;

            std::cout << "\nblack wins           " << blackWins << " / " << opt.games
                      << "\nmean plies           "
                      << (static_cast<double>(totalPlies) / opt.games)
                      << "\nterminals per ply    "
                      << ratioOr(static_cast<double>(runWindow.terminals),
                                 static_cast<double>(runWindow.plies), 1)
                      << "   (over " << runWindow.plies << " opening plies)"
                      << "\nplayouts terminal    "
                      << ratioOr(100.0 * static_cast<double>(runWindow.terminals),
                                 static_cast<double>(runWindow.rollouts), 2)
                      << " %   (of " << runWindow.rollouts << " playouts)"
                      << "\nwall clock           " << seconds << " s\n";
        }

        AbsGame::MemTrack::stop();
        return 0;
    } catch (const std::exception& ex) {
        std::cerr << "error: " << ex.what() << '\n';
        return 1;
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.
