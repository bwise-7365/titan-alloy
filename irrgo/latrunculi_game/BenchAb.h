// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "EvalWeights.h"
#include "Game.h"
#include "GameStats.h"

#include <cstdint>
#include <string>
#include <vector>

// A-vs-B support for latrunculi_bench: two evaluation weight sets fight over mirrored
// pairs of games, so a candidate set's playing strength is measured against an
// incumbent instead of asserted. Bench-only code -- it is linked into the bench
// executable, not into latrunculi_lib -- kept out of bench.cpp so that file stays a
// driver rather than growing a reporting module.
//
// Conventions, shared with bench.cpp:
//   - N games = 2 * pairs, indexed g in [0, N). pair = g/2, flip = g%2.
//   - Both games of a pair use the same seed (base + pair), so they differ only in
//     which set holds which color.
//   - flip == 0: set A is player 0.  flip == 1: set A is player 1.
namespace Latrunculi::Bench {

// One row per tunable weight: its CLI/CSV name and where it lives in EvalWeights. The
// single source for applyWeightKey and the CSV writer, so a new weight added to
// EvalWeights fails compilation here rather than silently missing from the tools.
struct WeightField {
    const char* name;
    double EvalWeights::*member;
};

// All EvalWeights fields, in a fixed order (the CSV column order).
const std::vector<WeightField>& weightFields();

// Set the named field of `weights` to `value`. `field` is the bare name ("threat",
// "spearheadPairs", ...) without the wA./wB. prefix. Throws std::invalid_argument on an
// unknown name -- a typo must not silently sweep a default instead of the candidate.
void applyWeightKey(EvalWeights& weights, const std::string& field, double value);

// Which weight set holds player `player` in game `g` of an A-vs-B run.
inline char weightSetOf(int player, int flip) {
    return (player == (flip == 0 ? 0 : 1)) ? 'A' : 'B';
}

// Per-game columns appended to the stats row in A-vs-B mode: which color set A held
// and which SET won (the stats row's own `win` column keeps meaning player 0/1).
std::string abHeader();
std::string abColumns(int flip, int winnerPlayer);

// Aggregate A-vs-B summary over the whole run: set wins with per-color breakdown, the
// quiet-game share of each set's wins (a set that wins by stalling shows itself here),
// and the one-sided 95% significance threshold so nobody recomputes the binomial by
// hand. `stats` is indexed by game g; flip is derived as g%2.
std::string formatAbSummary(const std::vector<GameStats>& stats);

// Everything one CSV line needs beyond the per-game stats. The full weight vectors are
// included even though the sweep driver knows what it passed: a self-describing CSV
// survives being found months later.
struct AbRunInfo {
    std::uint64_t baseSeed = 0;
    int pairs = 0;
    int msPerPly = 0;
    int rows = 0;
    int columns = 0;
    int perSide = 0;
    MoveStyle style = MoveStyle::Slide;
    PayoffStyle payoff = PayoffStyle::ConvexMargin;
    double komi = 0.0;
    double wallSeconds = 0.0;
    EvalWeights weightsA{};
    EvalWeights weightsB{};
};

// Append one machine-readable summary line to `path`, writing the header first when the
// file does not yet exist. Any I/O failure throws -- a sweep that silently dropped its
// result line would corrupt the comparison it exists for.
void appendAbCsv(const std::string& path, const AbRunInfo& info,
                 const std::vector<GameStats>& stats);

}  // namespace Latrunculi::Bench
// Copyright Ben Paul Wise. All Rights Reserved.
