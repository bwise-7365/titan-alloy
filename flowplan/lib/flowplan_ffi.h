// Copyright Ben Paul Wise. All Rights Reserved.
//
// Pure C-ABI entry point for the flow planner, intended to be called across a
// foreign-function-interface boundary (e.g. JNI). It speaks only in flat
// double buffers and int lengths -- no C++ or JNI types -- so it is trivially
// bindable and unit-testable without a JVM.
//
#ifndef FLOWPLAN_FFI_H
#define FLOWPLAN_FFI_H

#include <cstdint>

#ifdef __cplusplus
extern "C" {
#endif

// Solve a transportation / min-cost-flow problem.
//
//   src      : n_src supply values
//   dst      : n_dst demand values
//   cost     : n_src * n_dst per-unit costs, row-major (cost[i*n_dst + j])
//   flow_out : caller-allocated n_src * n_dst buffer, row-major; receives the
//              optimized flow plan (flow_out[i*n_dst + j]).
//
// Preconditions (NOT checked here -- the caller must guarantee them):
//   - sum(src) == sum(dst) (the problem must be balanced).
// Values are used as-is; no truncation or rebalancing is performed.
//
// Returns 0 on success, or a negative status code on invalid dimensions /
// null pointers.
int32_t flowplan_solve(const double *src,  int32_t n_src,
                       const double *dst,  int32_t n_dst,
                       const double *cost,
                       double       *flow_out);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // FLOWPLAN_FFI_H
// Copyright Ben Paul Wise. All Rights Reserved.
