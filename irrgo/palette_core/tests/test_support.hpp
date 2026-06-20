// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PALETTE_TEST_SUPPORT_HPP
#define PALETTE_TEST_SUPPORT_HPP

// Minimal, dependency-free test harness. No external framework is pulled in so
// palette_core configures and builds offline on both Windows (MSVC) and Debian.
// Each test executable owns its own counters and returns non-zero on failure so
// CTest reports pass/fail.

#include <cmath>
#include <cstdio>
#include <functional>

namespace palette_test {

inline int g_checks = 0;
inline int g_failures = 0;

inline void record(bool ok, const char* expr, const char* file, int line) {
    ++g_checks;
    if (!ok) {
        ++g_failures;
        std::printf("  FAIL [%s:%d] %s\n", file, line, expr);
    }
}

// Runs `fn`, treating an escaping exception as a failed expectation.
inline void expectThrows(const std::function<void()>& fn, const char* what,
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

inline int summarize(const char* suite) {
    std::printf("[%s] %d checks, %d failures\n", suite, g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}

} // namespace palette_test

#define CHECK(cond) ::palette_test::record((cond), #cond, __FILE__, __LINE__)

#define CHECK_NEAR(a, b, tol) \
    ::palette_test::record(std::fabs((a) - (b)) <= (tol), \
                           #a " ~= " #b " (tol " #tol ")", __FILE__, __LINE__)

#define CHECK_THROWS(stmt) \
    ::palette_test::expectThrows([&]() { stmt; }, #stmt, __FILE__, __LINE__)

#endif // PALETTE_TEST_SUPPORT_HPP
// Copyright Ben Paul Wise. All Rights Reserved.
