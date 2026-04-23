/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.BaseSim;

import static java.lang.Math.*;

import java.util.Random;
import org.apache.commons.math4.legacy.linear.ArrayRealVector;
import org.apache.commons.math4.legacy.linear.RealVector;

/**
 *
 * @author BenWise
 *
 * Define useful constants and functions. All times are seconds. All distances
 * are meters. All weights are kilograms. All volumes are liters.
 */
abstract public class DSUtils {


    /**
     * Convert the given number of seconds into D:HM:S. Unless you go past
     * midnight on the 999th day, the time stamp is 14 characters. Days are at
     * least three zero-padded digits, more when necessary. Hours and Minutes
     * are four digits in 24-hour US Military format, so 0000 is midnight, 0030
     * is thirty minutes after midnight, 1230 is thirty minutes after noon, 2300
     * is 11pm, and so on. The seconds are two zero-padded digits then two
     * decimal places.
     *
     * @param t The time, in seconds, to be formatted
     * @return String of the formatted time
     */
    static public String timeStamp(double t) {
        assert (0.0 <= t);
        int days = (int) (t / SECONDS_PER_DAY);
        t = t - (days * SECONDS_PER_DAY);
        int hours = (int) (t / SECONDS_PER_HOUR);
        t = t - (hours * SECONDS_PER_HOUR);
        int minutes = (int) (t / SECONDS_PER_MINUTE);
        t = t - (minutes * SECONDS_PER_MINUTE);
        assert (0.0 <= t);
        String rslt = String.format("%03d:%02d%02d:%07.4f", days, hours, minutes, t);
        return rslt;
    }

    static public RealVector makeZeroRV(int numDim) {
        double[] dv = new double[numDim];
        for (int i = 0; i < numDim; i++) {
            dv[i] = 0.0;
        }
        RealVector v = new ArrayRealVector(dv);
        return v;
    }

    static public RealVector makeRV2(double a, double b) {
        double[] dv = new double[2];
        dv[0] = a;
        dv[1] = b;
        RealVector v = new ArrayRealVector(dv);
        return v;
    }

    static public RealVector makeRV3(double a, double b, double c) {
        double[] dv = new double[3];
        dv[0] = a;
        dv[1] = b;
        dv[2] = c;
        RealVector v = new ArrayRealVector(dv);
        return v;
    }

    /**
     * Make a random vector with coordinates in [-0.5, +0.5]
     *
     * @param numDim how many dimensions
     * @param prng pseudo random number generator
     * @return random vector
     */
    static public RealVector makeUnitBoxRandomRV(int numDim, Random prng) {
        double[] dv = new double[numDim];
        for (int i = 0; i < numDim; i++) {
            dv[i] = prng.nextDouble() - 0.5;
        }
        RealVector v = new ArrayRealVector(dv);
        return v;
    }

    /**
     * If necessary, rescale the vector so that its magnitude is no more than x
     *
     * @param v1 initial vector
     * @param x maximum norm of result
     * @return rescaled vector
     */
    static public RealVector clipNorm(RealVector v1, double x) {
        assert (0.0 <= x);
        RealVector v2 = v1;
        double x1 = v1.getNorm();
        if (x < x1) {
            double f = x / x1;
            v2 = v2.mapMultiply(f);
        }
        return v2;
    }

    static public boolean segmentIntersect(RealVector va0, RealVector va1,
                                           RealVector vb0, RealVector vb1,
                                           double thresh) {
        /*
        Choose u and v to minimize |E| where
        E = [(a0+a1)/2 + u(a1-a0)/2] - [(b0+b1)/2 + v(b1-b0)/2]
        On the left, u=+1 -> a1 and u=-1 -> a0.
        Similarly on right.
        So we clip at |u| <= 1, similarly for y
        E = X - (uY + vZ)
        minimizing E*E yields
        XY = uYY + vZY
        XZ = uYZ + vZZ
        and we solve the 2x2
         */
        RealVector sa = va0.add(va1);
        RealVector sb = vb0.add(vb1);
        RealVector X = (sa.subtract(sb)).mapDivide(2.0);
        RealVector Y = (va0.subtract(va1)).mapDivide(2.0);
        RealVector Z = (vb1.subtract(vb0)).mapDivide(2.0);

        double a11 = Y.dotProduct(Y);
        double a12 = Z.dotProduct(Y);
        double a21 = Y.dotProduct(Z);
        double a22 = Z.dotProduct(Z);
        double b1 = X.dotProduct(Y);
        double b2 = X.dotProduct(Z);

        // Now d*u  = t1 and d*v = t2
        double d = (a11 * a22) - (a12 * a21);
        double t1 = (b1 * a22) - (b2 * a12);
        double t2 = (b2 * a11) - (b1 * a21);
        if (d < 0.0) {
            t1 = -t1;
            t2 = -t2;
            d = -d;
        }

        double u;
        if (t1 <= -d) {
            u = -1.0;
        } else if (t1 >= +d) {
            u = +1.0;
        } else {
            u = t1 / d;
        }

        double v;
        if (t2 <= -d) {
            v = -1.0;
        } else if (t2 >= +d) {
            v = +1.0;
        } else {
            v = t2 / d;
        }
        RealVector uY = Y.mapMultiply(u);
        RealVector vZ = Z.mapMultiply(v);
        RealVector E = X.subtract(uY.add(vZ));
        double eMag = E.getNorm();
        return (eMag <= thresh);
    }

