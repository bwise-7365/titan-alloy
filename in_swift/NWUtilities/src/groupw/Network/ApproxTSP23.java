/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package groupw.Network;

import groupw.Network.NWUtils.PointCoords;
import java.util.List;

/**
 *
 * This class uses the 2-opt and 3-opt heuristics
 *
 * @author BenWise
 *
 */
public class ApproxTSP23 extends ApproxTSP {

    public ApproxTSP23() { super(); }

    public ApproxTSP23(ApproxTSP a) {
        super(a);
    }

    public ApproxTSP23(List<Integer> ps) { super(ps); }

    /**
     * Build an efficient traveling salesman tour using the provided symmetric
     * matrix of positive costs. It starts with a complete but random tour then
     * applies the 2-opt and 3-opt heuristics until no more improvement is possible.
     * The tour is not guaranteed to be optimal, as
     * absolute optimality is very difficult. The maxIter parameter should be
     * odd, like 1 or 3, so it ends on improve2EH that eliminates obvious
     * crossings. For most purposes, maxIter = 3 is best.
     *
     * @param et0 initial tour, which we desire to improve
     * @param cMat symmetric cost-matrix of positive entries.
     * @param maxIter maximum number of improvement-sweeps.
     * @return an efficient traveling salesman tour through the points
     */
    static public ImprvTour improve23(ApproxTSP23 et0, PointCoords pcs, // double[][] cMat,
                                      boolean verbose, int maxIter) {
        ApproxTSP23 etA = et0;
        //etA.distMatrix = et0.distMatrix;
        etA.points = et0.points;
        etA.numPoints = et0.numPoints;

        int nPnts = etA.numPoints - 1;
        if (verbose && (nPnts <= 100)) {
            for (int i = 0; i < nPnts; i++) {
                for (int j = 0; j < nPnts; j++) {
                    System.out.printf(" %7.2f",   pcs.cost(i, j));
                }
                System.out.println();
                System.out.flush();
            }
            System.out.println();
        }
        boolean successP = true;
        int iter = 0;
        if (maxIter < 1) { // always at least one iteration
            maxIter = 1;
        }
        if (0 == (maxIter % 2)) { // always odd, so it will end on improve2EH
            maxIter = maxIter + 1;
        }
        while ((iter < maxIter) && (successP)) {
            //System.gc(); // force garbage collection
            ImprvTour ir = new ImprvTour(false, 0, 0, et0); // avoid compiler warning
            successP = false;
            if (0 == (iter % 2)) {
                if (verbose) {
                    System.out.printf("Outer iteration %3d trying 2EH ... ", iter);
                }
                ir = improve2EH(etA, pcs);
            } else {
                if (verbose) {
                    System.out.printf("Outer iteration %3d trying 3EH ... ", iter);
                }
                ir = improve3EH(etA, pcs);
            }
            boolean s2 = ir.improved;
            int n2 = ir.numImproved;
            int innerIter = ir.numIter;
            double searchFrac = ((double)innerIter)/(((double) etA.numPoints) * ((double) etA.numPoints));
            ApproxTSP23 etB = (ApproxTSP23) ir.newTour;
            if (s2) {
                double c1 = etA.costETour(pcs);
                double c2 = etB.costETour(pcs);
                double absChange = c1 - c2;
                double pctChange = 100.0 * (c1 - c2) / c1;
                if (verbose) {
                    System.out.flush();
                    System.out.printf("in %8d iterations (%.4f) with %7d changes, reduced cost %11.3f to %11.3f , abs: %11.3f , pct: %8.4f \n",
                            innerIter, searchFrac, n2, c1, c2, absChange, pctChange);
                    System.out.flush();
                }
                successP = true;
                etA = etB;
            } else {
                if (verbose) {
                    System.out.printf("in %8d iterations with %d changes, no reduction. \n",
                            innerIter, n2);
                }
                assert (0 == n2);
            }
            iter++;
        }
        if (verbose) {
            System.out.println();
        }
        ImprvTour rslt = new ImprvTour(successP, 0, iter, etA); // always zero improvement, because that's how we knew to stop.
        rslt.newCost = etA.costETour(pcs);
        return rslt;
    }


