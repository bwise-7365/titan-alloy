/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/UnitTests/JUnit4TestClass.java to edit this template
 */
package groupw.SimpleIADS;

import static groupw.Network.NWUtils.DefaultSeedPRNG;

import static groupw.BaseSim.DSUtils.KILOMETER;
import static groupw.BaseSim.DSUtils.makeZeroRV;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static java.lang.Math.abs;

import groupw.BaseSim.Scheduler;
import org.apache.commons.math4.legacy.linear.RealVector;
import org.junit.Test;
import static org.junit.Assert.*;

/**
 *
 * @author bwise
 */
public class RouteTest {

    public RouteTest() {
    }

    @Test
    public void test00() {
        System.out.println("\nStarting test00");
        int numDim = 2;
        int sd0 = DefaultSeedPRNG;
        double dTol = 0.001; // meters
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.schedRLevel = High;
        //sim.rLevel = Silent;
        sim.setPRNG(sd0);

        RealVector p1 = makeZeroRV(numDim);
        RealVector p5 = makeZeroRV(numDim);
        p5.setEntry(0, 10.0 * KILOMETER);
        p5.setEntry(1, 10.0 * KILOMETER);

        // with a 14 Kilometer separation,
        // the 10 Km threshold should cause one split
        Route rt0 = Route.fractalSegment(0.0,
                10.0 * KILOMETER,
                p1, p5,
                sim.prng);

        int n0 = rt0.numPoints();
        System.out.printf("NumPoints %d\n", n0);
        assertEquals(3, (int) n0);

        RealVector p3 = rt0.wps.get(1);

        RealVector t = p5.mapMultiply(0.5);
        double d = t.getDistance(p3);

        assertTrue(d < dTol);

    }

    @Test
    public void test01() {
        System.out.println("\nStarting test01");
        int numDim = 2;
        int sd0 = 27185305;
        double dTol = 0.001; // meters
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.schedRLevel = High;
        //sim.rLevel = Silent;
        sim.setPRNG(sd0);

        RealVector p1 = makeZeroRV(numDim);
        RealVector p5 = makeZeroRV(numDim);
        p5.setEntry(0, 10.0 * KILOMETER);
        p5.setEntry(1, 10.0 * KILOMETER);

        // with a 14 Kilometer separation,
        // the 5 Km threshold should cause
        // two levels of binary splits,
        // adding three intermediate points
        Route rt1 = Route.fractalSegment(0.0,
                5.0 * KILOMETER,
                p1, p5,
                sim.prng);

        int n1 = rt1.numPoints();
        System.out.printf("NumPoints %d\n", n1);
        assertEquals(5, (int) n1);

        for (int i = 0; i < 5; i++) {
            RealVector p = rt1.wps.get(i);
            RealVector t = p5.mapMultiply(i / 4.0);
            assertTrue (t.getDistance(p) < dTol);
        }

    }

    @Test
    public void test02() {
        System.out.println("\nStarting test02");
        int numDim = 2;
        int sd0 = DefaultSeedPRNG;
        double dTol = 0.001; // meters
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.schedRLevel = High;
        //sim.rLevel = Silent;
        sim.setPRNG(sd0);

        RealVector p1 = makeZeroRV(numDim);

        RealVector p2 = makeZeroRV(numDim);
        p2.setEntry(0, 10.0 * KILOMETER);
        p2.setEntry(1, 10.0 * KILOMETER);

        RealVector p3 = makeZeroRV(numDim);
        p3.setEntry(0, 20.0 * KILOMETER);
        p3.setEntry(1, 20.0 * KILOMETER);

        Route rt0 = new Route(numDim);
        rt0.appendPoint(p1);
        rt0.appendPoint(p2);
        rt0.appendPoint(p3);

        // with a 14 Kilometer separation,
        // the 5 Km threshold should cause one split
        // and produce exactly nine points
        Route rt1 = Route.fractalRoute(0.0,
                5.0 * KILOMETER,
                rt0,
                sim.prng);
        assertEquals(9, (int) rt1.numPoints());

        // with a 14 Kilometer separation,
        // the 1 Km threshold should produce
        // exactly 33 points
        Route rt2 = Route.fractalRoute(0.0,
                KILOMETER,
                rt0,
                sim.prng);
        int np2 = rt2.numPoints();
        System.out.printf("Route2 length %d \n", np2);
        assertEquals(33, np2);

        // The random motion to the side makes longer
        // segments, so we end up with more segments.
        // Results depend on the PRNG seed, but this one
        // should be 42.
        Route rt3 = Route.fractalRoute(0.2,
                KILOMETER,
                rt0,
                sim.prng);
        int np3 = rt3.numPoints();
        System.out.printf("Route3 length %d \n", np3);
        assertEquals(42, np3);
    }

