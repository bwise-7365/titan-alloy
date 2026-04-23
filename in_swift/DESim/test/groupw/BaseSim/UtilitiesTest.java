/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */

package groupw.BaseSim;

import static groupw.BaseSim.DSUtils.*;
import static java.lang.Math.abs;
import static java.lang.Math.exp;
import static java.lang.Math.log;
import org.apache.commons.math4.legacy.linear.RealVector;
import org.junit.Test;
import static org.junit.Assert.*;

/**
 *
 * @author bwise
 */
public class UtilitiesTest {

    public UtilitiesTest() {
    }

    protected static double eNormP1 = 1.199164642501;

    /**
     * Make a test point about 1,269 Km below above level
     *
     * @return
     */
    protected RealVector testP1() {
        int numDim = 3;
        RealVector p = makeZeroRV(numDim);
        p.setEntry(0, 5500166.0);
        p.setEntry(1, 4039332.0);
        p.setEntry(2, -3442482.0);
        return p;
    }

    protected static double eNormP2 = 1.028387323431;

    /**
     * Make a test point about 181 Km above sea level
     *
     * @return
     */
    protected RealVector testP2() {
        int numDim = 3;
        RealVector p = makeZeroRV(numDim);
        p.setEntry(0, -1861000.0);
        p.setEntry(1, 3304000.0);
        p.setEntry(2, -5334000.0);
        return p;
    }

    protected static double eNormP3 = 0.912238303380151;

    /**
     * Make a test point about 560 Km below sea level
     *
     * @return
     */
    protected RealVector testP3() {
        int numDim = 3;
        RealVector p = makeZeroRV(numDim);
        p.setEntry(0, 4926477.0);
        p.setEntry(1, -1089889.0);
        p.setEntry(2, 2887788.0);
        return p;
    }

    protected static double eNormP4 = 0.271865474396;

    /**
     * Make a test point about 4,639 Km below sea level
     *
     * @return
     */
    protected RealVector testP4() {
        int numDim = 3;
        RealVector p = makeZeroRV(numDim);
        p.setEntry(0, 1000.0 * KILOMETER);
        p.setEntry(1, 1000.0 * KILOMETER);
        p.setEntry(2, -1000.0 * KILOMETER);
        return p;
    }
    protected static double eNormP5 = 4.2447374188;

    /**
     * Make a test point about 20,672 Km above sea level
     *
     * @return
     */
    protected RealVector testP5() {
        int numDim = 3;
        RealVector p = makeZeroRV(numDim);
        p.setEntry(0, 17500.0 * KILOMETER);
        p.setEntry(1, 13000.0 * KILOMETER);
        p.setEntry(2, -16000.0 * KILOMETER);
        return p;
    }
    protected static double eNormP6 = -1;

    /**
     * Make a test point about 100 Km above sea level
     *
     * @return
     */
    protected RealVector testP6() {
        int numDim = 3;
        RealVector p = makeZeroRV(numDim);
        double f = 0.7275;
        p.setEntry(0, f *  4963.0 * KILOMETER);
        p.setEntry(1, f * -6895.0 * KILOMETER);
        p.setEntry(2, f *  2660.0 * KILOMETER);
        return p;
    }

    /**
     * Test of eNorm method, of class DSUtils.
     */
    @Test
    public void testENorm01() {
        testEProjWGS84(testP1(), eNormP1);
        testEProjWGS84(testP2(), eNormP2);
        testEProjWGS84(testP3(), eNormP3);
        testEProjWGS84(testP4(), eNormP4);
        testEProjWGS84(testP5(), eNormP5);
        testEProjWGS84(testP6(), eNormP6);
    }

    /**
     * Test ellipsoidal projection's initial estimate and Newton step assuming
     * WGS84 ellipsoid. The precise convergence criteria do depend somewhat on
     * the chosen test points.
     *
     * @param p1 Three-Dim point in WGS84 coordinates
     * @param enp Ellipsoidal norm expected for that point
     */
    protected void testEProjWGS84(RealVector p1, double enp) {
        double tol = 1e-6;
        int numDim = 3;
        double stepFrac = 1.0;
        RealVector a = makeRV3(WGS84_MAJOR, WGS84_MAJOR, WGS84_MINOR) ;
        double alpha = WGS84_VMEAN;
        //System.out.printf("%.10f \n", alpha);

        System.out.println("Testing ellipsoidal projection with WGS84");
        double en1 = eNorm(p1, a);
        System.out.printf("%.10f \n", en1);
        System.out.flush();
        if (0.0 < enp){
        assertTrue (abs(en1 - enp) < tol);
        }

        // a reasonable estimate of the local-perpendicular height
        // above the ellipsoid.
        // Proof is left as an exercise to the reader.
        // Hint: use the geometric average
        double estAlt1 = alpha * (en1 - 1.0);
        System.out.printf("Est Alt1: %14.4f meters for initial point\n", estAlt1);

        // a surprisingly close estimate.
        // Proof is left as an exercise to the reader.
        // Hint: approximate the ellipsoid as a sphere
        double lambda1 = 1 - en1;

        RealVector x2 = eProj(p1, a, lambda1, alpha);
        double en2 = eNorm(x2, a);
        double estAlt2 = alpha * (en2 - 1.0);
        System.out.printf("Est Alt2: %14.4f meters\n", estAlt2);
        assertTrue(abs(estAlt2) < KILOMETER); // less than one kilometer error

        // Take one step of Newton's Method
        double lambda2 = eProjNewtonStep(p1, a, lambda1, alpha, stepFrac);
        RealVector x3 = eProj(p1, a, lambda2, alpha);
        double en3 = eNorm(x3, a);
        double estAlt3 = alpha * (en3 - 1.0);
        System.out.printf("Est Alt3: %14.4f meters\n", estAlt3);
        assertTrue(abs(estAlt3) < 1.0); // less than one meter error
        System.out.println();
    }

