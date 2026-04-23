/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Network;

import groupw.Network.NWUtils.PointCoords;
import groupw.Network.NWUtils.Tuple2;

import java.util.Random;

import static groupw.Network.NWUtils.*;
import static java.lang.Math.abs;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

/**
 *
 * @author BenWise
 */
public class UtilitiesTest {

    public UtilitiesTest() {
    }

    @Test
    public void testMakeModifiedTetrahedon() {
        int outerLength = 48;
        int innerLength = 24;
        boolean modifiedP = true;
        int sd = 1173;
        Random prng = makePRNG(sd, true);
        PointCoords pcs = makeTetrahedon(outerLength, innerLength, modifiedP, prng);
        for (int i = 0; i < pcs.nPoints; i++) {
            double x = pcs.xs[i];
            double y = pcs.ys[i];
            System.out.printf("%4d , %7.3f , %7.3f  \n", i, x, y);
        }
    }

    //@Test
    public void testMakeRing() {
        System.out.println("Start testMakeRing");
        int sd = 64826325;
        Random prng = makePRNG(sd, true);
        // these intermediate variables are to ensure that it is the same size as gridTSP
        int nRows = 10 + iMod(prng.nextInt(), 51);
        int nClms = 10 + iMod(prng.nextInt(), 51);
        int nPoints = nRows * nClms;
        double rMin = 300.0;
        double rMax = 375.0;
        NWUtils.PointCoords pc = makeRing(nPoints, rMin, rMax, prng);
        System.out.printf("Made ring of %d points in [%.2f, %.2f] ring\n", nPoints, rMin, rMax);
    }

    @Test
    public void testRoundN() {
        System.out.println("Start testRoundN");
        double x1 = 3.141592654;
        assert(showRoundN(x1, 2, 3.14));
        assert(showRoundN(x1, 4, 3.1416));


        assert(showRoundN(-x1, 2, -3.14));
        assert(showRoundN(-x1, 4, -3.1416));

        double x2 = 1000*x1;
        assert(showRoundN(x2, -2, 3100.0));

        double x3 = 1000*x2;
        assert(showRoundN(x3, -2, 3141600.0));

    }

    private  boolean showRoundN(double x, int n, double expY) {
        double tol = 1e-9;
        double y = roundN(x, n);
        System.out.printf("%.9f , %3d -> %.9f (expected %.9f)\n", x, n, y, expY);
        return (abs(y-expY) < tol);
    }

    //@Test
    public void testMakeNoisyGrid() {
        System.out.println("Start testMakeNoisyGrid");
        int sd = 64826325;
        Random prng = makePRNG(sd, true);
        int nRows = 10 + iMod(prng.nextInt(), 101);
        int nClms = 10 + iMod(prng.nextInt(), 101);
        boolean shuffleP = true;
        double noise = 0.2;
        NWUtils.PointCoords pc = makeNoisyGrid(nRows, nClms, shuffleP, noise, prng);
        System.out.printf("Made noisy grid of %d x %d points \n",
                nRows, nClms);
    }

    //@Test
    public void testNRC() {
        int numTests = 2500;
        System.out.printf("Start testNRC for %d trials\n", numTests);
        int sd = 0; //64826325;
        Random prng = makePRNG(sd, true);
        for (int i = 0; i < numTests; i++) {
            int nRows = 1 + iMod(prng.nextInt(), 251);
            int nClms = 1 + iMod(prng.nextInt(), 251);
            int nPnts = nRows * nClms;
            int n0 = iMod(prng.nextInt(), nPnts);
            assertTrue(0 <= n0);
            assertTrue(n0 < nPnts);
            Tuple2<Integer, Integer> rcPair = rcFromN(n0, nRows, nClms);
            int r0 = rcPair.get0();
            assertTrue(0 <= r0);
            assertTrue(r0 < nRows);
            int c0 = rcPair.get1();
            assertTrue(0 <= c0);
            assertTrue(c0 < nClms);
            int n1 = nFromRC(r0, c0, nRows, nClms);
            assertEquals(n0, n1);
        }
    }
}

// =============================================================================
