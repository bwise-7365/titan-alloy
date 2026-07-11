// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VIMCP_MCPENGINES_HPP
#define VIMCP_MCPENGINES_HPP

// ============================================================================
// Test-only harness that binds ONE mixed-NCP VIModel to several solver engines
// ("rows"), so a translated problem drops into the suite as data + a VIModel
// builder rather than as copied solver glue. Built for the translated
// GMS-corpus models (gms_alloceff_test.cpp, gms_deploy_test.cpp), whose problems are
// NONMONOTONE: any single engine may legitimately fail to converge or trip a
// divergence guard, so every row runs through runCase (which times the solve,
// prints the standard stats, and reports a throw as that row's failure without
// aborting the case) and the caller decides how many converged rows constitute
// a pass. Adding an engine to a test is one entry in its engine list; adding a
// new engine kind is one case in makeMcpEngineRows.
// ============================================================================

#include "josephynewton.hpp"
#include "semismoothnewton.hpp"
#include "utils.hpp"
#include "vimcp.hpp"

#include <cstdio>

using std::string;
using std::vector;

namespace VIMCP {

    // Engines that can drive a mixed-NCP VIModel end to end.
    enum class McpEngine {
        Ssn,    // semismoothNewtonSolve, DIRECT on the nonlinear model
        JnHan,  // Josephy-Newton outer, dHan06 inner (refactors every inner iter)
        JnHe,   // Josephy-Newton outer, bsHe94b inner (factors once per outer step)
        JnIpm,  // Josephy-Newton outer, mehrotraIpm inner (numFree = model.n;
                //   the inner linearization must be monotone for it to be reliable)
    };

    // Per-problem controls shared by every row. All tolerances are SQUARED
    // Euclidean norms, per the library convention.
    struct McpEngineParams {
        double magTol       = 1.0e-8;   // stop for ssn and the JN outer loop
        int    ssnIterMax   = 200;      // semismooth Newton iteration cap
        int    outerIterMax = 100;      // JN outer iteration cap
        double innerMagTol  = 1.0e-10;  // JN inner (affine-VI) tolerance
        int    innerIterMax = 20000;    // JN inner cap for the projection engines
        int    ipmIterMax   = 200;      // JN inner cap for mehrotraIpm (counts LUs)
        int    iterFreq     = 0;        // heartbeat: log every N ssn / JN-outer
                                        //   iterations, flushed (0 = silent).
                                        //   Turn on for long runs -- stdout is
                                        //   fully buffered under a piped test
                                        //   runner, so a killed silent run
                                        //   loses even runCase's output.
        int    jnStallIterMax = 0;      // JN no-progress cutoff (consecutive
                                        //   stalled outer iterations before an
                                        //   honest converged=false; 0 = off).
    };

    // One named, fully bound solver row.
    struct McpEngineRow {
        string  name;
        SolveFn solve;
    };

    inline const char*
    mcpEngineName(McpEngine engine)
    {
        switch (engine) {
            case McpEngine::Ssn:   return "ssn";
            case McpEngine::JnHan: return "jn+dhan06";
            case McpEngine::JnHe:  return "jn+bshe94b";
            case McpEngine::JnIpm: return "jn+ipm";
        }
        return "unknown";
    }

    // Immediate-flush heartbeat logger, so progress survives a killed run.
    // IterationLogger and OuterLogger share the signature, so this serves both
    // the ssn iteration hook and the JN outer hook.
    inline IterationLogger
    heartbeatLogger(const string& label)
    {
        return [label](int iter, int iterMax, double mag, double magTol) {
            std::printf("    %s iter %d/%d: residual^2 %.3e (tol %.1e)\n",
                        label.c_str(), iter, iterMax, mag, magTol);
            std::fflush(stdout);
            return;
        };
    }

    // One fully bound ssn row from caller-supplied solver params: the hook for
    // VARIANT rows (nonmonotone memory, alternative NCP functions, restart
    // lambdas) beyond the default McpEngine::Ssn entry. Captures by value.
    inline McpEngineRow
    makeSsnRow(const VIModel& model, const string& name,
               const SemismoothNewtonParams& ssnParams, int iterFreq = 0)
    {
        const IterationLogger logger =
            (0 < iterFreq) ? heartbeatLogger(name) : IterationLogger{};
        return McpEngineRow{ name,
            [model, ssnParams, logger](const VectorXd& z0) {
                return semismoothNewtonSolve(model, z0, ssnParams, logger);
            } };
    }

