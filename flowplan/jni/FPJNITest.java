// Copyright Ben Paul Wise. All Rights Reserved.
//
// Demo driver for FlowPlannerNative (the JNI binding around the
// native flow planner). Builds a couple of floww problems,
// solves them using C++ library, prints the resulting flow plans.
//
// Run from the directory holding FlowPlannerNative.class / FPJNITest.class with
// the native flowplan_jni library on java.library.path, e.g.:
//   java -Djava.library.path=../cmake-build-debug FPJNITest
//
import java.util.Arrays;
import java.lang.Math;

public final class FPJNITest {

    // Fractional tolerance for floating-point comparisons.
    private static final double EPS = 1.0e-6;

    public static void main(String[] args) {

        // generate random problems and solve them
        runGeneratedCase("FlowPlan problem 8x5", 8, 5, 123456);
        //runGeneratedCase("FlowPlan problem 20x10", 20, 10, 654321);

    }

    // Build a problem (fixed seed -> reproducible), then solve and check it.
    private static void runGeneratedCase(String name, int nSrc, int nDst, long seed) {


        // -------------------------------------------------------------
        // NOTE: this section generates random test data: src, dst, cost
        // -------------------------------------------------------------

        java.util.Random rng = new java.util.Random(seed);

        double[] src = new double[nSrc];
        double[] dst = new double[nDst];
        double[] cost = new double[nSrc * nDst];

        double srcSum = 0, dstSum = 0;
        for (int i = 0; i < nSrc; i++) {
            src[i] = 100 + rng.nextInt(200);
            srcSum += src[i];
        }
        for (int j = 0; j < nDst; j++) {
            dst[j] = 100 + rng.nextInt(200);
            dstSum += dst[j];
        }

        // Rescale demand so sum(dst) == sum(src)
        double scale = srcSum / dstSum;
        dstSum = 0;
        for (int j = 0; j < nDst; j++) {
            dst[j] *= scale;
            dstSum += dst[j];
        }
        dst[nDst - 1] += srcSum - dstSum; // round-off error

        for (int k = 0; k < cost.length; k++) {
            cost[k] = 10 + rng.nextInt(50);
        }


        // -------------------------------------------------------------
        // NOTE: this section checks the data and solves (if OK)
        // -------------------------------------------------------------
        double[] flow = null;
        boolean OK = checkCase(name, src, dst, cost);
        if (OK) {
            flow = runCase(name, src, dst, cost);
        }


        // -------------------------------------------------------------
        // NOTE: a section should go here to use the 'flow' results
        // -------------------------------------------------------------

        // For now, we just print the flow plan as a matrix
        // and show what cost was achieved.
        // NOTE the order in which 'flow' is unpacked.
        System.out.println("Flow plan received in Java:");
        for (int i = 0; i < nSrc; i++) {
            StringBuilder row = new StringBuilder("  ");
            for (int j = 0; j < nDst; j++) {
                row.append(String.format("%8.2f", flow[i * nDst + j]));
            }
            System.out.println(row);
        }

        double total = 0;
        for (int k = 0; k < flow.length; k++) {
            total += flow[k] * cost[k];
        }
        System.out.printf("total cost: %.4f%n", total);

    }


    /**
     * Check that the problem meets basic requirements.
     *
     * 1. sum(src) == sum(dst), which might be relaxed to d <= s later.
     * 2. non-negative costs
     * 3. positive cost sum
     * 3. positive source and delivery
     * 4. dimensions are positive
     * 5. dimensions are not inconsistent
     *
     * @param name Descriptive name of the problem
     * @param src 1D array of source quantities
     * @param dst 1D array of delivery quantities
     * @param cost 1D array of per-unit costs, encoding 2D flow costs
     * @return  TRUE if the problem is OK, FALSE otherwise
     */
    private static boolean checkCase(String name, double[] src, double[] dst, double[] cost) {
        boolean OK = true;
        final double EPS = 1.0e-6;
        double sSum = 0;
        double dSum = 0;
        final int nSrc = src.length;
        final int nDst = dst.length;
        final int nCost = cost.length;

        System.out.println("=== " + name + " (" + nSrc + " x " + nDst + ") ===");
        String msg = "";

        if (0 == nSrc) {
            msg += "Empty source \n";
            OK = false;
        }
        if (0 == nDst) {
            msg += "Empty delivery \n";
            OK = false;
        }
        if (nCost != nSrc*nDst) {
            msg += "Cost length (" + nCost + ") not product of source and delivery lengths (" +nSrc+", "+ nDst + ") \n";
        }

        for (int i = 0; i < nSrc; i++) {
            sSum += src[i];
            if (src[i] <= 0) {
                msg += "Non-positive source: src[" + i + "] = "+src[i]+" <= 0 \n";
                OK = false;
            }
        }
        for (int j = 0; j < nDst; j++) {
            dSum += dst[j];
            if (dst[j] <= 0) {
                msg += "Non-positive delivery: dst[" + j + "] = "+dst[j]+" <= 0 \n";
            }
        }

        if (sSum <= 0) {
            msg += "Non-positive source sum: sSum = "+sSum+" <= 0 \n";
            OK = false;
        }
        if (dSum <= 0) {
            msg += "Non-positive delivery sum: dSum = "+dSum+" <= 0 \n";
            OK = false;
        }

        if (OK) { // avoid division by zero
            double symDiff = (2.0 * Math.abs(sSum - dSum)) / (sSum + dSum);
            if (symDiff > EPS) {
                msg += "Mismatched sums: sum(src) = " + sSum + " != sum(dst) = " + dSum + "\n";
                OK = false;
            }
        }

        double costSum = 0;
        for (int k = 0; k < cost.length; k++) {
            costSum += cost[k];
            if (cost[k] < 0) {
                msg += "Non-positive cost: cost[" + k + "] = "+cost[k]+" <= 0 \n";
                OK = false;
            }
        }
        if (costSum <= 0) {
            msg += "Non-positive cost sum: costSum = "+costSum+" <= 0 \n";
            OK = false;
        }

        if (!OK) {
            System.out.printf("Problem %s was not OK: \n", msg);
        }
        else {
            System.out.printf("Problem %s was OK\n", name);
        }

        return OK;
    }


    /**
     * Solve the known-feasible problem, return the flow plan.
     *
     * @param name Descriptive name of the problem
     * @param src 1D array of source quantities
     * @param dst 1D array of delivery quantities
     * @param cost 1D array of per-unit costs, encoding 2D flow costs
     * @return   1D array that encodes 2D flow plan
     *
     */
    private static double[]  runCase(String name, double[] src, double[] dst, double[] cost) {
        final int nSrc = src.length;
        final int nDst = dst.length;

        System.out.println("Attempting to solve " + name + " (" + nSrc + " x " + nDst+")");

        // This is where JNI to C++ is used
        double[] flow = FlowPlannerNative.solve(src, dst, cost);

        // check basic error conditions
        if (flow == null) {
            System.out.printf("solve returned empty array");
            return null;
        }
        else if (flow.length != nSrc * nDst) {
            System.out.printf("solve returned an array of the wrong shape: "+flow.length);
            return null;
        }

        // TODO: check for negative values (should be impossible)


        return flow;
    }
}
// Copyright Ben Paul Wise. All Rights Reserved.
