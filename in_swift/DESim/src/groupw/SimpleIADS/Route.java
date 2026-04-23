/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */

package groupw.SimpleIADS;

import java.util.Random;
import java.util.ArrayList;

import groupw.BaseSim.DSUtils;
import org.apache.commons.math4.legacy.linear.RealVector;

/**
 * The Route class does not use GraphPath<V,E> because fractal paths will deviate from the edges of the graph
 * @author bwise
 */
public class Route {

    public Route(int n) {
        numDim = n;
        wps = new ArrayList<>(2); // reasonable minimum for a route
    }

    // prependPoint was never used, but definition is obvious

    /**
     * Add a point to the end of the Route, cost is O(1)
     *
     * @param p point to be added.
     */
    public void appendPoint(RealVector p) {
        assert (p.getDimension() == numDim);
        wps.add(p);
    }

    public int numPoints() {
        return wps.size();
    }

    public RealVector getWP(int n) {return wps.get(n);}

    /**
     * Useful for squeezing or stretching 3D routes into a reasonable vertical
     * range. Reasonable defaults when upper and lower limits coincide.
     *
     * @param zMin desired minimum Z coordinate, meters
     * @param zMax desired maximum Z coordinate, meters
     */
    public void rescaleZ(double zMin, double zMax) {
        assert (3 == numDim);
        assert (zMin <= zMax);
        double z0 = wps.get(0).getEntry(2);
        double z1 = wps.get(0).getEntry(2);
        int np = wps.size();
        if (zMin < zMax) {// This is not necessary if zMin == zMax
            for (RealVector wp : wps) { // get current min/max
                double z = wp.getEntry(2);
                if (z < z0) {
                    z0 = z;
                } else if (z1 < z) {
                    z1 = z;
                }
            }
            if (z0 < z1) {
                double a = (zMax - zMin) / (z1 - z0);
                double b = ((zMin * z1) - (zMax * z0)) / (z1 - z0);

                for (int k = 0; k < np; k++) {
                    double z = wps.get(k).getEntry(2);
                    double z2 = (a * z) + b;
                    wps.get(k).setEntry(2, z2);
                }
            } else { // current min == current max
                double zMid = (zMin + zMax) / 2.0;
                for (int k = 0; k < np; k++) {
                    wps.get(k).setEntry(2, zMid);
                }
            }
        } else { // zMin and zMax are equal
            for (int k = 0; k < np; k++) {
                wps.get(k).setEntry(2, zMin);
            }
        }
    }

    public ArrayList<RealVector> wps;
    public int numDim;

    /**
     * Returns a new route which is all of the first route and all of the second
     * route.
     *
     * @param r1 first route to be copied completely
     * @param r2 second route to be copied completely
     * @return merged route
     */
    static public Route merge(Route r1, Route r2) {
        int nd = r1.numDim;
        assert (r2.numDim == nd);
        int n1 = r1.numPoints();
        int n2 = r2.numPoints();
        Route r3 = new Route(nd);
        for (int i = 0; i < n1; i++) {
            r3.appendPoint(r1.wps.get(i));
        }
        for (int j = 0; j < n2; j++) {
            r3.appendPoint(r2.wps.get(j));
        }
        return r3;
    }

    /**
     * Returns a route with all but the first waypoint
     *
     * @param r1 Route to be shortened
     * @return New route, but with the first waypoint droppd
     */
    static public Route tail(Route r1) {
        Route r3 = new Route(r1.numDim);
        int np = r1.numPoints();
        for (int j = 1; j < np; j++) {
            r3.appendPoint(r1.wps.get(j));
        }
        return r3;
    }

