/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Network;

import groupw.Network.NWUtils.Tuple2;
import groupw.Network.NWUtils.Tuple3;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import static groupw.Network.NWUtils.*;
import static java.lang.Math.abs;
import static java.lang.Math.sqrt;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

public class FloydWarshallTest {


    static private double myEps = 1e-6; // how closely route-lengths must match

    static private void verifySymmetric(int nPnts, double[][] cMat) {
        for (int i = 0; i < nPnts; i++) {
            for (int j = 0; j < nPnts; j++) {
                double cij = cMat[i][j];
                double cji = cMat[j][i];
                assertTrue(abs(cij - cji) < myEps);
            }
        }
    }

    /**
     * Generate a cost matrix from noisy grid
     *
     * @param nRows
     * @param nClms
     * @param offSetFrac
     * @param sd
     * @return matrix of direct-connection costs as double[][]
     */
    static public double[][] setupDMat(
            int nRows, int nClms,
            double offSetFrac,  int sd) {
        Random prng = makePRNG(sd, false);
        double maxArcCost = -1.0;
        int nPoints = nRows * nClms;
        double scale = 10.0;
        double maxSep = 1.415 * scale; // more than sqrt(2), less than 2.0
        double[][] dMat = new double[nPoints][nPoints];
        List<Double> xs = new ArrayList<>(nPoints);
        List<Double> ys = new ArrayList<>(nPoints);
        for (int k=0; k<nPoints; k++){
            xs.add(0.0);
            ys.add(0.0);
        }
        assertEquals(xs.size(), nPoints);
        assertEquals(ys.size(), nPoints);
        for (int r=0; r<nRows; r++) {
            for (int c=0; c<nClms; c++) {
                int k = nFromRC(r, c, nRows, nClms);
                double x = scale * (c + offSetFrac*prng.nextDouble());
                double y = scale * (r + offSetFrac*prng.nextDouble());
                xs.set(k, x);
                ys.set(k,y);
            }
        }
        for (int i=0; i<nPoints; i++) {
            for (int j = i; j < nPoints; j++) {
                if (i == j) {
                    dMat[i][j] = 0.0;
                } else {
                    double dx = xs.get(i) - xs.get(j);
                    double dy = ys.get(i) - ys.get(j);
                    double d = sqrt((dx * dx) + (dy * dy));
                    if (d < maxSep) {
                        dMat[i][j] = d;
                        dMat[j][i] = d;
                        if (maxArcCost < d){
                            maxArcCost = d;
                        }
                    } else {
                        dMat[i][j] = Double.MAX_VALUE;
                        dMat[j][i] = Double.MAX_VALUE;
                    }
                }
            }
        }
        System.out.printf("Maximum real arc cost: %9.4f\n", maxArcCost);
        verifySymmetric(nPoints, dMat);
        return dMat;
    }


    @Test
    public void testCalcMinDist() {
        System.out.println("\nStarting testCalcMinDist");
        boolean verbose = true;
        int nRows = 15;
        int nClms = 24;
        // the above three numbers are carefully tuned to give 1300-1320
        // undirected arcs. That is is an average connectivity of 7.2-7.3.
        // If each of 360 points had 7 arcs, that would be 2520 with double
        // counting, or about 1260 undirected arcs.
        int sd = DefaultSeedPRNG;
        sd = 0;
        double offSetFrac = 0.10; // noise in grid
        double[][] dm0 = setupDMat(nRows, nClms, offSetFrac, sd);

        int numPoints = nRows * nClms;
        FloydWarshall fw = new FloydWarshall();
        fw.calcMinCost(numPoints, dm0, verbose);

        //fw.showRawCost();
        if (verbose) {
            NWUtils.Tuple3<Integer, Integer, Double> rslt = showLongestOptRoute(numPoints, fw);
            int maxJ = rslt.get0();
            int maxI = rslt.get1();
            Tuple2<Integer, Integer> rj = rcFromN(maxJ, nRows, nClms);
            int xj = rj.get0();
            int yj = rj.get1();
            Tuple2<Integer, Integer> ri = rcFromN(maxI, nRows, nClms);
            int xi = ri.get0();
            int yi = ri.get1();
            System.out.printf("From [%2d, %2d] to [%2d, %2d] \n",
                    xj, yj, xi, yi);
        }
    }

    public Tuple3<Integer, Integer, Double> showLongestOptRoute(int nPoints, FloydWarshall fw) {
        double maxCost = -1.0;
        int maxI = 0;
        int maxJ = 0;
        for (int i = 0; i < nPoints; i++) {
            for (int j = 0; j < nPoints; j++) {
                double cij = fw.getMinCost(i, j);
                double cji = fw.getMinCost(j, i);
                if (cij != cji) {
                    assertTrue(abs(cij - cji)<myEps); // My test cost matrix is symmetric
                }
                if ((cij < Double.MAX_VALUE) && (maxCost < cij)) {
                    //System.out.printf("Resetting max cost route [ %4d -> %4d ] from %7.2f to %7.2f \n", i, j, maxCost, cij);
                    maxCost = cij;
                    maxI = i;
                    maxJ = j;
                }
            }
        }
        assertTrue(0.0 < maxCost);
        // Because I tested on Euclidean distances, the cost matrix
        // must be symmetric: swap them to make sure order does not matter
        List<Integer> path = fw.calcMinPath(maxJ, maxI);
        int pLength = path.size();
        double pCost = 0.0;
        for (int i = 0; i < pLength; i++) {
            int ndx = path.get(i);
            if (0 < i) {
                pCost = pCost + fw.getMinCost(path.get(i - 1), ndx);
            }
        }
        System.out.printf("Longest optimal route [ %4d -> %4d ] has length %2d steps and cost %8.4f\n",
                maxJ, maxI, pLength, pCost);
        for (int i = 0; i < pLength; i++) {
            int ndx = path.get(i);
            System.out.printf("  %4d\n", ndx);
        }
        assertTrue(abs(pCost - maxCost) < myEps);
        Tuple3<Integer, Integer, Double> rslt = new Tuple3<>(maxJ, maxI, pCost);
        return rslt;
    }

}

// =============================================================================
