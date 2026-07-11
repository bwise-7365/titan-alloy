// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef VIMCP_TESTSUPPORT_HPP
#define VIMCP_TESTSUPPORT_HPP

// ============================================================================
// Test-only glue between the shared SolveFn/CheckFn harness (utils.hpp) and
// GoogleTest. This header pulls in <gtest/gtest.h>, so it lives under test/ and
// is NEVER included by the vimcp library -- GoogleTest stays out of the shipped
// code. It replaces only runCase's int-return / stdout-reporting driver role;
// the SolveFn/CheckFn seam itself is unchanged and still lives in utils.hpp.
// ============================================================================

#include "utils.hpp"

#include <gtest/gtest.h>

#include <vector>

namespace VIMCP {
    // Run a bound solve from z0 and assert it neither throws nor fails a check.
    // A throw is fatal (ASSERT_NO_THROW aborts the case, since the checks below
    // would read an unset result); each CheckFn becomes a nonfatal EXPECT whose
    // failure message is that check's own one-line report. Call from inside a
    // TEST(): the GoogleTest assertion macros record against the running case.
    inline void expectSolvePasses(const SolveFn& solve, const VectorXd& z0,
                                  const std::vector<CheckFn>& checks) {
        VIResult result;
        ASSERT_NO_THROW({ result = solve(z0); });
        for (const CheckFn& check : checks) {
            const CheckResult cr = check(result);
            EXPECT_TRUE(cr.pass) << cr.report;
        }
    }
} // namespace VIMCP

#endif // VIMCP_TESTSUPPORT_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
