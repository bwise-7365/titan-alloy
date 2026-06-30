import java.util.Random;

import static java.lang.Math.abs;
import static java.lang.Math.min;

public class BDFP {

    public BDFP() {

    }


    // Initialize your problem by setting these five variables.
    // Nodes can have positive capacity, positive requirement,
    // or both positive.
    //
    // NOTE WELL: the sum of capacities must be greater
    // or equal to the sum of the requirements.
    // The sum of the requirements must be positive.
    //
    // If initial requirements exceed the capacities,
    // reduce them somehow BEFORE using them here.
    //
    public double[] cap; // max capacity available at a node
    public double[] rqt; // demand at a node
    public double[][] cost; // cost[i][j] = per-unit cost of moving i -> j
    public double[][] flow; // flow[i][j] = amount moved i -> j
    public int nNodes;

    // NOTE: If you find tiny amount of material being transported,
    // either ignore them (0.3 gallons out of 17.8 million is negligible)
    // or make 'minDecline' even smaller.
    public double minDecline = 1e-10; // minimum significant fractional decline

    /**
     * Initialize a RANDOM problem
     * NOTE WELL: the sum of capacities must be greater
     * or equal to the sum of the requirements.
     * (If initial requirements exceed the capacities,
     * scale them down somehow BEFORE using them)
     *
     * @param nc   number of nodes with positive capacity and zero requirements
     * @param ncr  number of nodes with positive capacity and positive requirements
     * @param nr   number of nodes with positive requirements and zero capacity
     * @param seed PRNG seed
     */
    void initRANDOM(int nc, int ncr, int nr, long seed) {
        Random rng = new Random(seed);
        nNodes = nc + ncr + nr;
        cap = new double[nNodes];
        rqt = new double[nNodes];
        cost = new double[nNodes][nNodes];
        flow = new double[nNodes][nNodes];
        double capSum = 5000001;
        double rqtSum = 5000000;
        double nodeMoveCost = 1.0; // even movement on-base has a cost
        double minCost = 1000.0;
        double maxCost = 5000.0;

        double cs = 0.0;
        double rs = 0.0;
        for (int i = 0; i < nNodes; i++) {
            if (i < nc + ncr) {
                cap[i] = 100 + rng.nextInt(200);
            } else {
                cap[i] = 0;
            }
            if (nc < i) {
                rqt[i] = 100 + rng.nextInt(200);
            } else {
                rqt[i] = 0;
            }

            cs = cs + cap[i];
            rs = rs + rqt[i];
        }

        // rescale to desired sums
        for (int i = 0; i < nNodes; i++) {
            cap[i] = (capSum / cs) * (cap[i]);
            rqt[i] = (rqtSum / rs) * (rqt[i]);
        }


        for (int i = 0; i < nNodes; i++) {
            System.out.printf("%2d  %6.2f  %6.2f\n", i, cap[i], rqt[i]);
        }
        System.out.println();

        for (int i = 0; i < nNodes; i++) {
            cost[i][i] = nodeMoveCost;
            flow[i][i] = 0;
            for (int j = i + 1; j < nNodes; j++) {
                double c = minCost + rng.nextDouble()*(maxCost-minCost);
                cost[i][j] = c + (c * rng.nextDouble() * 0.05);
                cost[j][i] = c + (c * rng.nextDouble() * 0.05);

                flow[i][j] = 0;
                flow[j][i] = 0;
            }
        }


    }


   
    /**
     * Assuming that the sum of requirements <= sum of capacities,
     * supply each demand with the nearest available capacity.
     */
    void makeGreedyFP() {
        double[] remCap = new double[nNodes];
        double[] remRqt = new double[nNodes];
        for (int i = 0; i < nNodes; i++) {
            remCap[i] = cap[i];
            remRqt[i] = rqt[i];
        }


        double rMax = +1.0;
        int iter = 0;
        int maxIter = 50 * nNodes;
        while (0 < rMax) {
            int rNdx = 0; // index of largest remaining demand
            rMax = -1.0;
            for (int i = 0; i < nNodes; i++) {
                if (rMax < remRqt[i]) {
                    rMax = remRqt[i];
                    rNdx = i;
                }
            }

            int cNdx = -1; // index of cheapest potential supplier
            double cMin = Double.MAX_VALUE;
            for (int i = 0; i < nNodes; i++) {
                if ((0 < remCap[i]) && (cost[i][rNdx] < cMin)) {
                    cNdx = i;
                    cMin = cost[cNdx][rNdx];
                }
            }
            if (cNdx != -1) {
                double q = min(remRqt[rNdx], remCap[cNdx]);
                System.out.printf("supplying %6.2f from %2d to %2d\n", q, cNdx, rNdx);
                remRqt[rNdx] = remRqt[rNdx] - q;
                remCap[cNdx] = remCap[cNdx] - q;
                flow[cNdx][rNdx] = flow[cNdx][rNdx] + q;
            } else {
                throw new RuntimeException("Could not find any remaining capacity");
            }
            iter++;
            if (iter > maxIter) {
                throw new RuntimeException("Exceeded max iterations");
            }
        }


    }


