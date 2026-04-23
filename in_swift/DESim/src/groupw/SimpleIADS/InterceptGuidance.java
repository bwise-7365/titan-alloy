/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.SimpleIADS;

import groupw.BaseSim.DSUtils;
import static java.lang.Math.abs;

import org.apache.commons.math4.legacy.linear.RealVector;
import static groupw.BaseSim.DSUtils.makeZeroRV;
import groupw.Network.NWUtils.ReportingLevel;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static groupw.Network.NWUtils.ReportingLevel.Silent;

/**
 *
 * @author BenWise
 *
 * This class provides the logic to calculate intercept courses. If the target
 * is stationary, it converges quickly. If the target is too fast, returns the
 * time and vector of closest approach.
 */
public class InterceptGuidance {

    static public class InterceptResult {
        public double tIntercept = 0.0;
        public double missDist = 0.0;
        public int iterations = 0;
        public RealVector v;
}

    public InterceptGuidance() {
        // nothing yet
    }

    /**
     * Calculate exactly the time, future or past, of closest intercept
     *
     * @param p Position vector of first object
     * @param v Velocity vector of first object
     * @param q Position vector of second object
     * @param w Velocity vector of second object
     * @return
     */
    static public double closestInterceptTime(RealVector p, RealVector v, RealVector q, RealVector w)
            throws ArithmeticException {
        int numDim = p.getDimension();
        if ((numDim < 2) || (4 < numDim)) {
            throw new ArithmeticException("closestInterceptTime: Dimensions must be 2, 3, or 4.");
        }
        RealVector A = p.subtract(q);
        RealVector b = w.subtract(v);
        double num = A.dotProduct(b);
        double dnm = b.dotProduct(b);
        if (0.0 < dnm) {
            // do nothing
        } else {
            throw new ArithmeticException("closestInterceptTime: zero relative motion is not allowed.");
        }
        double t = num / dnm;
        return t;
    }

    /**
     * Calculate intercept course and return data about interception or near-miss.
     *
     * If the target is stationary, it converges quickly.
     * If the target is too fast, returns the time and vector of closest approach.
     * That time might be zero delay, but never negative.
     * This is an iterative algorithm: it might oscillate or even fail to converge.
     * The 'tTol' parameter is critical. One heuristic is to pick a threshold
     * distance based on the physics of the problem (e.g. 50% kill blast
     * radius of an air-to-air missile), divide by the sum of the target
     * and interceptor speeds, quarter it then round to two significant figures.
     *
     * @param p The current position vector of the interceptor
     * @param s Desired speed of the interceptor, meters / second
     * @param q The current position vector of the target
     * @param w The current velocity vector of the target (often stationary)
     * @param tTol tolerance on intercept-time below which we consider the
     * algorithm to have converged, seconds
     *
     * @return InterceptResult of data about how the interception (or closest approach) will happen
     */
    static public InterceptResult interceptCourse(
            RealVector p, double s,
            RealVector q, RealVector w,
            double tTol) {
        assert (0.0 < tTol);
        RealVector A = p.subtract(q);

        // We can only test the stability of the result, but
        // that might be several steps from the true time.
        // A factor of 10 seems to be enough to get it
        // withing tTol of the true time.
        //Utilities.showVector(A, "%9.2f");
        boolean done = false;
        int iter = 0;
        RealVector miss;
        double missDist = 0.0; // meters
        double t1 = 0.0; // seconds
        RealVector v1 = makeZeroRV(p.getDimension());

        while (!done) {
            // q2 = lead-position, i.e. where target would be after the lead-time elapses
            RealVector q2 = q.combine(1.0, t1, w); // q2 = (1.0 * q) + (t1 * w)

            // speed-limited velocity vector pointing at that lead-position
            RealVector v2 = q2.subtract(p).unitVector().mapMultiply(s); // v2 = s * unitize(q2-p)

            double t2 = closestInterceptTime(p, v2, q, w);
            if (t2 < 0.0) {
                t2 = 0.0;
            }
            double deltaT = t2 - t1;
            double absDT = abs(deltaT);

            if (High.ordinal() <= reportLevel.ordinal()) {
                System.out.printf("interceptCourse teration %3d \n", iter);
                System.out.println("V2:");
                DSUtils.showVector(v2, "%9.3f");
                System.out.printf("Speed: %.3f \n", v2.getNorm());
                System.out.println("Q2:");
                DSUtils.showVector(q2, "%9.3f");
                System.err.flush();
            }

            // update loop variables
            done = (absDT < tTol);
            iter = iter + 1;
            RealVector p3 = p.combine(1.0, t2, v2); // p3 = p + (t2 * v2)
            RealVector q3 = q.combine(1.0, t2, w);  // q3 = q + (t2 * w)
            miss = p3.subtract(q3);
            missDist = miss.getNorm();

            // try to avoid very slow convergence.
            // If we just adjusted t1 = t2, i,e, t1 = t1 + deltaT,
            // then it would perpetually aim slightly behind the target.
            // So we adjust by 5/4 of deltaT, not exactly deltaT.
            // Q: Why add 1/4 of deltaT?
            // A: Out of the set {3/4, 1/2, 3/8, 1/4, 1/8, 0} that fraction gave the
            //    lowest RMS iterations over my test suite with timeTol = 0.0001:
            //    {30.76 , 16.27 , 14.75 , 14.45 , 15.71 , 17.63}
            //    and also the lowest average:
            //    {25.33 , 15.33 , 13.67 , 13.33 , 14.00, 14.00 }
            t1 = t2 + (0.25 * deltaT);
            v1 = v2;

            if (High.ordinal() <= reportLevel.ordinal()) {
                System.out.printf("Iter: %3d  T: %.5f missDist: %.4f  dt: %.5f / %.5f \n\n",
                        iter, t1, missDist, deltaT, tTol);
            }
        }
        assert (0.0 <= t1);
        InterceptResult rslt = new InterceptResult();
        rslt.tIntercept = t1;
        rslt.missDist = missDist;
        rslt.iterations = iter;
        rslt.v = v1;
        return rslt;
    }
    public static ReportingLevel reportLevel = Silent;

}


// =============================================================================