    //--------------------------------------------------------------------------
    /**
     * Repeatedly scan this tour trying to improve it with two-edge splices.
     *
     * It always compares the first edge to other edges, so the tour gets
     * rotated after each improvement. This continues until no further two-edge
     * improvements are found.
     *
     * @param et0 Extended ApproxTSP to be improved
     * @param pcs PointCoords to calculate costs between points
     *
     * @return
     */
    static protected ImprvTour improve2EH(ApproxTSP23 et0, PointCoords pcs) {
        assert (et0.eTourP());
        int m = et0.numPoints;
        ApproxTSP23 etA = et0;
        boolean anyImprovement = false;
        int numImprovements = 0;
        int numUnimproved = 0;
        int iter = 0;
        int maxIter = (m<220) ? 1000 :  100*m; // arbitrary limit for testing which is never reached.
        while ((iter < maxIter) && (numUnimproved < m + 1)) { // if we've tried every edge at the front without improvement, stop.
            //System.out.printf("Iteration improve2EH %6d: ", iter);
            IndexImprv bibs = scanBest2EH(etA, pcs);
            int bestIndx = bibs.index;
            double bestSvng = bibs.improvement;
            if (bestIndx < 0) { // no improvement found
                numUnimproved++;
                etA = new ApproxTSP23(rotateETourLeft(etA));
                //System.out.printf("number of unimproved: %5d \n", numUnimproved);
            } else {
                etA = splice2(etA, bestIndx);
                double c = etA.costETour(pcs);
                numUnimproved = 0;
                anyImprovement = true;
                numImprovements++;
                //System.out.printf("found improvement %4d with new cost %10.3f after savings %10.3f \n", bestIndx, dMax, bestSvng);
            }
            iter++;
        }

        etA = new ApproxTSP23(rotateToFront(0, etA));

        ImprvTour rslt = new ImprvTour(anyImprovement, numImprovements, iter, etA);
        rslt.newCost = etA.costETour(pcs);
        return rslt;
    }

    /**
     * Compare each edge to the [0,1] edge to see if a two-edge splice would
     * help. A positive savings is helpful; a negative savings is harmful.
     *
     * @param et0 The extended tour to be analyzed.
     * @param pcs PointCoords to calculate costs between points
     *
     * @return IndexImprv object, i.e. [bestNdx, bestSvng]
     */
    static protected IndexImprv scanBest2EH(ApproxTSP23 et0, PointCoords pcs) {
        assert (et0.eTourP());
        int m = et0.numPoints;
        double bestSvng = 0.0;
        int bestIndx = -1; // negative one means "none found yet"
        for (int i = 0; i <= m + 1; i++) {
            if (validSplice2(et0, i)) {
                double si = savings2(et0, i, pcs);
                if (bestSvng < si) {
                    bestIndx = i;
                    bestSvng = si;
                }
            }
        }
        IndexImprv rslt = new IndexImprv(bestIndx, bestSvng);
        return rslt;
    }

    static protected boolean validSplice2(ApproxTSP23 et1, int k) {
        // 'm' is the length of the extended tour, 'n' is the basic tour.
        // the positions are [P0, P1, P2, ... Pn-2, Pn-1,   P0]
        // Because [offSet, offSet+1] cannot include [0,1] at either end,
        // we need 2 <= offSet and offSet+1 <= n-1.
        // In terms of 'm',  [P0, P1, P2, ... Pm-3, Pm-2, Pm-1]
        // i.e. offSet+1 <= m-2, so 2 <= offSet <= m-3
        assert (et1.eTourP());
        int m = et1.numPoints; // including duplicate at the end
        boolean ok = ((2 <= k) && (k <= m - 3));
        return ok;
    }

    /**
     * Savings if edge [0,1] is uncrossed with edge [sk, sk+1]
     *
     * @param et0 extended ApproxTSP which might be spliced
     * @param sk location of splice
     * @param pcs PointCoords to calculate costs between points
     *
     * @return Reduction in tour-length if they swap (negative means increase)
     */
    static protected double savings2(ApproxTSP23 et0, int sk, PointCoords pcs) {
        int i = et0.points.get(0);
        int j = et0.points.get(1);
        int n = et0.points.get(sk);
        int m = et0.points.get(sk + 1);
        double s = savings2(i, j, n, m, pcs);
        return s;
    }

