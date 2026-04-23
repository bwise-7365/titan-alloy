/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Network;

import static groupw.Network.NWUtils.DefaultSeedPRNG;

import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

import static java.lang.Math.abs;
import static org.junit.Assert.assertTrue;

import groupw.Network.NWUtils.PointCoords;
import groupw.Network.NWUtils.Tuple2;
import static groupw.Network.NWUtils.iMod;
import static groupw.Network.NWUtils.makePRNG;
import static groupw.Network.NWUtils.shuffle;

public class combine23FI {

    @Test
    public void someTest() {
        // nothing yet
    }

    @Test
    public void testOverall() {
        boolean verbose = false;
        int sd = DefaultSeedPRNG;
        Random prng = makePRNG(sd, false);

        int max23Iter = 20;
        int outerIter = 10;
        int innerIter = 5;

        int nRows = 15;
        int nClms = 24;

        for (int iter = 0; iter < outerIter; iter++) {
            List<Double> costs = new ArrayList<>(3 * innerIter);
            int sd2 = prng.nextInt(1000 * 1000);
            Tuple2<ApproxTSP, PointCoords> rslt;
            if (0 == iMod(iter, 2)) {
                rslt = ApproxTSP.ringTSP(verbose, sd2);
            } else {
                rslt = ApproxTSP.rectTSP(nRows, nClms, verbose, sd2);
            }
            PointCoords pcs = rslt.get1();
            //List<Double> xs = pcs.xs;
            //List<Double> ys = pcs.ys;
            int nPointsTotal = pcs.nPoints; //xs.size();

            for (int perm = 0; perm < innerIter; perm++) {
                // try Approx23 alone
                List<Integer> pts = shuffle(rslt.get0().points, prng);
                ApproxTSP23 bt0 = new ApproxTSP23(pts);
                ApproxTSP23 et0 = new ApproxTSP23(bt0.extendTour());
                ApproxTSP23.ImprvTour it = ApproxTSP23.improve23(et0, pcs, verbose, max23Iter);
                double c23 = it.newCost;

                // try ApproxFI alone
                ApproxTSPFI et1 = new ApproxTSPFI();
                et1.add(bt0.points.get(nPointsTotal / 2));
                et1.add(bt0.points.get(nPointsTotal / 2));
                while (et1.points.size() < nPointsTotal) {
                    ApproxTSPFI.extendETour(et1, nPointsTotal, pcs);
                }
                double cFI = et1.costETour(pcs);

                // try Approx23 after ApproxFI
                ApproxTSP23 et2 = new ApproxTSP23();
                for (int i = 0; i < et1.numPoints; i++) {
                    et2.add(et1.points.get(i));
                }

                assertTrue(abs(et2.costETour(pcs) - cFI) < 1e-6);
                ApproxTSP23.ImprvTour it2 = ApproxTSP23.improve23(et2, pcs, verbose, max23Iter);
                double c23FI = it2.newCost;

                System.out.printf("TSP costs , %2d , %2d , %10.3f , %10.3f , %10.3f\n", iter, perm, c23, cFI, c23FI);
                System.out.flush();
            }
        }
    }
}

// =============================================================================