    protected Route make3DRoute() {
        int numDim = 3;
        RealVector p1 = makeZeroRV(numDim);
        p1.setEntry(2, 2.0 * KILOMETER);

        RealVector p2 = makeZeroRV(numDim);
        p2.setEntry(0, 10.0 * KILOMETER);
        p2.setEntry(1, 10.0 * KILOMETER);
        p2.setEntry(2, 2.0 * KILOMETER);

        RealVector p3 = makeZeroRV(numDim);
        p3.setEntry(0, 20.0 * KILOMETER);
        p3.setEntry(1, 20.0 * KILOMETER);
        p3.setEntry(2, 2.0 * KILOMETER);

        Route rt0 = new Route(numDim);
        rt0.appendPoint(p1);
        rt0.appendPoint(p2);
        rt0.appendPoint(p3);
        return rt0;
    }

    protected double routeZMin(Route rt0) {
        double z0 = rt0.wps.get(0).getEntry(2);
        int np = rt0.wps.size();
        for (int k = 0; k < np; k++) { // get current min/max
            double z = rt0.wps.get(k).getEntry(2);
            if (z < z0) {
                z0 = z;
            }
        }
        return z0;
    }

    protected double routeZMax(Route rt0) {
        double z1 = rt0.wps.get(0).getEntry(2);
        int np = rt0.wps.size();
        for (int k = 0; k < np; k++) { // get current min/max
            double z = rt0.wps.get(k).getEntry(2);
            if (z1 < z) {
                z1 = z;
            }
        }
        return z1;
    }

    @Test
    public void test03a() {
        System.out.println("\nStarting test03a");
        int sd0 = 53052718;
        double dTol = 0.001; // meters
        double zMin = 0.0;
        double zMax = 0.0;
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.schedRLevel = High;
        //sim.rLevel = Silent;
        sim.setPRNG(sd0);

        Route rt0 = make3DRoute();
        zMin = routeZMin(rt0);
        zMax = routeZMax(rt0);
        assertTrue(abs(2.0 * KILOMETER - zMin) < dTol);
        assertTrue(abs(2.0 * KILOMETER - zMax) < dTol);

        // The random motion to the side makes longer
        // segments, so we end up with more segments.
        // Results depend on the PRNG seed, but this one
        // should be 47.
        Route rt3 = Route.fractalRoute(0.5,
                KILOMETER,
                rt0,
                sim.prng);

        zMin = routeZMin(rt3);
        zMax = routeZMax(rt3);
        int np3 = rt3.numPoints();
        System.out.printf("Route3a length %d \n", np3);
        System.out.printf("Route z min, max: %9.3f %9.3f\n",
                zMin, zMax);
        // I happen to know the exact upper and lower
        // Z coordinates of the fractal path, given those parameters
        assertTrue(abs( 449.183 - zMin) < dTol);
        assertTrue(abs(4232.992 - zMax) < dTol);

        rt3.rescaleZ(5000, 6000);
        zMin = routeZMin(rt3);
        zMax = routeZMax(rt3);
        assertTrue(abs(5000.0 - zMin) < dTol);
        assertTrue(abs(6000.0 - zMax) < dTol);

        rt3.rescaleZ(7000, 7000);
        zMin = routeZMin(rt3);
        zMax = routeZMax(rt3);
        assertTrue(abs(7000.0 - zMin) < dTol);
        assertTrue(abs(7000.0 - zMax) < dTol);

        rt3.rescaleZ(5000, 6000);
        zMin = routeZMin(rt3);
        zMax = routeZMax(rt3);
        assertTrue(abs(5500.0 - zMin) < dTol);
        assertTrue(abs(5500.0 - zMax) < dTol);
    }