    /**
     * Computes the savings if edges E1=(i,j) and E2=(n,m) are replaced by (i,n)
     * and (j,m). If E1 and E2 crossed in the original Tour, the savings will
     * always be positive. It might be positive in other cases as well. Negative
     * savings means it would actually length the Tour.
     *
     * @param i point at which first edge started
     * @param j point at which first edge ended
     * @param n point at which second edge started
     * @param m point at which second edge ended
     * @param pcs PointCoords to calculate costs between points
     *
     * @return Reduction in tour-length if they swap (negative means increase)
     */
    static protected double savings2(int i, int j, int n, int m, PointCoords pcs) {
        double d1 = pcs.cost(i, j) + pcs.cost(n, m);
        double d2 = pcs.cost(i, n) + pcs.cost(j, m);
        double s = d1 - d2;
        return s;
    }

    /**
     * Do the two-edge splice of edges at positions [0,1] with [k,k+1]. Ideally,
     * it uncrosses two edges, which is always a reduction. However, that is not
     * the only way to get a reduction. Notice that half of the Tour has its
     * direction reversed.
     *
     * Suppose the tour is [0, 1, 2, 3, 4, 5, 6, 7, 8, 0] and k=4, so [0,1]
     * crosses [4,5] Uncrossed is [0, 4, 3, 2, 1, 5, 6, 7, 8, 0].
     *
     * If the tour is [a, b, c, d, e, f, g, h, i, a] and k=4, so [a, b] crosses
     * [e, f] Uncrossed is [a, e, d, c, b, f, g, h, i, a].
     *
     * @param et1 Extended ApproxTSP on which to perform a two-edge splice
     * operation
     * @param sk Index at which to start the splice
     *
     * @return The modified, extended ApproxTSP
     */
    static protected ApproxTSP23 splice2(ApproxTSP23 et1, int sk) {
        assert (et1.eTourP());
        int en = et1.numPoints; // this is the length of the extended tour, which is a loop
        assert (3 <= en); // a ApproxTSP over two points would have en == 3
        assert (validSplice2(et1, sk));

        ApproxTSP23 et2 = new ApproxTSP23();

        et2.add(et1.points.get(0));
        for (int i = sk; 1 <= i; i--) {
            et2.add(et1.points.get(i));
        }
        for (int i = sk + 1; i < en; i++) {
            et2.add(et1.points.get(i));
        }
        et2.numPoints = et2.points.size();

        assert (en == et2.numPoints);

        return et2;
    }

    // -------------------------------------------------------------------------
    /**
     * Repeatedly scan this tour trying to improve it with three-edge splices.
     *
     * It always compares the first edge to other edges, so the tour gets
     * rotated after each improvement. This continues until no further two-edge
     * improvements are found.
     *
     * @param et0 Extended ApproxTSP to be improved
     * @param pcs PointCoords to calculate costs between points
     *
     * @return
     */
    static protected ImprvTour improve3EH(ApproxTSP23 et0, PointCoords pcs) {
        assert (et0.eTourP());
        int m = et0.numPoints;
        ApproxTSP23 etA = et0;
        boolean anyImprovement = false;
        int numImproved = 0;
        int numUnimproved = 0;
        int iter = 0;
        int maxIter = (m<220) ? 1000 :  100*m; // arbitrary limit for testing which is never reached.
        while ((iter < maxIter) && (numUnimproved < m + 1)) { // if we've tried every edge at the front without improvement, stop.
            //System.out.printf("Iteration improve3EH %6d: ", iter);
            IndexImprv bibs = scanBest3EH(etA, pcs);
            int bestIndx = bibs.index;
            double bestSvng = bibs.improvement;
            if (bestIndx < 0) { // no improvement found
                numUnimproved++;
                etA = new ApproxTSP23(rotateETourLeft(etA));
                //System.out.printf("number of unimproved: %5d \n", numUnimproved);
            } else {
                etA = splice3(etA, bestIndx);
                double c = etA.costETour(pcs);
                numUnimproved = 0;
                anyImprovement = true;
                numImproved++;
                //System.out.printf("found improvement %4d with new cost %10.3f after savings %10.3f \n", bestIndx, dMax, bestSvng);
            }
            iter++;
        }
        etA = new ApproxTSP23(rotateToFront(0, etA));

        ImprvTour rslt = new ImprvTour(anyImprovement, numImproved, iter, etA);
        rslt.newCost = etA.costETour(pcs);
        return rslt;
    }

