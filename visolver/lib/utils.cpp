// Copyright Ben Paul Wise. All Rights Reserved.
#include "utils.hpp"

#include "saoe.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <ctime>
#include <exception>
#include <stdexcept>
#include <string>
#include <vector>

namespace VINCP {
    uint64_t microsecondSeed() {
        const auto sinceEpoch = std::chrono::system_clock::now().time_since_epoch();
        const auto micros = std::chrono::duration_cast<std::chrono::microseconds>(sinceEpoch);
        return static_cast<uint64_t>(micros.count());
    }

    // My extension of RC6's function.
    // It is 1-to-1
    uint64_t qTrans(uint64_t x) {
        const uint64_t a = 3; //  must be positive, odd
        const uint64_t n = 4; //  must be positive, even
        const uint64_t c = 17; //  must be positive, odd
        const uint64_t y = (x+a)*((n*x)+c);
        return y;
    }

    uint64_t makeSeed(const uint64_t s1, const bool verbose) {
        uint64_t s2 = s1;
        bool generated = false;
        while (0 == s2) {
            s2 = qTrans(microsecondSeed());
            generated = true;
        }
        if (verbose) {
            if (generated) {
                printf("Generated PRNG seed\n");
            }
            printf("Using PRNG seed  %020llu , 0x%0llX\n",
                    s2, s2);
        }
        return qTrans(s2);
    }

    void printVector(const char* label, const VectorXd& v) {
        std::printf("%s = [", label);
        for (Index i = 0; i < v.size(); ++i) {
            std::printf(" %9.4f", v(i));
        }
        std::printf(" ]\n");
    }

    void makeComplementaryPair(Index n, std::mt19937& rng,
                               int intLo, int intHi,
                               VectorXd& w, VectorXd& z) {
        std::uniform_int_distribution<int> intDist(intLo, intHi);
        w = VectorXd::Zero(n);
        z = VectorXd::Zero(n);
        for (Index i = 0; i < n; ++i) {
            if (i % 2 == 0) {
                w(i) = static_cast<double>(intDist(rng));   // even index: w active, z = 0
            } else {
                z(i) = static_cast<double>(intDist(rng));   // odd  index: z active, w = 0
            }
        }
    }

    void printConstructed(const VectorXd& z, const VectorXd& w) {
        std::printf("\nconstructed solution (0 <= z _|_ w >= 0, w = M z + q):\n");
        printVector("  z", z);
        printVector("  w", w);
    }

    int runCase(const char* name, const SolveFn& solve, const VectorXd& z0,
                const std::vector<CheckFn>& checks) {
        std::printf("\n--- %s ---\n", name);
        const auto tStart = utcNow();
        VIResult r;
        try {
            r = solve(z0);
        } catch (const std::exception& ex) {
            utcElapsed(tStart);
            std::printf("  %s: FAIL (threw: %s)\n", name, ex.what());
            return 1;
        }
        utcElapsed(tStart);
        printSolveStats(name, r);

        bool pass = true;
        for (const CheckFn& check : checks) {
            const CheckResult cr = check(r);
            std::printf("  %s\n", cr.report.c_str());
            pass = pass && cr.pass;
        }
        std::printf("  %s: %s\n", name, pass ? "PASS" : "FAIL");
        return pass ? 0 : 1;
    }

    CheckFn checkCloseToKnown(const VectorXd& zStar, double tol) {
        return [zStar, tol](const VIResult& r) -> CheckResult {
            const double err = (r.z - zStar).norm();
            char buf[128];
            std::snprintf(buf, sizeof buf, "||z - z*|| = %.3e (tol %.1e)", err, tol);
            return CheckResult{ err < tol, std::string(buf) };
        };
    }

