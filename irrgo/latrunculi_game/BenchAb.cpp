// Copyright Ben Paul Wise. All Rights Reserved.

#include "BenchAb.h"

#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <locale>
#include <sstream>
#include <stdexcept>

namespace Latrunculi::Bench {

const std::vector<WeightField>& weightFields() {
    static const std::vector<WeightField> fields = {
        {"threat", &EvalWeights::threat},
        {"pair", &EvalWeights::pair},
        {"mobility", &EvalWeights::mobility},
        {"centre", &EvalWeights::centre},
        {"vulnerableAxes", &EvalWeights::vulnerableAxes},
        {"oneMoveCapturable", &EvalWeights::oneMoveCapturable},
        {"spearheadPairs", &EvalWeights::spearheadPairs},
        {"diagonalSupport", &EvalWeights::diagonalSupport},
        {"deniedSquares", &EvalWeights::deniedSquares},
        {"strikers", &EvalWeights::strikers},
        {"notchExposure", &EvalWeights::notchExposure},
    };
    return fields;
}

void applyWeightKey(EvalWeights& weights, const std::string& field, double value) {
    for (const WeightField& f : weightFields()) {
        if (field == f.name) {
            weights.*(f.member) = value;
            return;
        }
    }
    std::ostringstream msg;
    msg << "bench: unknown weight field \"" << field << "\"; valid fields:";
    for (const WeightField& f : weightFields()) {
        msg << ' ' << f.name;
    }
    throw std::invalid_argument(msg.str());
}

std::string abHeader() {
    std::ostringstream out;
    out << std::setw(6) << "AasP" << std::setw(6) << "wSet";
    return out.str();
}

std::string abColumns(int flip, int winnerPlayer) {
    std::ostringstream out;
    out << std::setw(6) << (flip == 0 ? 0 : 1)
        << std::setw(6) << weightSetOf(winnerPlayer, flip);
    return out.str();
}

namespace {

// Win/quiet tallies over an A-vs-B run; the one derivation shared by the printed
// summary and the CSV line, so the two can never disagree.
struct AbTally {
    int games = 0;
    int winsA = 0;
    int winsAAsP0 = 0;
    int winsAAsP1 = 0;
    int quietAll = 0;
    int quietAWins = 0;
    int quietBWins = 0;
    int significanceThreshold = 0;  // wins needed for one-sided 95% significance
};

AbTally tallyAb(const std::vector<GameStats>& stats) {
    AbTally tally;
    tally.games = static_cast<int>(stats.size());
    for (std::size_t g = 0; g < stats.size(); ++g) {
        const GameStats& s = stats[g];
        const int flip = static_cast<int>(g % 2);
        const bool quiet = (s.reason == WinReason::QuietGame);
        const bool aWon = (weightSetOf(s.winner, flip) == 'A');
        if (quiet) {
            ++tally.quietAll;
        }
        if (aWon) {
            ++tally.winsA;
            if (flip == 0) {
                ++tally.winsAAsP0;
            } else {
                ++tally.winsAAsP1;
            }
            if (quiet) {
                ++tally.quietAWins;
            }
        } else if (quiet) {
            ++tally.quietBWins;
        }
    }
    // One-sided binomial against p = 0.5 at alpha = 0.05: wins >= N/2 + 1.6449*sqrt(N/4).
    tally.significanceThreshold = static_cast<int>(
        std::ceil(tally.games / 2.0 + 1.6449 * std::sqrt(tally.games / 4.0)));
    return tally;
}

std::string percentOr(int numerator, int denominator) {
    if (denominator <= 0) {
        return "-";
    }
    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << std::fixed << std::setprecision(1)
        << (100.0 * numerator / denominator) << '%';
    return out.str();
}

}  // namespace

std::string formatAbSummary(const std::vector<GameStats>& stats) {
    if (stats.empty() || stats.size() % 2 != 0) {
        throw std::invalid_argument("bench: A-vs-B summary needs a non-empty even game count");
    }
    const AbTally tally = tallyAb(stats);
    const int winsB = tally.games - tally.winsA;
    const int perColor = tally.games / 2;

    std::ostringstream out;
    out.imbue(std::locale::classic());
    out << "weight-set A wins    " << tally.winsA << " / " << tally.games
        << "  (" << percentOr(tally.winsA, tally.games) << ")"
        << "   [as P0: " << tally.winsAAsP0 << "/" << perColor
        << ", as P1: " << tally.winsAAsP1 << "/" << perColor << "]\n"
        << "weight-set B wins    " << winsB << " / " << tally.games << "\n"
        << "quiet-game share     A-wins " << percentOr(tally.quietAWins, tally.winsA)
        << "   B-wins " << percentOr(tally.quietBWins, winsB)
        << "   all " << percentOr(tally.quietAll, tally.games) << "\n"
        << "95% threshold        wins >= " << tally.significanceThreshold << " of "
        << tally.games << " (one-sided): ";
    if (tally.winsA >= tally.significanceThreshold) {
        out << "A is significantly stronger\n";
    } else if (winsB >= tally.significanceThreshold) {
        out << "B is significantly stronger\n";
    } else {
        out << "neither side significant\n";
    }
    return out.str();
}

void appendAbCsv(const std::string& path, const AbRunInfo& info,
                 const std::vector<GameStats>& stats) {
    if (stats.empty() || stats.size() % 2 != 0) {
        throw std::invalid_argument("bench: A-vs-B CSV needs a non-empty even game count");
    }
    const AbTally tally = tallyAb(stats);
    const int winsB = tally.games - tally.winsA;

    long long pliesSum = 0;
    long long capturesSum = 0;
    long long leadChangesSum = 0;
    for (const GameStats& s : stats) {
        pliesSum += s.plies;
        capturesSum += s.captures;
        leadChangesSum += s.leadChanges;
    }

    const std::filesystem::path csvPath(path);
    if (csvPath.has_parent_path()) {
        std::filesystem::create_directories(csvPath.parent_path());
    }
    const bool writeHeader = !std::filesystem::exists(csvPath);

    std::ofstream file(csvPath, std::ios::app);
    if (!file) {
        throw std::runtime_error("bench: could not open \"" + path + "\" for appending");
    }
    file.imbue(std::locale::classic());

    if (writeHeader) {
        file << "timestamp,seed,pairs,ms,rows,columns,perside,style,payoff,komi,"
                "winsA,winsB,winsA_asP0,winsA_asP1,"
                "quietPct,quietPctAwins,quietPctBwins,"
                "meanPlies,meanCaptures,meanLeadChanges,wallSecs";
        for (const WeightField& f : weightFields()) {
            file << ",wA_" << f.name;
        }
        for (const WeightField& f : weightFields()) {
            file << ",wB_" << f.name;
        }
        file << '\n';
    }

    // Timestamp as whole seconds since the Unix epoch: machine-sortable and free of
    // locale and platform time-formatting differences.
    const auto now = std::chrono::system_clock::now().time_since_epoch();
    const long long epochSeconds =
        std::chrono::duration_cast<std::chrono::seconds>(now).count();

    const double n = static_cast<double>(tally.games);
    file << epochSeconds << ',' << info.baseSeed << ',' << info.pairs << ','
         << info.msPerPly << ',' << info.rows << ',' << info.columns << ','
         << info.perSide << ','
         << (info.style == MoveStyle::Slide ? "slide" : "stepleap") << ','
         << (info.payoff == PayoffStyle::ConvexMargin ? "convex" : "gradient") << ','
         << info.komi << ','
         << tally.winsA << ',' << winsB << ','
         << tally.winsAAsP0 << ',' << tally.winsAAsP1 << ','
         << std::fixed << std::setprecision(2)
         << (100.0 * tally.quietAll / n) << ','
         << (tally.winsA > 0 ? 100.0 * tally.quietAWins / tally.winsA : 0.0) << ','
         << (winsB > 0 ? 100.0 * tally.quietBWins / winsB : 0.0) << ','
         << (pliesSum / n) << ',' << (capturesSum / n) << ','
         << (leadChangesSum / n) << ',' << info.wallSeconds;
    file << std::setprecision(6);
    for (const WeightField& f : weightFields()) {
        file << ',' << info.weightsA.*(f.member);
    }
    for (const WeightField& f : weightFields()) {
        file << ',' << info.weightsB.*(f.member);
    }
    file << '\n';
    file.close();
    if (!file) {
        throw std::runtime_error("bench: failed while writing \"" + path + "\"");
    }
}

}  // namespace Latrunculi::Bench
// Copyright Ben Paul Wise. All Rights Reserved.