    static public double negExp(double mean, Random prng) {
        double dv = prng.nextDouble();
        double v = -mean * log(1 - dv);
        return v;
    }

    static public double uniform(double vMin, double vMax, Random prng) {
        double dv = prng.nextDouble();
        double v = vMin + ((vMax - vMin) * dv);
        return v;
    }

    static public void showVector(RealVector v, String frmt) {

        int dim = v.getDimension();
        String f2 = "%2d " + frmt + "\n";
        for (int i = 0; i < dim; i++) {
            System.out.printf(f2, i, v.getEntry(i));
        }
    }

    /**
     * Recommended length of time step for a Mover moving to a desired location.
     *
     * The recommendation is to set the time step so that the Mover goes 1/4 of
     * the way to the desired location (target, way-point, etc) in the next
     * step. If it is chasing a moving target, this speeds up events as it gets
     * closer. If it is approaching a way-point, this ensures that it will not
     * overshoot the WP before it gets close enough to start turning. There is a
     * maximum time step to ensure that updates are "frequent enough" when far
     * from the desired location.
     *
     * @param dist Distance to target, way-point, etc in meters
     * @param speed Expected mean speed of approach in meters / second
     * @param dtMax Longest desired time step
     *
     * @return
     */
    static public double recMoverTimeStep(double dist, double speed, double dtMax) {
        assert (0.0 <= dist);
        assert (0.0 <= speed);
        assert (0.0 < dtMax);
        double dt = dtMax;
        if (0.0 < dist) {
            double remTime = dist / speed;
            dt = min(dtMax, RECOMMENDED_STEP_FRACTION * remTime);
        }
        return dt;
    }

    /**
     * Return the norm of x, using ellipsoid radii 'a'. Exactly one if on the
     * ellipsoid, more than one if outside, less than one if inside.
     *
     * @param x Vector whose eNorm is to be computed
     * @param a Radii of the reference ellipsoid
     * @return ellipsoidal norm of x
     */
    static public double eNorm(RealVector x, RealVector a) {
        double s = 0.0;
        int n = x.getDimension();
        assert (a.getDimension() == n);
        for (int k = 0; k < n; k++) {
            double ai = a.getEntry(k);
            double xi = x.getEntry(k);
            s = s + (xi * xi) / (ai * ai);
        }
        s = sqrt(s);
        return s;
    }

    static public RealVector eProj(RealVector p, RealVector a,
            double lambda, double alpha) {
        int n = p.getDimension();
        assert (a.getDimension() == n);
        RealVector x = makeZeroRV(n);
        double a2 = (alpha * alpha);
        for (int k = 0; k < n; k++) {
            double ak = a.getEntry(k);
            double pk = p.getEntry(k);
            double xk = pk * (ak * ak) / ((ak * ak) - lambda * a2);
            x.setEntry(k, xk);
        }
        return x;
    }

    static public double eProjNewtonStep(RealVector p, RealVector a,
            double lambda, double alpha,
            double stepFrac) {
        int n = p.getDimension();
        assert (a.getDimension() == n);
        assert (0.0 < stepFrac);
        assert (stepFrac <= 1.0);
        double a2 = (alpha * alpha);
        double fVal = 0.0;
        double fSlp = 0.0;
        for (int k = 0; k < n; k++) {
            double ak1 = a.getEntry(k);
            double ak2 = ak1 * ak1;
            double pk = p.getEntry(k);
            double dnm = ak2 - (a2 * lambda);
            double fx = (pk * ak1) / dnm;
            double fx2 = fx * fx;
            fVal = fVal + fx2;
            double sx = (fx2 * 2.0 * a2) / dnm;
            fSlp = fSlp + sx;
        }
        fVal = fVal - 1.0;
        double delta = -fVal / fSlp;
        double lambda2 = lambda + (stepFrac * delta);
        return lambda2;
    }