    @Test
    public void testGreatCircleDistance_HAV_SVD() {
        double tol = 1.0;
        double lat1 = 22.9892;
        double lng1 = -82.4091;
        double lat2 = 13.1564;
        double lng2 = -61.1499;
        double gcDist = 2478820.12; // in meters

        double gcEst = greatCircleDistance(lat1, lng1, lat2, lng2);
        assertTrue(abs(gcDist-gcEst) < tol);
    }
    @Test
    public void testGreatCircleDistance_HAV_SJU() {
        double tol = 1.0;
        double lat1 = 22.9892;
        double lng1 = -82.4091;
        double lat2 = 18.4370;
        double lng2 = -66.0012;
        double gcDist = 1772942.55; // in meters

        double gcEst = greatCircleDistance(lat1, lng1, lat2, lng2);
        assertTrue(abs(gcDist-gcEst) < tol);
    }
    @Test
    public void testGreatCircleDistance_SVD_SJU() {
        double tol = 1.0;
        double lat1 = 13.1564;
        double lng1 = -61.1499;
        double lat2 = 18.4370;
        double lng2 = -66.0012;
        double gcDist = 783061.00; // in meters

        double gcEst = greatCircleDistance(lat1, lng1, lat2, lng2);
        assertTrue(abs(gcDist-gcEst) < tol);
    }

    @Test
    public void testENorm02() {
        int numDim = 5;
        RealVector a = makeZeroRV(numDim);
        RealVector p = makeZeroRV(numDim);
        for (int k = 0; k < numDim; k++) {
            a.setEntry(k, (1.0 + k) * 5000 * KILOMETER);
            p.setEntry(k, 20000 * KILOMETER);
        }
        testEProj(p, a);
    }

    /**
     * Test of eProj method, of class Utilities. The 5-dimensional example shows
     * that there is some tendency to overshoot in highly elliptical cases, even
     * though those overshoots converge fastest. Levenberg-Marquardt would
     * probably help, but I just allowed fractional steps.
     */
    protected void testEProj(RealVector p1, RealVector a) {
        int numDim = p1.getDimension();
        double stepFrac = 1.0;
        System.out.printf("Testing ellipsoidal projection in %d dimensions\n",
                numDim);
        assertTrue (numDim == a.getDimension());
        double alpha = 1.0;
        System.out.println(" k          Axis         Coord    C/A");
        for (int k = 0; k < numDim; k++) {
            double ak = a.getEntry(k);
            double pk = p1.getEntry(k);
            alpha = alpha * ak;
            System.out.printf("%2d  %12.1f  %12.1f  %5.2f\n",
                    k, ak, pk, pk / ak);
        }
        alpha = exp(log(alpha) / numDim);

        double en1 = eNorm(p1, a);

        // a reasonable estimate of the local-perpendicular height
        // above the ellipsoid.
        // Proof is left as an exercise to the reader.
        // Hint: use the geometric average
        double estAlt = alpha * (en1 - 1.0);
        int iter = 0;
        System.out.printf("Est Alt  %3d: %14.2f meters\n", iter, estAlt);
        double lambda1 = 1 - en1;
        while (1.0 < abs(estAlt)) {
            iter++;
            RealVector x2 = eProj(p1, a, lambda1, alpha);
            double en2 = eNorm(x2, a);
            double estAlt2 = alpha * (en2 - 1.0);
            System.out.printf("Est Alt2 %3d: %14.2f meters, down by %8.2f\n",
                    iter, estAlt2, abs(estAlt / estAlt2));
            // Take one step of Newton's Method
            double lambda2 = eProjNewtonStep(p1, a, lambda1, alpha, stepFrac);
            estAlt = estAlt2;
            lambda1 = lambda2;
        }
        System.out.println();
    }

}

// =============================================================================
