/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/UnitTests/JUnit4TestClass.java to edit this template
 */
package groupw.SimpleIADS;

import static groupw.Network.NWUtils.DefaultSeedPRNG;
import groupw.BaseSim.Scheduler;
import static groupw.SimpleIADS.SimpleAirFrame.makeSimpleFWA;
import static groupw.SimpleIADS.SimpleRadar.RadarCrossSectionMap;
import static groupw.BaseSim.DSUtils.KILOMETER;
import static groupw.BaseSim.DSUtils.makeZeroRV;
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
public class SimpleRadarTest {

    public SimpleRadarTest() {
    }

    /**
     * Test of probDetect method, of class SimpleRadar. The expected ratios are
     * algebraic identities based on SimpleRadar.probDetect
     */
    @Test
    public void testProbDetect01() {
        System.out.println("\n\nStarting SimpleRadarTest.testProbDetect01");
        int numDim = 3;
        int sd0 = DefaultSeedPRNG;
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.setPRNG(sd0);
        EntityData.parameterizeRCSMap();

        int numTests = 50;

        for (int i = 0; i < numTests; i++) {
            SimpleRadar sr1 = new SimpleRadar("genericRadar", sim);

            // Note that half will be above 1m^2 and half below.
            double vMin = log(0.01);
            double vMax = log(100.0);

            sr1.rcs50 = exp(sim.uniform(vMin, vMax)); // meter ^ 2
            sr1.range50 = sim.uniform(50.0, 200.0) * KILOMETER;

            System.out.printf("%3d/%3d RCS50 = %7.3f , Range50 = %9.2f ",
                    i + 1, numTests, sr1.rcs50, sr1.range50);

            double dfltRCS = sr1.rcs50;
            double dfltRange = sr1.range50;
            double tol = 1e-4; // range0 has an effect

            double p0 = sr1.probDetect(dfltRCS, dfltRange);
            double err0 = abs(p0 - (1.0 / 2.0));
            assertTrue(err0 < tol);
            System.out.printf(".");

            double p5 = sr1.probDetect(dfltRCS, 0.5 * dfltRange);
            double err5 = abs(p5 - (80.0 / 85.0));
            assertTrue(err5 < tol);
            System.out.printf(".");

            double p1 = sr1.probDetect(dfltRCS, 2 * dfltRange);
            double err1 = abs(p1 - (1.0 / 17.0));
            assertTrue(err1 < tol);
            System.out.printf(".");

            double p3 = sr1.probDetect(dfltRCS, 3 * dfltRange);
            double err3 = abs(p3 - (1.0 / 82.0));
            assertTrue(err3 < tol);
            System.out.printf(".");

            double p6 = sr1.probDetect(0.5 * dfltRCS, dfltRange);
            double err6 = abs(p6 - (1.0 / 3.0));
            assertTrue(err6 < tol);
            System.out.printf(".");

            double p2 = sr1.probDetect(2 * dfltRCS, dfltRange);
            double err2 = abs(p2 - (2.0 / 3.0));
            assertTrue(err2 < tol);
            System.out.printf(".");

            double p4 = sr1.probDetect(3 * dfltRCS, dfltRange);
            double err4 = abs(p4 - (3.0 / 4.0));
            assertTrue(err4 < tol);
            System.out.printf(".");

            System.out.println("");
        }
    }

    /**
     * Test of detect method, of class SimpleRadar against an FWA. We create a bunch of FWA at
     * various distances - including Zero - and verify that it detects the correct number of them.
     */
    @Test
    public void testDetectt01() {
        System.out.println("Starting SimpleRadarTest.testDetectt01");
        int sd0 = 0;
        // These number do not depend on the PRNG
        int nrd1 = testDetect(EntityData.fwaBlue01, false, sd0);
        System.out.printf("Deterministic hits: %2d \n", nrd1);
        assertEquals(25, nrd1);
    }
    @Test
    public void testDetectt02() {
        System.out.println("Starting SimpleRadarTest.testDetectt02");
        int sd0 = 0;
        // These number do not depend on the PRNG
        int nrd2 = testDetect(EntityData.fwaBlue02, false, sd0);
        System.out.printf("Deterministic hits: %2d \n", nrd2);
        assertEquals(21, nrd2);
    }
    @Test
    public void testDetectt03() {
        System.out.println("Starting SimpleRadarTest.testDetectt03");
        int sd0 = DefaultSeedPRNG;
        // These numbers depend on the particular PRNG seed
        int nrs1 = testDetect(EntityData.fwaRed01, true, sd0);
        System.out.printf("Stochastic hits: %2d \n", nrs1);
        assertEquals(25, nrs1);
    }
    @Test
    public void testDetectt04() {
        System.out.println("Starting SimpleRadarTest.testDetectt04");
        int sd0 = DefaultSeedPRNG;
        
        // These numbers depend on the particular PRNG seed
        int nrs2 = testDetect(EntityData.fwaRed02, true, sd0);
        System.out.printf("Stochastic hits: %2d \n", nrs2);
        assertEquals(20, nrs2);
    }

    public int testDetect(String fwaType, boolean sp, int sd0) {
        System.out.printf("Starting SimpleRadarTest.testDetect with %s \n",
                fwaType);
        int numDim = 3;
        Scheduler sim = new Scheduler();
        sim.setPRNG(sd0);
        EntityData.parameterizeRCSMap();

        double tNow = sim.getCurrTime();
        RealVector origin = makeZeroRV(numDim);
        RealVector step = makeZeroRV(numDim); // slightly less than 5Km
        step.setEntry(0, 2886.5);
        step.setEntry(1, 2886.5);
        step.setEntry(2, 2886.5);
        SimpleRadar sr1 = new SimpleRadar("genericRadar", sim);
        sr1.stochasticP = sp;
        int numFWA = 50;
        int numHits = 0;
        RealVector c2 = origin;

        for (int i = 0; i < numFWA; i++) {
            SimpleAirFrame sf1 = makeSimpleFWA(fwaType, numDim, sim);
            sf1.setLastPosTime(c2, tNow);

            double rcs = RadarCrossSectionMap.get(sf1.getTypeName());
            double range = origin.getDistance(c2);
            double pd = sr1.probDetect(rcs, range);
            //System.out.printf("At range %10.2f, rcs %.2f, pd = %.6f ", range, rcs, pd);
            boolean h = sr1.detect(rcs, range);
            if (h) {
                //System.out.println("hit");
                numHits++;
            } else {
                //System.out.println("miss");
            }

            c2 = c2.add(step);
        }
        sim = null;
        return numHits;
    }
 
}

// =============================================================================