    @Test
    public void test04() {
        System.out.println("\nStarting test04");
        int sd0 = 27185305;
        double dTol = 0.001; // meters
        double zMin = 0.0;
        double zMax = 0.0;
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.schedRLevel = High;
        //sim.rLevel = Silent;
        sim.setPRNG(sd0);

        int numDim = 3;
        RealVector p0 = makeZeroRV(numDim);
        p0.setEntry(0, 50.0 * KILOMETER);

        RealVector p1 = makeZeroRV(numDim);
        p1.setEntry(0, 0.0);
        p1.setEntry(1, 100.0 * KILOMETER);
        p1.setEntry(2, 8.0 * KILOMETER);

        RealVector p2 = makeZeroRV(numDim);
        p2.setEntry(0, 0.0);
        p2.setEntry(1, 120.0 * KILOMETER);
        p2.setEntry(2, 8.0 * KILOMETER);

        RealVector p3 = makeZeroRV(numDim);
        p3.setEntry(0, 100.0 * KILOMETER);
        p3.setEntry(1, 120.0 * KILOMETER);
        p3.setEntry(2, 8.0 * KILOMETER);

        RealVector p4 = makeZeroRV(numDim);
        p4.setEntry(0, 100.0 * KILOMETER);
        p4.setEntry(1, 100.0 * KILOMETER);
        p4.setEntry(2, 8.0 * KILOMETER);

        // with such a large dThresh, it will not 'refine' at all.
        Route rt1 = Route.fractalOrbit(p0,
                p1, p2, p3, p4,
                1,
                0.0, 200.0 * KILOMETER,
                0.0 * KILOMETER,
                0.0, 10.0 * KILOMETER,
                sim.prng);
        int np1 = rt1.numPoints();
        System.out.printf("Number of rt1 points: %4d\n", np1);
        assertEquals(6, np1);

        // 10 Km dThresh drives much refinement
        Route rt2 = Route.fractalOrbit(p0,
                p1, p2, p3, p4,
                1,
                0.0, 10.0 * KILOMETER,
                0.0 * KILOMETER,
                0.0, 10.0 * KILOMETER,
                sim.prng);
        int np2 = rt2.numPoints();
        System.out.printf("Number of rt2 points: %4d\n", np2);
        assertEquals(53, np2);

        Route rt3 = Route.fractalOrbit(p0,
                p1, p2, p3, p4,
                5,
                0.0, 10.0 * KILOMETER,
                0.0 * KILOMETER,
                0.0, 10.0 * KILOMETER,
                sim.prng);
        int np3 = rt3.numPoints();
        System.out.printf("Number of rt3 points: %4d\n", np3);
        assertEquals(197, np3);
        System.out.printf("Z rt3 min, max: %8.2f, %8.2f\n",
                routeZMin(rt3), routeZMax(rt3));

        // frac = 0.05 means that the mid-point of the 100Km
        // edges could be displaced by +/- 5 Km in each of the three dimensions.
        Route rt4 = Route.fractalOrbit(p0,
                p1, p2, p3, p4,
                5,
                0.05, 10.0 * KILOMETER,
                0.0 * KILOMETER,
                1.0, 10.0 * KILOMETER,
                sim.prng);
        int np4 = rt4.numPoints();
        System.out.printf("Number of rt4 points: %4d\n", np4);
        assertEquals(208, np4);
        System.out.printf("Z rt4 min, max: %8.2f, %8.2f\n",
                routeZMin(rt4), routeZMax(rt4));
    }

}
// =============================================================================