    // Bind one row per requested engine. The VIModel and params are captured BY
    // VALUE in each row's SolveFn, so the rows may outlive the caller's locals.
    inline vector<McpEngineRow>
    makeMcpEngineRows(const VIModel& model,
                      const vector<McpEngine>& engines,
                      const McpEngineParams& params = McpEngineParams{})
    {
        JosephyNewtonParams jnParams;
        jnParams.outerTol      = params.magTol;
        jnParams.outerIterMax  = params.outerIterMax;
        jnParams.outerIterFreq = params.iterFreq;
        jnParams.stallIterMax  = params.jnStallIterMax;
        const auto bindJn = [&](const char* name, const InnerSolver& inner) -> SolveFn {
            const OuterLogger logger =
                (0 < params.iterFreq) ? heartbeatLogger(name) : OuterLogger{};
            return [model, inner, jnParams, logger](const VectorXd& z0) {
                return solveVI(model, z0, inner, jnParams, logger);
            };
        };

        vector<McpEngineRow> rows;
        for (const McpEngine engine : engines) {
            SolveFn solve;
            switch (engine) {
                case McpEngine::Ssn: {
                    SemismoothNewtonParams ssnParams;
                    ssnParams.magTol   = params.magTol;
                    ssnParams.iterMax  = params.ssnIterMax;
                    ssnParams.iterFreq = params.iterFreq;
                    solve = makeSsnRow(model, mcpEngineName(engine), ssnParams,
                                       params.iterFreq).solve;
                    break;
                }
                case McpEngine::JnHan: {
                    solve = bindJn("jn+dhan06",
                                   makeDHan06Solver(params.innerMagTol,
                                                    params.innerIterMax, 0));
                    break;
                }
                case McpEngine::JnHe: {
                    solve = bindJn("jn+bshe94b",
                                   makeBsHe94bSolver(params.innerMagTol,
                                                     params.innerIterMax, 0));
                    break;
                }
                case McpEngine::JnIpm: {
                    solve = bindJn("jn+ipm",
                                   makeMehrotraIpmSolver(model.n, params.innerMagTol,
                                                         params.ipmIterMax, 0));
                    break;
                }
            }
            rows.push_back(McpEngineRow{ mcpEngineName(engine), solve });
        }
        return rows;
    }

    // The convergence gate for "solves it at all": the solver's own converged
    // flag (residual under tolerance within the cap). Deliberately says nothing
    // about WHICH solution was reached -- these models have multiple equilibria;
    // reference-solution checks come later, as extra CheckFns.
    inline CheckFn
    checkConvergedFlag()
    {
        return [](const VIResult& r) -> CheckResult {
            char buf[96];
            std::snprintf(buf, sizeof buf, "converged = %s (squared residual %.3e)",
                          r.converged ? "true" : "false", r.residual);
            return CheckResult{ r.converged, string(buf) };
        };
    }

    // Run every row from the same z0 and return how many converged. Each row
    // must pass 'extraChecks' AND the converged flag to count; report-only
    // extras (always-pass CheckFns that print a solution summary) are the
    // intended use. runCase prints per-row timing/stats and absorbs throws.
    inline int
    countConvergedRows(const vector<McpEngineRow>& rows, const VectorXd& z0,
                       const vector<CheckFn>& extraChecks = {})
    {
        int converged = 0;
        for (const McpEngineRow& row : rows) {
            vector<CheckFn> checks = extraChecks;
            checks.push_back(checkConvergedFlag());
            if (0 == runCase(row.name.c_str(), row.solve, z0, checks)) {
                ++converged;
            }
            std::fflush(stdout);   // keep completed rows visible if a later
                                   //   row is killed mid-run
        }
        return converged;
    }

} // namespace VIMCP

#endif // VIMCP_MCPENGINES_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