    /**
     * Compare each edge to the [0,1] edge to see if a two-edge splice would
     * help. A positive savings is helpful; a negative savings is harmful.
     *
     * @param et0 The extended tour to be analyzed.
     * @param pcs PointCoords to calculate costs between points
     *
     * @return IndexImprv object, i.e. [bestNdx, bestSvng]
     */
    static protected IndexImprv scanBest3EH(ApproxTSP23 et0, PointCoords pcs) {
        assert (et0.eTourP());
        int m = et0.numPoints;
        double bestSvng = 0.0;
        int bestIndx = -1; // negative one means "none found yet"
        for (int i = 0; i <= m + 1; i++) {
            if (validSplice3(et0, i)) {
                double si = savings3(et0, i, pcs);
                if (bestSvng < si) {
                    bestIndx = i;
                    bestSvng = si;
                }
            }
        }
        IndexImprv rslt = new IndexImprv(bestIndx, bestSvng);
        return rslt;
    }

    static protected boolean validSplice3(ApproxTSP23 et1, int sk) {
        // the positions are [0, 1, 2, ... n-2, n-1,   0]
        // and thus also     [0, 1, 2, ... m-3, m-2, m-1]
        // because [offSet-1, offSet, offSet+1] cannot include [0,1]
        // we need 2 <= offSet-1 and offSet+1 <= m-2, i.e.
        // 3 <= offSet <= m-3
        assert (et1.eTourP());
        int m = et1.points.size(); // including duplicate at the end
        boolean ok = ((3 <= sk) && (sk <= m - 3));
        return ok;
    }

    static protected double savings3(ApproxTSP23 et0, int sk, PointCoords pcs) {
        int x = et0.points.get(0);
        int y = et0.points.get(1);
        int i = et0.points.get(sk - 1);
        int j = et0.points.get(sk);
        int k = et0.points.get(sk + 1);
        double s = savings3(x, y, i, j, k, pcs);
        return s;
    }

    static protected double savings3(int x, int y, int i, int j, int k, PointCoords pcs) {
        double d1 = pcs.cost(x, y) + pcs.cost(i, j) + pcs.cost(j, k);
        double d2 = pcs.cost(x, j) + pcs.cost(j, y) + pcs.cost(i, k);
        double s = d1 - d2;
        return s;

    }

    /**
     * Do the three-edge splice of edges at positions [0,1] with [k-1,k,k+1].
     *
     * Ideally, it replaces a long jog, [k-1,k,k+1] by the short jog [0, k, 1]
     * That is not always a reduction, so you must check. If the tour is [0, 1,
     * 2, 3, 4, 5, 6, 7, 8, 0] splice at k=5, so [4,5,6] is a long jog toward
     * [0,1] After splicing [0, 5, 1, 2, 3, 4, 6, 7, 8, 0]. If the tour is [a,
     * b, c, d, e, f, g, h, i, a] splice at k=5, so [e,f,g] is a long jog toward
     * [a,b] After splicing [a, f, b, c, d e g h i a].
     *
     * @param et1
     * @param sk
     *
     * @return
     */
    static protected ApproxTSP23 splice3(ApproxTSP23 et1, int sk) {
        assert (et1.eTourP());
        int en = et1.points.size();
        assert (3 <= en); // a 'tour' over 2 points would have en == 3
        assert (validSplice3(et1, sk));
        ApproxTSP23 et2 = new ApproxTSP23();
        et2.add(et1.points.get(0));
        et2.add(et1.points.get(sk));
        for (int i = 1; i < sk; i++) {
            et2.add(et1.points.get(i));
        }
        for (int i = sk + 1; i < en; i++) {
            et2.add(et1.points.get(i));
        }
        assert (en == et2.numPoints);
        return et2;
    }

}


// =============================================================================