    void runSwap(boolean verbose) {
        double pc1 = planCost();

        int iter = 0;
        int maxIter = 500;

        double decline = -1.0;
        while ((decline < 0.0) && (iter < maxIter)) {
            decline = oneSwap();
            iter++;
            if (verbose) {
                System.out.printf("%3d/%3d swap  decline: %14.4f\n",
                                  iter, maxIter, decline);
                System.out.flush();
            }
        }

        if (verbose) {
            System.out.printf("Initial cost: %14.4f\n", pc1);
            double pc2 = planCost();
            System.out.printf("Final cost:   %14.4f\n", pc2);
            double pct = 100.0 * (pc1 - pc2) / pc1;
            System.out.printf("Percent reduction: %5.2f\n", pct);
            System.out.printf("Factor reduction:  %5.2f\n", pc1 / pc2);
            showPlanCompact();
        }

    }

    double oneSwap() {

        double fc0 = planCost();
        double bestDecline = -minDecline * fc0;
        boolean realDecline = false;

        for (int i = 0; i < nNodes; i++) {
            for (int j = 0; j < nNodes; j++) {
                for (int m = 0; m < nNodes; m++) {
                    for (int n = 0; n < nNodes; n++) {
                        double q = min(flow[i][n], flow[m][j]);
                        if (0.0 < q) { // there is something to be swapped
                            double disc = (cost[i][j] + cost[m][n]) - (cost[i][n] + cost[m][j]);
                            if (disc < 0.0) { // there is a marginal reduction in cost
                                double decline = disc * q;
                                if (decline < bestDecline) {
                                    bestDecline = decline;
                                    realDecline = true;
                                    flow[i][j] = flow[i][j] + q;
                                    flow[i][n] = flow[i][n] - q;
                                    flow[m][n] = flow[m][n] + q;
                                    flow[m][j] = flow[m][j] - q;
                                }
                            }
                        }
                    }
                }
            }
        }

        checkPlan(); // this was for verification during development. Use it if you change things.

        double actualDecline = 0.0;
        if (realDecline) {
            actualDecline = bestDecline;
        }
        return actualDecline;
    }


    // Calculate cost of a flow plan
    double planCost() {
        double pc = 0.0;
        for (int i = 0; i < nNodes; i++) {
            for (int j = 0; j < nNodes; j++) {
                pc = pc + (cost[i][j] * flow[i][j]);
            }
        }
        return pc;
    }

    boolean checkPlan() {
        boolean ok = true;
        double dTol = 1E-4;

        for (int j = 0; j < nNodes; j++) {
            double lhs = cap[j];
            double rhs = rqt[j];
            for (int i = 0; i < nNodes; i++) {
                assert (0.0 <= flow[i][j]);
                assert (0.0 <= flow[j][i]);
                lhs = lhs + flow[i][j];
                rhs = rhs + flow[j][i];
            }
            if (rhs > lhs + dTol) { // watch out for round-off
                ok = false;
            }
        }


        for (int i = 0; i < nNodes; i++) {
            double inFlow = 0.0;
            double outFlow = 0.0;
            for (int j = 0; j < nNodes; j++) {
                inFlow = inFlow + flow[j][i];
                outFlow = outFlow + flow[i][j];
            }
            double ri = rqt[i];
            double ci = cap[i];
            if (dTol < abs(inFlow - ri)) { // watch out for round-off
                ok = false;
            }
            if (outFlow > ci + dTol) { // watch out for round-off
                ok = false;
            }

        }
        return ok;
    }

    // This dumps the plan to the terminal.
    //
    // DO NOT USE THIS IN PRODUCTION. It is only for debugging.
    //
    // Save flow[][] to a file, use it in memory, etc.
    void showPlanVerbose() {
        double pc = planCost();
        System.out.printf("Plan cost: %12.2f\n", pc);
        System.out.printf("    ");
        for (int j = 0; j < nNodes; j++) {
            System.out.printf("     %3d", j);
        }
        System.out.println();
        for (int i = 0; i < nNodes; i++) {
            System.out.printf("%3d  ", i);
            for (int j = 0; j < nNodes; j++) {
                System.out.printf(" %6.1f ", flow[i][j]);
            }
            System.out.println();
        }
        System.out.flush();
    }

    // This dumps the plan to the terminal.
    //
    // DO NOT USE THIS IN PRODUCTION. It is only for debugging.
    //
    // Save flow[][] to a file, use it in memory, etc.
    void showPlanCompact() {
        double pc = planCost();
        System.out.printf("Plan cost: %12.2f\n", pc);
        for (int i = 0; i < nNodes; i++) {
            boolean anyOut = false;
            for (int j = 0; (!anyOut) && (j < nNodes); j++) {
                if (0 < flow[i][j]) {
                    anyOut = true;
                }
            }
            if (anyOut) {
                System.out.printf("%3d ==> ", i);
                for (int j = 0; j < nNodes; j++) {
                    if (0 < flow[i][j]) {
                        System.out.printf("(%3d, %6.1f) ", j, flow[i][j]);
                    }
                }
                System.out.println();
            }
        }
        System.out.flush();
    }


}

// end of file

