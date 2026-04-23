/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Network;

import static groupw.Network.ApproxTSP.rotateToFront;

import groupw.Network.NWUtils.PointCoords;
import java.util.Random;
import org.junit.Test;
import groupw.Network.NWUtils.Tuple2;

import static groupw.Network.NWUtils.DefaultSeedPRNG;
import static groupw.Network.NWUtils.makePRNG;
import static org.junit.Assert.assertTrue;

/**
 *
 * @author BenWise
 */
public class ApproxTSP23Test {

    public ApproxTSP23Test() {
    }

    @Test
    public void testOverall() {
        boolean verbose = true;
        int sd = DefaultSeedPRNG;
        int maxIter = 500; // eight is about the most I've seen
        int nRows = 25;
        int nClms = 30;
        System.out.printf("Start %d x %d (%d) points\n", nRows, nClms, nRows*nClms);
        System.out.flush();
        //Tuple2<ApproxTSP, PointCoords> rslt =  ApproxTSP.ringTSP(verbose, sd);
        Tuple2<ApproxTSP, PointCoords> rslt =  ApproxTSP.gridTSP(nRows, nClms, verbose, sd);
        //Tuple2<ApproxTSP, PointCoords> rslt = ApproxTSP.rectTSP(nRows, nClms, verbose, sd);


        long startTime = System.currentTimeMillis();

        // int BigN = 202;
        // outerLength = (3*BigN - 40)/10 ;
        // innerLength = (BigN + 2)/3 - outerLength;
        //assertTrue(BigN == 3*(outerLength+innerLength)-2);
        boolean modifiedP = false; //true;
        //Tuple2<ApproxTSP, PointCoords> rslt = ApproxTSP.tetrahedon(verbose, outerLength, innerLength, modifiedP, sd);

        ApproxTSP23 bt0 = new ApproxTSP23(rslt.get0());
        PointCoords pcs = rslt.get1();
        //double[][] dm0 = pcs.dMat;
        //List<Double> xs = pcs.xs;
        //List<Double> ys = pcs.ys;

        ApproxTSP23 et0 = new ApproxTSP23(bt0.extendTour());
        double c0 = et0.costETour(pcs);
        //et0.showWholeTour(xs, ys, dm0);

        ApproxTSP23.ImprvTour it = ApproxTSP23.improve23(et0, pcs, verbose, maxIter);
        //boolean improved = it.improved; // never improved on the last iteration
        ApproxTSP23 et1 = (ApproxTSP23) it.newTour;
        double c1 = it.newCost;
        double s = c0 - c1;
        System.out.printf("Initial random cost: %.2f \n", c0);
        System.out.printf("Final semi-optimized cost: %.2f \n", c1);
        System.out.printf("Improved by %.2f \n", s);
        if (et1.numPoints < 5000) {
            et1.showWholeTour(pcs);
        }

        double dMax = et1.highestEdgeCost(pcs);
        System.out.printf("Longest edge has length %.6f \n", dMax);

        long endTime = System.currentTimeMillis();
        System.out.printf("All times are in milliseconds since Unix epoch.\n");
        System.out.println("Start time:   " + startTime);
        System.out.println("End time:     " + endTime);
        System.out.println("Elapsed time: " + (endTime-startTime));
    }

    //@Test
    public void testRotateToFront() {
        System.out.printf("Starting testRotateToFront \n");
        boolean verbose = true;
        int sd = 0;
        Random prng = makePRNG(sd, verbose);
        int eolFreq = 80;
        int numTests = 10 * eolFreq;
        for (int iter = 1; iter <= numTests; iter++) {
            int numPoints = 10 + prng.nextInt(10);
            ApproxTSP23 bt1 = new ApproxTSP23();
            for (int i = 0; i < numPoints; i++) {
                bt1.add(i);
            }
            for (int i = 0; i < numPoints; i++) {
                int j = prng.nextInt(numPoints);
                int vi = bt1.points.get(i);
                int vj = bt1.points.get(j);
                bt1.points.set(i, vj);
                bt1.points.set(j, vi);
            }
            ApproxTSP23 et1 = new ApproxTSP23(bt1.extendTour());
            //System.out.printf("Before rotation:\n");

            int item = prng.nextInt(numPoints + 5) - 2;
            ApproxTSP23 et2 = new ApproxTSP23(rotateToFront(item, et1));
            ApproxTSP23 bt2 = new ApproxTSP23(et2.baseTour());
            assertTrue(numPoints == bt2.points.size());
            assertTrue(numPoints == bt2.numPoints);

            for (int i : bt1.points) {
                assertTrue(bt2.points.contains(i));
            }
            for (int i : bt2.points) {
                assertTrue(bt1.points.contains(i));
            }

            if ((0 <= item) && (item < numPoints)) {
                assertTrue(item == et2.points.get(0));
            } else {
                assertTrue(!bt1.points.contains(item));
                assertTrue(!et2.points.contains(item));
            }
            System.out.printf(".");
            if (0 == (iter % eolFreq)) {
                System.out.printf("|\n");
            }
        }
    }
}

// =============================================================================
