// Copyright Ben Paul Wise. All Rights Reserved.

// Unit tests for the placement-phase evaluation (PlacementEval.h) and its wiring into
// Game. Each fixture is one of the seven diagrams in
// doc/2026-08-24-latrunculi-placement-heuristics.md, hand-built as an ASCII board with
// hand-computed expected term counts, so a term drifting from its documented meaning
// fails a named test. The harness follows palette_core/tests/test_support.h: no
// framework, non-zero exit on failure for CTest.

#include "Eval.h"
#include "Game.h"
#include "PlacementEval.h"

#include <cmath>
#include <cstdio>
#include <functional>
#include <string>
#include <vector>

namespace {

int g_checks = 0;
int g_failures = 0;

void record(bool ok, const char* expr, const char* file, int line) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::printf("  FAIL [%s:%d] %s\n", file, line, expr);
    }
}

void expectThrows(const std::function<void()>& fn, const char* what,
                  const char* file, int line) {
    ++g_checks;
    bool threw = false;
    try {
        fn();
    } catch (...) {
        threw = true;
    }
    if (!threw) {
        ++g_failures;
        std::printf("  FAIL [%s:%d] expected exception: %s\n", file, line, what);
    }
}

#define CHECK(cond) record((cond), #cond, __FILE__, __LINE__)
#define CHECK_EQ(a, b) record((a) == (b), #a " == " #b, __FILE__, __LINE__)
#define CHECK_NEAR(a, b, tol) \
    record(std::fabs((a) - (b)) <= (tol), #a " ~= " #b, __FILE__, __LINE__)
#define CHECK_THROWS(stmt) expectThrows([&]() { stmt; }, #stmt, __FILE__, __LINE__)

using Latrunculi::Cell;
using Latrunculi::EvalWeights;
using Latrunculi::Game;
using Latrunculi::MoveStyle;
using Latrunculi::Phase;
using Latrunculi::PlacementTerms;
using Latrunculi::placementScore;
using Latrunculi::placementTerms;
using Latrunculi::positionalScore;
using Latrunculi::positionalTerms;
using Latrunculi::seamRamp;

// 'B' = player 0 Free, 'W' = player 1 Free, 'b'/'w' = Bound, '.' = empty. Throws on a
// ragged or unknown character so a typo in a fixture fails loudly instead of silently
// testing a different board.
std::vector<Cell> parseBoard(const std::vector<std::string>& rows) {
    if (rows.empty()) {
        throw std::invalid_argument("test: empty board");
    }
    const std::size_t columns = rows.front().size();
    std::vector<Cell> cells;
    cells.reserve(rows.size() * columns);
    for (const std::string& r : rows) {
        if (r.size() != columns) {
            throw std::invalid_argument("test: ragged board row");
        }
        for (char c : r) {
            switch (c) {
                case '.': cells.push_back(Cell::Empty); break;
                case 'B': cells.push_back(Cell::P0Free); break;
                case 'W': cells.push_back(Cell::P1Free); break;
                case 'b': cells.push_back(Cell::P0Bound); break;
                case 'w': cells.push_back(Cell::P1Bound); break;
                default: throw std::invalid_argument("test: unknown board character");
            }
        }
    }
    return cells;
}

const MoveStyle kSlide = MoveStyle::Slide;

// ── Fixture 1: edge wall (diagram latrunculi-placement-walls) ────────────────
void testEdgeWall() {
    const auto cells = parseBoard({
        "......",
        "......",
        "......",
        "......",
        "..WW..",
        ".BBBB.",
    });
    const PlacementTerms black = placementTerms(cells, 6, 6, 0, kSlide);
    // F3: a full edge wall is uncapturable, ends included -- no open axis anywhere.
    CHECK_EQ(black.vulnerableAxes, 0);
    CHECK_EQ(black.oneMoveCapturable, 0);
    CHECK_EQ(black.notchExposure, 0);
    CHECK_EQ(black.spearheadPairs, 0);
    // F11: the wall's own stones are not strikers (at most one mobile direction each).
    CHECK_EQ(black.strikers, 0);

    const PlacementTerms white = placementTerms(cells, 6, 6, 1, kSlide);
    // The white pair pressing the wall is safe along its own axis but open above/below.
    CHECK_EQ(white.vulnerableAxes, 2);
    // Open vertically, but no black striker can reach a completing square yet.
    CHECK_EQ(white.oneMoveCapturable, 0);
    CHECK_EQ(white.strikers, 2);
}

// ── Fixture 2: the 2x2 block (diagram latrunculi-placement-2x2) ──────────────
void testBlock2x2() {
    const auto cells = parseBoard({
        ".W...",
        "WBB..",
        ".BBW.",
        "..W..",
        ".....",
    });
    const PlacementTerms black = placementTerms(cells, 5, 5, 0, kSlide);
    // F3: fully surrounded and still uncapturable -- every axis holds a friend.
    CHECK_EQ(black.vulnerableAxes, 0);
    CHECK_EQ(black.oneMoveCapturable, 0);
    // Adjacent enemies on both axes are NOT notch exposure when friends block the
    // completions; this is exactly the refinement the 2x2 forces.
    CHECK_EQ(black.notchExposure, 0);
    // Two pairs aim at adjacent enemies with an empty square beyond: the right column
    // at W(2,3) and the bottom row at W(3,2).
    CHECK_EQ(black.spearheadPairs, 2);
    CHECK_EQ(black.diagonalSupport, 0);
    // F11 again: total safety costs total immobility.
    CHECK_EQ(black.strikers, 0);

    const PlacementTerms white = placementTerms(cells, 5, 5, 1, kSlide);
    CHECK_EQ(white.vulnerableAxes, 6);
    CHECK_EQ(white.oneMoveCapturable, 0);
}

// ── Fixture 3: the gapped pair (diagram latrunculi-placement-gapped-pair) ────
void testGappedPair() {
    const auto cells = parseBoard({
        "..W..",
        ".....",
        ".B.B.",
        ".....",
        "....W",
    });
    const PlacementTerms black = placementTerms(cells, 5, 5, 0, kSlide);
    // F5: the gap square is denied to the enemy.
    CHECK_EQ(black.deniedSquares, 1);
    CHECK_EQ(black.vulnerableAxes, 4);
    CHECK_EQ(black.strikers, 2);
    CHECK_EQ(black.spearheadPairs, 0);

    // The rules back the term up: White can neither place into the gap nor slide onto
    // it, while an adjacent stop short of the gap is fine.
    const int gap = 2 * 5 + 2;
    {
        Game placing(5, 5, 4, parseBoard({
                         "..W..",
                         ".....",
                         ".B.B.",
                         ".....",
                         "....W",
                     }),
                     Phase::Placement, 1, 2, 2);
        CHECK(!placing.isLegalMove(placing.placementMove(gap)));
    }
    {
        Game moving(5, 5, 2, parseBoard({
                        "..W..",
                        ".....",
                        ".B.B.",
                        ".....",
                        "....W",
                    }),
                    Phase::Movement, 1, 2, 2);
        const int from = 0 * 5 + 2;  // W at (0,2)
        CHECK(!moving.isLegalMove(moving.movementMove(from, gap)));
        CHECK(moving.isLegalMove(moving.movementMove(from, 1 * 5 + 2)));
    }
}

// ── Fixture 4: the diagonal's cross-fire (diagram latrunculi-placement-diagonal) ──
void testDiagonal() {
    const auto open = parseBoard({
        ".....",
        "....B",
        ".B...",
        "..B..",
        ".....",
    });
    const PlacementTerms blackOpen = placementTerms(open, 5, 5, 0, kSlide);
    // F6: one diagonal with both notch squares empty.
    CHECK_EQ(blackOpen.diagonalSupport, 1);
    CHECK_EQ(blackOpen.vulnerableAxes, 5);
    CHECK_EQ(blackOpen.strikers, 3);

    const auto entered = parseBoard({
        ".....",
        "....B",
        ".BW..",
        "..B..",
        ".....",
    });
    const PlacementTerms blackEntered = placementTerms(entered, 5, 5, 0, kSlide);
    // The notch is occupied, so the diagonal no longer scores as open support...
    CHECK_EQ(blackEntered.diagonalSupport, 0);
    // ...and attacking the diagonal did not expose it.
    CHECK_EQ(blackEntered.oneMoveCapturable, 0);

    const PlacementTerms whiteEntered = placementTerms(entered, 5, 5, 1, kSlide);
    // The intruder is half-pinned on both axes (F7)...
    CHECK_EQ(whiteEntered.notchExposure, 1);
    // ...and dies to one slide: B(1,4) reaches the completing square (1,2).
    CHECK_EQ(whiteEntered.oneMoveCapturable, 1);
}

// ── Fixture 5: contact and the spearhead (diagram latrunculi-placement-spearhead) ──
void testSpearhead() {
    const auto unbacked = parseBoard({
        "....B",
        ".....",
        "..BW.",
        ".....",
        ".W...",
    });
    // F4: unbacked contact is a mutual half-pin -- both sides are en prise.
    CHECK_EQ(placementTerms(unbacked, 5, 5, 0, kSlide).oneMoveCapturable, 1);
    CHECK_EQ(placementTerms(unbacked, 5, 5, 1, kSlide).oneMoveCapturable, 1);
    CHECK_EQ(placementTerms(unbacked, 5, 5, 0, kSlide).spearheadPairs, 0);

    const auto backed = parseBoard({
        "....B",
        ".....",
        ".BBW.",
        ".....",
        ".....",
    });
    const PlacementTerms black = placementTerms(backed, 5, 5, 0, kSlide);
    const PlacementTerms white = placementTerms(backed, 5, 5, 1, kSlide);
    // The backer turns the same contact one-sided: a spearhead, and only White en prise.
    CHECK_EQ(black.spearheadPairs, 1);
    CHECK_EQ(black.oneMoveCapturable, 0);
    CHECK_EQ(white.oneMoveCapturable, 1);
    CHECK_EQ(black.vulnerableAxes, 3);
}

// ── Fixture 6: corners (diagram latrunculi-placement-corner) ─────────────────
void testCorner() {
    const auto anchor = parseBoard({
        "BBW..",
        "W....",
        ".....",
        ".....",
        ".....",
    });
    const PlacementTerms black = placementTerms(anchor, 5, 5, 0, kSlide);
    // F3/H1: the corner pair is uncapturable -- the friend closes the corner trap and
    // the edge closes everything else.
    CHECK_EQ(black.vulnerableAxes, 0);
    CHECK_EQ(black.oneMoveCapturable, 0);

    const auto halfPin = parseBoard({
        "WB...",
        ".....",
        ".....",
        "..W..",
        ".....",
    });
    const PlacementTerms pinned = placementTerms(halfPin, 5, 5, 0, kSlide);
    // F8: the enemy corner half-pins the edge neighbour forever, and the striker at
    // W(3,2) can complete at (0,2) in one slide.
    CHECK_EQ(pinned.oneMoveCapturable, 1);
    CHECK_EQ(pinned.vulnerableAxes, 1);

    const PlacementTerms whiteCorner = placementTerms(halfPin, 5, 5, 1, kSlide);
    // The white corner disc itself is corner-trap vulnerable (both squares beside it
    // open to Black), but not capturable in one move -- no black striker reaches.
    CHECK_EQ(whiteCorner.vulnerableAxes, 3);
    CHECK_EQ(whiteCorner.oneMoveCapturable, 0);
}

// ── Fixture 7: the phase seam (diagram latrunculi-placement-seam) ────────────
void testSeam() {
    const auto cells = parseBoard({
        "....W.",
        "......",
        "......",
        "..WB..",
        "......",
        "......",
    });
    const PlacementTerms black = placementTerms(cells, 6, 6, 0, kSlide);
    // A1/F10: adjacent enemy jaw + empty, reachable completing square = dead on the
    // first movement ply.
    CHECK_EQ(black.oneMoveCapturable, 1);
    CHECK_EQ(black.vulnerableAxes, 2);
    CHECK_EQ(black.strikers, 1);

    // The seam ramp: mild at the start of placement, full at the end, cubic between.
    CHECK_NEAR(seamRamp(0.0), 0.25, 1e-12);
    CHECK_NEAR(seamRamp(0.5), 0.25 + 0.75 * 0.125, 1e-12);
    CHECK_NEAR(seamRamp(1.0), 1.0, 1e-12);
    const EvalWeights w;
    const double early = placementScore(black, 0.0, 20, w);
    const double late = placementScore(black, 1.0, 20, w);
    CHECK_NEAR(late - early, w.oneMoveCapturable * (1.0 - 0.25), 1e-12);
}

// ── Argument validation ──────────────────────────────────────────────────────
void testValidation() {
    const auto cells = parseBoard({"..", ".."});
    CHECK_THROWS(placementTerms(cells, 3, 3, 0, kSlide));
    CHECK_THROWS(placementTerms(cells, 2, 2, 2, kSlide));
    const PlacementTerms terms;
    CHECK_THROWS(placementScore(terms, -0.1, 20));
    CHECK_THROWS(placementScore(terms, 1.1, 20));
    CHECK_THROWS(placementScore(terms, std::nan(""), 20));
    CHECK_THROWS(placementScore(terms, 0.5, 0));

    EvalWeights bad;
    bad.spearheadPairs = std::nan("");
    CHECK_THROWS(Latrunculi::validateEvalWeights(bad));
    Game game;
    CHECK_THROWS(game.setEvalWeights(bad));
    EvalWeights negated;
    negated.threat = -0.25;  // sign flips are legitimate sweep territory
    game.setEvalWeights(negated);
    CHECK_NEAR(game.evalWeights().threat, -0.25, 1e-12);
}

// ── Wiring into Game: staticEval decomposition and moveOrderScore ────────────
void testGameWiring() {
    const std::vector<std::string> rows = {
        "....W.",
        "......",
        "......",
        "..WB..",
        "......",
        "......",
    };
    Game game(6, 6, 6, parseBoard(rows), Phase::Placement, 0, 1, 2);

    // staticEval during placement must equal the sum of its three published parts,
    // each recomputed here through the public pure functions.
    const auto cells = parseBoard(rows);
    const double material =
        (game.freeDiscs(0) + Latrunculi::immobilizationDiscount * game.boundDiscs(0)) -
        (game.freeDiscs(1) + Latrunculi::immobilizationDiscount * game.boundDiscs(1) +
         game.komi());
    const EvalWeights w = game.evalWeights();
    const double positional =
        positionalScore(positionalTerms(cells, 6, 6, 0, kSlide), w) -
        positionalScore(positionalTerms(cells, 6, 6, 1, kSlide), w);
    const double placement =
        placementScore(placementTerms(cells, 6, 6, 0, kSlide), 1.0 / 6.0, 6, w) -
        placementScore(placementTerms(cells, 6, 6, 1, kSlide), 2.0 / 6.0, 6, w);
    CHECK_NEAR(game.staticEval(), material + positional + placement, 1e-9);

    // moveOrderScore for a placement is the scored after-position, to the documented
    // scale. Recompute independently for the square right of the black disc.
    const int square = 3 * 6 + 4;
    CHECK(game.isLegalMove(game.placementMove(square)));
    auto after = cells;
    after[static_cast<std::size_t>(square)] = Cell::P0Free;
    const double score =
        placementScore(placementTerms(after, 6, 6, 0, kSlide), 2.0 / 6.0, 6, w) -
        placementScore(placementTerms(after, 6, 6, 1, kSlide), 2.0 / 6.0, 6, w);
    CHECK_EQ(game.moveOrderScore(game.placementMove(square)),
             static_cast<int>(std::lround(score * 1000.0)));
}

// ── reseedScanOrder determinism ──────────────────────────────────────────────
void testReseedScanOrder() {
    Game a;
    Game b;
    a.reseedScanOrder(42);
    b.reseedScanOrder(42);
    CHECK(a.getLegalMoves() == b.getLegalMoves());
    b.reseedScanOrder(43);
    b.reseedScanOrder(42);
    CHECK(a.getLegalMoves() == b.getLegalMoves());
}

}  // namespace

int main() {
    testEdgeWall();
    testBlock2x2();
    testGappedPair();
    testDiagonal();
    testSpearhead();
    testCorner();
    testSeam();
    testValidation();
    testGameWiring();
    testReseedScanOrder();
    std::printf("[placement_eval] %d checks, %d failures\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
// Copyright Ben Paul Wise. All Rights Reserved.