    /**
     *  Given (latitude,longitude) in degrees, return great circle distance in meters,
     *  assuming spherical earth.
     *
     * @param dLat1 first latitude, in degrees
     * @param dLng1 first longitude, in degrees
     * @param dLat2 second latitude, in degrees
     * @param dLng2 second longitude, in degrees
     * @return great circle distance in meters
     */
    static public double greatCircleDistance(double dLat1, double dLng1, double dLat2, double dLng2) {
        double phi1 = RAD_PER_DEGREE * dLat1;
        double lambda1 = RAD_PER_DEGREE * dLng1;
        double phi2 = RAD_PER_DEGREE * dLat2;
        double lambda2 = RAD_PER_DEGREE * dLng2;

        double dx = cos(phi2) * cos(lambda2) - cos(phi1) * cos(lambda1);
        double dy = cos(phi2) * sin(lambda2) - cos(phi1) * sin(lambda1);
        double dz = sin(phi2) - sin(phi1);

        double dc = sqrt((dx * dx) + (dy * dy) + (dz * dz));
        return dc * WGS84_VMEAN;
    }



    // Miscellaneous constants
    public static final double RAD_PER_DEGREE = PI/180.0;

    public static final double RECOMMENDED_STEP_FRACTION = 0.25;

    public static final Integer MINIMUM_TYPE_NAME_LENGTH = 2; // characters
    /**
     * There are 60 seconds in a minute.
     */
    public static final double SECONDS_PER_MINUTE = 60.0; // seconds

    /**
     * There are 3,600 seconds in an hour.
     */
    public static final double SECONDS_PER_HOUR = SECONDS_PER_MINUTE * 60.0; // seconds

    /**
     * There are 86,400 seconds in a day.
     */
    public static final double SECONDS_PER_DAY = 24.0 * SECONDS_PER_HOUR;

    /**
     * Definition of a nautical mile is 1852 meters exactly.
     */
    public static final double NAUTICAL_MILE = 1852.0;  // meters

    /**
     * Definition of a statue mile is 5280 feet, where 1 inch is defined as
     * exactly 2.54 cm, i.e. 0.0254 meters exactly Thus, 1609.344 = 5280 * 12 * 0.0254 exactly
     */
    public static final double STATUTE_MILE = 1609.344;  // meters
    
    /**
     * Definition of a kilometer is exactly 1000 meters
     */
    public static final double KILOMETER = 1000.0;  // meters

    /**
     * 1 Knot is defined to be 1 nm / 1 hour, so it is about 0.5144 m/s, because
     * 1 nm / hr = 1852 meters / 3600 sec
     */
    public static final double KNOTS = NAUTICAL_MILE / SECONDS_PER_HOUR;

    /**
     * Standard value for speed of sound through air at sea level, 20 degrees C,
     * is 343 meters / second. Exact value depends on many parameters (e.g.
     * humidity).
     */
    public static final double MACH_SEA_LEVEL = 343.0; // meters per second

    /**
     * Standard acceleration of gravity at sea level in m/s^2
     */
    public static final double GRAVITY = 9.80665; // meters / sec^2

    /**
     * WGS84 value for the major/equatorial radii of Earth.
     */
    public static final double WGS84_MAJOR = 6378137.000000; // meters

    /**
     * WGS84 value for the minor/polar radius of Earth.
     */
    public static final double WGS84_MINOR = 6356752.314245; // meters

    /**
     * WGS84 volumetric mean radius of Earth, about 6371.00079 km.
     */
    public static final double WGS84_VMEAN = exp(log(WGS84_MAJOR * WGS84_MAJOR * WGS84_MINOR) / 3.0); // meters

    /**
     * WGS84 root mean square radius of Earth, about 6371.01675 km.
     */
    public static final double WGS84_RMS = sqrt(((2.0*WGS84_MAJOR*WGS84_MAJOR) + (WGS84_MINOR*WGS84_MINOR))/3.0); // meters

}


// =============================================================================
