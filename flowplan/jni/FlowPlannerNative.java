// Copyright Ben Paul Wise. All Rights Reserved.
//
// Java17 binding for the C++20 flow planner.
//
// Loads flowplan_jni (flowplan_jni.dll on Windows, libflowplan_jni.so on Linux)
// and provides min-cost-flow solver.
//
public final class FlowPlannerNative {

    static {
        System.loadLibrary("flowplan_jni");
    }

    private FlowPlannerNative() {
    }

    /**
     * Solve a balanced transportation problem.
     * Inputs and outputs are arranged as 1D vectors.
     *
     * @param src  supply values, length nSrc
     * @param dst  demand values, length nDst
     * @param cost per-unit costs, length nSrc*nDst, row-major (cost[i*nDst + j])
     * @return     the optimized flow plan, length nSrc*nDst, row-major
     *             (flow[i*nDst + j]); reshape to [nSrc][nDst] if desired.
     *
     * <p>The caller MUST ensure the problem is balanced (sum(src) == sum(dst));
     * the native side does not re-check or rebalance.
     */
    public static native double[] solve(double[] src, double[] dst, double[] cost);
}
// Copyright Ben Paul Wise. All Rights Reserved.
