/*
 *
 */
package groupw.SimpleIADS;

import static groupw.Network.NWUtils.ReportingLevel.Silent;
import groupw.SimpleIADS.InterceptGuidance.InterceptResult;
import static groupw.SimpleIADS.InterceptGuidance.closestInterceptTime;
import static groupw.SimpleIADS.InterceptGuidance.interceptCourse;
import static java.lang.Math.abs;
import static java.lang.Math.sqrt;
import org.apache.commons.math4.legacy.linear.ArrayRealVector;
import org.apache.commons.math4.legacy.linear.RealVector;
import org.junit.Test;
import static org.junit.Assert.*;

/**
 *
 * @author bwise
 */
public class InterceptGuidanceTest {

    public InterceptGuidanceTest() {
    }
    /**
     * Test of closestInterceptTime method 2D, of class InterceptGuidance. Make
     * sure it gets the right intercept time in a case where perfect
     * interception is possible.
     */
    @Test
    public void testClosestInterceptTimeV00() {
        System.out.println("---------------");
        System.out.println("testClosestInterceptTimeV00");
        int numDim = 2;
        double[] p = new double[numDim];
        double[] v = new double[numDim];
        double[] q = new double[numDim];
        double[] w = new double[numDim];
        for (int i = 0; i < numDim; i++) {
            p[i] = 0.0;
            v[i] = 0.0;
            q[i] = 0.0;
            w[i] = 0.0;
        }
        p[1] = 0;
        p[0] = 20 * 1000; // meters
        v[0] = -100;
        v[1] = 100;
        q[0] = 10 * 1000; // meters
        q[1] = 10 * 1000; // meters
        // |p-q| is 14 * 1000 meters
        // |v| is 141 m/s,
        // so time should be 100 seconds
        double tau = 100.0;
        RealVector missileP = new ArrayRealVector(p);
        RealVector missilV = new ArrayRealVector(v);
        RealVector trgtQ = new ArrayRealVector(q);
        RealVector trgtW = new ArrayRealVector(w);

        InterceptGuidance.reportLevel = Silent;
        double t2 = closestInterceptTime(
                missileP, missilV,
                trgtQ, trgtW);
        System.out.println("");
        System.out.printf("Closest intercept time: %.4f \n", t2);
        System.out.flush();
        boolean e = abs(t2 - tau) < 0.1;
        assertTrue(e);
        InterceptGuidance.reportLevel = Silent;
    }

    /**
     * Test of closestInterceptTime method 4D, of class InterceptGuidance. Make
     * sure it gets the right intercept time in a case where perfect
     * interception is possible.
     */
    @Test
    public void testClosestInterceptTimeV01() {
        System.out.println("---------------");
        System.out.println("testClosestInterceptTimeV01");
        // I picked random P, Q, W
        // s.t. (Q-p) dot W > 0 and |W| = 300 m/s
        // set intercept time to tau = 120 seconds,
        // calculated the resulting V = W + (Q-p)/tau.
        // This gave the interceptor speed of 480.418 m/s
        int numDim = 4;
        double[] p = new double[numDim];
        double[] v = new double[numDim];
        double[] q = new double[numDim];
        double[] w = new double[numDim];
        p[0] = +8450; // meters
        p[1] = -8395;
        p[2] = -9332;
        p[3] = -171;

        v[0] = 207.033; // meters / second
        v[1] = 305.300;
        v[2] = 307.692;
        v[3] = -7.508;

        q[0] = 12294; // meters
        q[1] = 15761;
        q[2] = 2511;
        q[3] = 7328;

        w[0] = 175; // meters/sec
        w[1] = 104;
        w[2] = 209;
        w[3] = -70;

        double tau = 120.0; // seconds

        RealVector missileP = new ArrayRealVector(p);
        RealVector missilV = new ArrayRealVector(v);
        RealVector trgtQ = new ArrayRealVector(q);
        RealVector trgtW = new ArrayRealVector(w);

        InterceptGuidance.reportLevel = Silent;
        double t2 = closestInterceptTime(
                missileP, missilV,
                trgtQ, trgtW);
        System.out.println("");
        System.out.printf("Closest intercept time: %.4f \n", t2);
        System.out.flush();
        boolean e = abs(t2 - tau) < 0.1;
        assertTrue(e);
        InterceptGuidance.reportLevel = Silent;
    }

    /**
     * Test of interceptCourse method in 2D, of class InterceptGuidance. Make
     * sure it gets approximately right intercept vector in a case where perfect
     * interception is possible.
     */
    @Test
    public void testInterceptCourseV00() {
        System.out.println("---------------");
        System.out.println("testInterceptCourseV00");
        int numDim = 2;
        double[] p = new double[numDim];
        double[] q = new double[numDim];
        double[] w = new double[numDim];
        p[1] = 0;
        p[0] = 20 * 1000; // meters
        q[0] = 10 * 1000; // meters
        q[1] = 10 * 1000; // meters
        w[0] = 0.0;
        w[1] = 0.0;
        // |p-q| is 100*100*sqrt(2) meters
        // |v| is 100*sqrt(2) m/s,
        // so time should be 100 seconds
        double tau = 100.0; // seconds
        double missDist = 0.0; // meters
        double timeTol = 0.0001; // seconds, convergence
        int maxIter = 100;

        RealVector missileP = new ArrayRealVector(p);
        RealVector trgtQ = new ArrayRealVector(q);
        RealVector trgtW = new ArrayRealVector(w);
        double speed = 100.0 * sqrt(2);

        InterceptGuidance.reportLevel = Silent;
        InterceptResult ir = interceptCourse(missileP, speed,
                trgtQ, trgtW,
                timeTol);

        reportIR(ir);
        assertTrue(abs(ir.tIntercept - tau) < 0.1);
        assertTrue(abs(ir.missDist - missDist) < 0.1);
        assertTrue(ir.iterations < maxIter);

        // The intercept vector is simple
        RealVector v2 = ir.v;
        assertTrue(abs(v2.getEntry(0) + 100.0) < 0.1);
        assertTrue(abs(v2.getEntry(1) - 100.0) < 0.1);
        InterceptGuidance.reportLevel = Silent;
    }