    CubicProblem makeCubicProblem(Index nIn, Index nOut,
                                  std::mt19937& rng,
                                  const VectorXd& xStar,
                                  const VectorXd& fStar,
                                  bool forcePSD,
                                  double aLo, double aHi) {
        if (nIn <= 0 || nOut <= 0) {
            throw std::invalid_argument("makeCubicProblem: nIn and nOut must be positive.");
        }
        if (forcePSD && nIn != nOut) {
            throw std::invalid_argument("makeCubicProblem: forcePSD requires nIn == nOut.");
        }
        if (xStar.size() != nIn) {
            throw std::invalid_argument("makeCubicProblem: xStar length must equal nIn.");
        }
        if (fStar.size() != nOut) {
            throw std::invalid_argument("makeCubicProblem: fStar length must equal nOut.");
        }

        std::uniform_real_distribution<double> aDist(aLo, aHi);
        const Index rows = forcePSD ? nIn : nOut;   // A is rows x nIn
        MatrixXd A(rows, nIn);
        for (Index r = 0; r < rows; ++r) {
            for (Index c = 0; c < nIn; ++c) {
                A(r, c) = aDist(rng);
            }
        }

        // base(x) = g(A x), or A^T g(A x) in the PSD form, with g(u) = u.^3 + u.
        const bool psd = forcePSD;
        const auto base = [A, psd](const VectorXd& x) -> VectorXd {
            const VectorXd u = A * x;
            const VectorXd gu = (u.array().cube() + u.array()).matrix();
            if (psd) {
                return A.transpose() * gu;
            }
            return gu;
        };

        const VectorXd k = fStar - base(xStar);
        const auto F = [base, k](const VectorXd& x) -> VectorXd {
            return base(x) + k;
        };

        return CubicProblem{ nIn, nOut, psd, A, k, xStar, fStar, F };
    }