    /**
     * Given two points, recursively add points between them to get a fractal
     * Route
     *
     * @param frac Multiplier to control amount of variation
     * @param dThresh Minimum length of a segment to get subdivided
     * @param pa first point
     * @param pb second point
     * @param prng pseudo-random number generator
     * @return route of waypoints
     */
    static public Route fractalSegment(
            double frac, double dThresh,
            RealVector pa, RealVector pb,
            Random prng) {
        //System.out.printf("Starting fractalSegment \n");
        //showVector(pa, " %8.2f");
        //showVector(pb, " %8.2f");
        //System.out.flush();
        int na = pa.getDimension();
        int nb = pb.getDimension();
        assert (0.0 <= frac);
        assert (na == nb);
        Route r3 = new Route(na);
        double d = pa.getDistance(pb);
        if (dThresh < d) {
            // Get the center point of the segment
            RealVector pc = pa.add(pb);
            pc = pc.mapMultiply(0.5);
            if (0.0 < frac) { // avoid zero-variation work
                // Get the proportional random variation
                RealVector var = DSUtils.makeUnitBoxRandomRV(na, prng);
                var = var.mapMultiply(frac * d);
                pc = pc.add(var);
            }
            Route r1 = fractalSegment(frac, dThresh, pa, pc, prng);
            Route r2 = fractalSegment(frac, dThresh, pc, pb, prng);
            r3 = merge(r1, tail(r2)); // avoid duplicating pc
        } else {
            r3.appendPoint(pa);
            r3.appendPoint(pb);
        }
        return r3;
    }

    /**
     * Given a route, iteratively refine each segment
     *
     * @param frac Multiplier to control amount of variation
     * @param dThresh Minimum length of a segment to get subdivided
     * @param r0 route to be refined
     * @param prng pseudo-random number generator
     * @return route of waypoints
     */
    static public Route fractalRoute(
            double frac, double dThresh,
            Route r0,
            Random prng) {
        assert (2 <= r0.numPoints());
        // Suppose r0 = [ p0, p1, p2, p3, p4 ... pk]
        // r1 is [ p0, ... p1 ]
        Route r1 = fractalSegment(frac, dThresh,
                r0.wps.get(0), r0.wps.get(1),
                prng);
        if (2 == r0.numPoints()) {
            return r1;
        } else {
            // r2 = [ p1, x, ... pk ]
            Route r2 = fractalRoute(frac, dThresh,
                    tail(r0), prng);
            // tail(r2) = [ x, ... pk ]
            // so r3 = [p0, ... p1, x, ... pk ]
            return merge(r1, tail(r2));
        }

    }

    /**
     * Make a Route which starts at p, orbits p1->p2->p3->p4 repeatedly, then
     * returns to p0. Enters the orbit via p0->p1 and leaves via p4->p0. Random
     * variation at corners is only the (X,Y) plane with no random variation in
     * Z coordinate. Fractal noise is added to segments, then the Z coordinate
     * is rescaled.
     *
     * @param p0 start/end point of route
     * @param p1 1st point of orbit
     * @param p2 2nd point of orbit
     * @param p3 3rd point of orbit
     * @param p4 4th point of orbit
     * @param numLoops how many times to perform the orbit
     * @param frac Multiplier to control amount of variation
     * @param dThresh Minimum length of a segment to get subdivided
     * @param cr radius of horizontal random variations at corners
     * @param zMin minimum Z coordinate of route
     * @param zMax maximum Z coordinate of route
     * @param prng pseudo random number generator
     * @return
     */
    static public Route fractalOrbit(
            RealVector p0,
            RealVector p1, RealVector p2, RealVector p3, RealVector p4,
            int numLoops,
            double frac, double dThresh,
            double cr,
            double zMin, double zMax,
            Random prng) {
        int numDim = p0.getDimension();
        assert (3 == numDim);
        Route rt1 = new Route(numDim);
        rt1.appendPoint(p0);
        RealVector offset;
        double rf = 2.0 * cr; // rescale [-0.5, +0.5] to [-cr, +cr]
        for (int k = 0; k < numLoops; k++) {
            perturbAndFlatten(p1, p2, prng, numDim, rt1, rf);
            perturbAndFlatten(p3, p4, prng, numDim, rt1, rf);
        }
        rt1.appendPoint(p0);

        Route rt2 = fractalRoute(frac, dThresh, rt1, prng);
        rt2.rescaleZ(zMin, zMax);
        return rt2;
    }

    private static void perturbAndFlatten(RealVector p1, RealVector p2, Random prng, int numDim, Route rt1, double rf) {
        RealVector offset;
        offset = DSUtils.makeUnitBoxRandomRV(numDim, prng);
        offset = offset.mapMultiply(rf);
        offset.setEntry(2, 0.0);
        RealVector q1 = p1.add(offset);
        rt1.appendPoint(q1);

        offset = DSUtils.makeUnitBoxRandomRV(numDim, prng);
        offset = offset.mapMultiply(rf);
        offset.setEntry(2, 0.0);
        RealVector q2 = p2.add(offset);
        rt1.appendPoint(q2);
    }
}
// =============================================================================