    /**
     * Test of interceptCourse method in 2D, of class InterceptGuidance. Make
     * sure it gets approximately right nearest-approach vector in a case where
     * perfect interception is not possible.
     */
    @Test
    public void testInterceptCourseV01() {
        System.out.println("---------------");
        System.out.println("testInterceptCourseV01");
        int numDim = 2;
        double[] p = new double[numDim];
        double[] v = new double[numDim];
        double[] q = new double[numDim];
        double[] w = new double[numDim];
        p[1] = 0;
        p[0] = 0;
        q[0] = 10 * 1000; // meters
        q[1] = 0; // meters
        w[0] = 0;
        w[1] = 500.0; // m/s
        RealVector missileP = new ArrayRealVector(p);
        RealVector missilV = new ArrayRealVector(v);
        RealVector trgtQ = new ArrayRealVector(q);
        RealVector trgtW = new ArrayRealVector(w);
        double speed = 400.0;

        double tau = 80.0 / 3.0; // seconds
        double missDist = 6000.0; // meters
        double timeTol = 0.0001; // seconds convergence
        int maxIter = 100;

        InterceptGuidance.reportLevel = Silent;
        InterceptResult ir = interceptCourse(missileP, speed,
                trgtQ, trgtW,
                timeTol);
        reportIR(ir);
        assertTrue(abs(ir.tIntercept - tau) < 0.1);
        assertTrue(abs(ir.missDist - missDist) < 0.1);
        assertTrue(ir.iterations < maxIter);

        // The intercept vector is simple
        RealVector v2 = ir.v;
        assertTrue(abs(v2.getEntry(0) - 240.0) < 0.1);
        assertTrue(abs(v2.getEntry(1) - 320.0) < 0.1);
        InterceptGuidance.reportLevel = Silent;
    }

    /**
     * Test of interceptCourse method in 4D, of class InterceptGuidance. Make
     * sure it gets the right intercept vector in a case where perfect
     * interception is possible.
     */
    @Test
    public void testInterceptCourseV02() {
        System.out.println("---------------");
        System.out.println("testInterceptCourseV02");
        // I picked random P, Q, W
        // s.t. (Q-p) dot W > 0 and |W| = 300 m/s
        // set intercept time to tau = 120 seconds,
        // calculated the resulting V = W + (Q-p)/tau
        // This gave the interceptor speed of 480.41 m/s
        int numDim = 4;
        double[] p = new double[numDim];
        double[] v = new double[numDim];
        double[] q = new double[numDim];
        double[] w = new double[numDim];
        p[0] = +8450; // meters
        p[1] = -8395;
        p[2] = -9332;
        p[3] = -171;

        v[0] = 207.0; // meters / second
        v[1] = 305.3;
        v[2] = 307.7;
        v[3] = -7.5;

        q[0] = 12294; // meters
        q[1] = 15761;
        q[2] = 2511;
        q[3] = 7328;

        w[0] = 175; // meters/sec
        w[1] = 104;
        w[2] = 209;
        w[3] = -70;

        double tau = 120.0; // seconds
        double missDist = 0.0; // meters
        double timeTol = .0001; // seconds convergence
        int maxIter = 100;

        RealVector missileP = new ArrayRealVector(p);
        RealVector missilV = new ArrayRealVector(v);
        RealVector trgtQ = new ArrayRealVector(q);
        RealVector trgtW = new ArrayRealVector(w);
        double speed = 480.41;

        InterceptGuidance.reportLevel = Silent;
        InterceptResult ir = interceptCourse(missileP, speed,
                trgtQ, trgtW,
                timeTol);
        reportIR(ir);
        assertTrue(abs(ir.tIntercept - tau) < 0.1);
        assertTrue(abs(ir.missDist - missDist) < 0.1);
        assertTrue(ir.iterations < maxIter);
        InterceptGuidance.reportLevel = Silent;
    }

    public void reportIR(InterceptResult ir) {
        double t2 = ir.tIntercept;
        double md = ir.missDist;
        int iter = ir.iterations;
        RealVector v2 = ir.v;
        int numDim = v2.getDimension();
        double s2 = v2.getNorm();
        System.out.println("");
        System.out.printf("Iterations: %d \n", iter);
        System.out.printf("Time: %8.4f \n", t2);
        System.out.printf("Miss: %8.4f \n", md);

        System.out.printf("%d-Velocity: %.3f [ ", numDim, s2);

        for (int i = 0; i < numDim; i++) {
            System.out.printf(" %6.2f ", v2.getEntry(i));
        }
        System.out.printf("] \n");
        System.out.flush();

    }
}