    int runCubicLsqCase(const char* solverName,
                        Index nIn, Index nOut,
                        std::uint_fast32_t seed,
                        const std::function<VIResult(const VectorField&,
                                                     const VectorXd&)>& solve) {
        const int    magLo   = 1,     magHi   = 3;      // magnitude range for x* components
        const double aLo     = -0.5,  aHi     = 0.5;    // forms-matrix range
        const double startLo = -15.0, startHi = 15.0;   // random offset of the start from x*
        const double solTol  = 1.0e-6;   // ||x - x*|| bound (unique / overdetermined root)
        const double rootTol = 1.0e-6;   // ||A (x - x*)|| bound (manifold / underdetermined)

        const bool overdetermined = (nOut >= nIn);

        std::printf("\n=== %s: cubic F: R^%lld -> R^%lld  (%s) ===\n",
                    solverName,
                    static_cast<long long>(nIn), static_cast<long long>(nOut),
                    overdetermined ? "overdetermined, unique root"
                                   : "underdetermined, root manifold");

        std::mt19937 rng(seed);

        // Known root x* with mixed signs: magnitude in [magLo, magHi], sign alternating.
        std::uniform_int_distribution<int> magDist(magLo, magHi);
        VectorXd xStar(nIn);
        for (Index i = 0; i < nIn; ++i) {
            const double sign = (i % 2 == 0) ? 1.0 : -1.0;
            xStar(i) = sign * static_cast<double>(magDist(rng));
        }

        // Cubic F: R^nIn -> R^nOut with F(x*) = 0 (non-PSD / general cubic).
        const VectorXd fStar = VectorXd::Zero(nOut);
        const CubicProblem prob =
            makeCubicProblem(nIn, nOut, rng, xStar, fStar, /*forcePSD=*/false, aLo, aHi);

        // Random starting point (a random offset from x*).
        std::uniform_real_distribution<double> startDist(startLo, startHi);
        VectorXd x0(nIn);
        for (Index i = 0; i < nIn; ++i) {
            x0(i) = xStar(i) + startDist(rng);
        }

        printVector("x* (root)", xStar);
        printVector("start    ", x0);

        try {
            const VIResult r = solve(prob.F, x0);

            std::printf("\n");
            printVector("x (solved)", r.z);
            printSolveStats(solverName, r);

            // Gate on solution quality (the real correctness), not the solver's
            // self-reported 'converged' flag, since different globalizations reach
            // a given accuracy with different merit trajectories. solErr < solTol
            // already implies a small residual; manifoldErr < rootTol implies F ~ 0.
            bool pass = false;
            if (overdetermined) {
                const double solErr = (r.z - xStar).norm();
                std::printf("solution err = %.3e (||x - x*||, unique root)\n", solErr);
                pass = (solErr < solTol);
            } else {
                const double manifoldErr = (prob.A * (r.z - xStar)).norm();
                std::printf("residual^2 = %.3e, manifold err = %.3e (||A (x - x*)||, root not unique)\n",
                            r.residual, manifoldErr);
                pass = (manifoldErr < rootTol);
            }

            std::printf("%s\n", pass ? "  case PASS" : "  case FAIL");
            return pass ? 0 : 1;
        } catch (const std::exception& ex) {
            std::printf("  case FAIL: %s threw: %s\n", solverName, ex.what());
            return 1;
        }
    }
// ---------------------------------------------------------------------------
// SAOE table display (LaTeX tabular or column-aligned ASCII)
// ---------------------------------------------------------------------------

namespace {

const double kSaoeTol = 1.0e-6;   // below this an effort is treated as zero

std::string fmt(double v, int dec) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%.*f", dec, v);
    return std::string(buf);
}
std::string fmtw(double v, int width, int dec) {
    char buf[64];
    std::snprintf(buf, sizeof buf, "%*.*f", width, dec, v);
    return std::string(buf);
}
std::string latexColSpec(int nData) {   // "r r | r r ... r"
    std::string s = "r r |";
    for (int k = 0; k < nData; ++k) {
        s += " r";
    }
    return s;
}
void latexRow(const std::vector<std::string>& cells) {
    for (std::size_t k = 0; k < cells.size(); ++k) {
        std::printf("%s", cells[k].c_str());
        if (k + 1 < cells.size()) {
            std::printf(" & ");
        }
    }
    std::printf(" \\\\\n");
}
void asciiRow(const std::vector<std::string>& cells,
              const std::vector<std::size_t>& width) {
    for (std::size_t k = 0; k < cells.size(); ++k) {
        if (k > 0) {
            std::printf("  ");
        }
        std::printf("%*s", static_cast<int>(width[k]), cells[k].c_str());
    }
    std::printf("\n");
}
void asciiRule(const std::vector<std::size_t>& width) {
    std::size_t total = 0;
    for (std::size_t k = 0; k < width.size(); ++k) {
        total += width[k] + (k > 0 ? 2 : 0);
    }
    for (std::size_t k = 0; k < total; ++k) {
        std::printf("-");
    }
    std::printf("\n");
}
void renderTable(const std::vector<std::string>& header,
                 const std::vector<std::vector<std::string>>& body,
                 const std::vector<std::string>& footer, bool latex) {
    const std::size_t ncol = header.size();
    if (latex) {
        std::printf("\\begin{tabular}{%s}\n", latexColSpec(static_cast<int>(ncol) - 2).c_str());
        latexRow(header);
        std::printf("\\hline\n");
        for (const auto& row : body) {
            latexRow(row);
        }
        if (!footer.empty()) {
            std::printf("\\hline\n");
            latexRow(footer);
        }
        std::printf("\\end{tabular}\n");
    } else {
        std::vector<std::size_t> width(ncol, 0);
        const auto widen = [&](const std::vector<std::string>& r) {
            for (std::size_t k = 0; k < ncol; ++k) {
                width[k] = std::max(width[k], r[k].size());
            }
        };
        widen(header);
        for (const auto& row : body) {
            widen(row);
        }
        if (!footer.empty()) {
            widen(footer);
        }
        asciiRow(header, width);
        asciiRule(width);
        for (const auto& row : body) {
            asciiRow(row, width);
        }
        if (!footer.empty()) {
            asciiRule(width);
            asciiRow(footer, width);
        }
    }
}

} // namespace

void printComment(bool latex, const char* text) {
    std::printf("%s%s\n", latex ? "% " : "# ", text);
}

void saoePrintInputs(const MatrixXd& R, const VectorXd& S, bool latex) {
    const int M = static_cast<int>(R.rows());
    const int N = static_cast<int>(R.cols());
    printComment(latex, "SAOE inputs: col 1 = actor i, col 2 = strength S_i, cols 3.. = rewards r_{ij}");

    std::vector<std::string> header = { "", "" };
    for (int j = 0; j < N; ++j) {
        header.push_back(std::to_string(j));
    }
    std::vector<std::vector<std::string>> body;
    for (int i = 0; i < M; ++i) {
        std::vector<std::string> row = { std::to_string(i), fmt(S(i), 1) };
        for (int j = 0; j < N; ++j) {
            row.push_back(fmtw(R(i, j), 7, 1));   // rewards: %7.1f
        }
        body.push_back(row);
    }
    renderTable(header, body, {}, latex);
}

void saoePrintSolution(const MatrixXd& R, const MatrixXd& e,
                       double eps, bool latex) {
    const int M = static_cast<int>(R.rows());
    const int N = static_cast<int>(R.cols());

    std::vector<int> cols;   // options with non-zero effort somewhere
    for (int j = 0; j < N; ++j) {
        double mx = 0.0;
        for (int i = 0; i < M; ++i) {
            mx = std::max(mx, std::abs(e(i, j)));
        }
        if (mx > kSaoeTol) {
            cols.push_back(j);
        }
    }

    const VectorXd P = saoeProbabilities(e, eps);
    const VectorXd u = saoeUtilities(R, e, eps);

    printComment(latex, "SAOE solution: col 1 = actor i, col 2 = utility u_i, cols 3.. = efforts e_{ij}");
    printComment(latex, "(options with no effort omitted; last row is the option probabilities P_j)");

    std::vector<std::string> header = { "", "" };
    for (int j : cols) {
        header.push_back(std::to_string(j));
    }
    std::vector<std::vector<std::string>> body;
    for (int i = 0; i < M; ++i) {
        std::vector<std::string> row = { std::to_string(i), fmt(u(i), 3) };
        for (int j : cols) {
            row.push_back((std::abs(e(i, j)) > kSaoeTol) ? fmtw(e(i, j), 5, 1)
                                                         : std::string("-"));
        }
        body.push_back(row);
    }
    std::vector<std::string> footer = { latex ? "$P_j$" : "P_j", "" };
    for (int j : cols) {
        footer.push_back(fmt(P(j), 3));
    }
    renderTable(header, body, footer, latex);
}

// ---------------------------------------------------------------------------
// UTC wall-clock timing
// ---------------------------------------------------------------------------

namespace {

// "YYYY-MM-DD HH:MM:SS.mmm" in UTC for a system_clock time point.
std::string utcString(std::chrono::system_clock::time_point tp) {
    const std::time_t t = std::chrono::system_clock::to_time_t(tp);
    const long long ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                             tp.time_since_epoch()).count() % 1000;
    std::tm tmv{};
#if defined(_WIN32)
    gmtime_s(&tmv, &t);          // (tm*, time_t*)
#else
    gmtime_r(&t, &tmv);          // (time_t*, tm*)
#endif
    char date[32];
    std::strftime(date, sizeof date, "%Y-%m-%d %H:%M:%S", &tmv);
    char out[48];
    std::snprintf(out, sizeof out, "%s.%03lld", date, ms);
    return std::string(out);
}

} // namespace

std::chrono::system_clock::time_point utcNow() {
    const auto now = std::chrono::system_clock::now();
    std::printf("UTC: %s\n", utcString(now).c_str());
    return now;
}

void utcElapsed(std::chrono::system_clock::time_point start) {
    const auto now = std::chrono::system_clock::now();
    const long long ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count();
    std::printf("UTC: %s  elapsed %.3f s (%lld ms)\n",
                utcString(now).c_str(), static_cast<double>(ms) / 1000.0, ms);
}

ScopedUtcTimer::ScopedUtcTimer(std::string label)
    : label_(label), start_(std::chrono::system_clock::now()) {
    std::printf("%s%sSTART  UTC: %s\n",
                label_.c_str(), label_.empty() ? "" : " ", utcString(start_).c_str());
}

ScopedUtcTimer::~ScopedUtcTimer() {
    const auto now = std::chrono::system_clock::now();
    const long long ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(now - start_).count();
    std::printf("%s%sFINISH UTC: %s  elapsed %.3f s (%lld ms)\n",
                label_.c_str(), label_.empty() ? "" : " ", utcString(now).c_str(),
                static_cast<double>(ms) / 1000.0, ms);
}

void printSolveStats(const char* label, const VIResult& r) {
    if (r.innerIters > 0) {
        std::printf("%s: outer iters = %d, total inner iters = %d, residual^2 = %.3e, converged = %s\n",
                    label, r.iter, r.innerIters, r.residual, r.converged ? "true" : "false");
    } else {
        std::printf("%s: iterations = %d, residual^2 = %.3e, converged = %s\n",
                    label, r.iter, r.residual, r.converged ? "true" : "false");
    }
}

} // namespace VINCP
// Copyright Ben Paul Wise. All Rights Reserved.
