/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/UnitTests/JUnit4TestClass.java to edit this template
 */
package groupw.SimpleIADS;

import static groupw.Network.NWUtils.DefaultSeedPRNG;
import groupw.BaseSim.EntEvent;
import groupw.BaseSim.Scheduler;
import static groupw.SimpleIADS.SimpleAirFrame.makeSimpleFWA;
import static groupw.SimpleIADS.SimpleMissile.makeSimpleMissile;
import static groupw.BaseSim.DSUtils.KILOMETER;
import static groupw.BaseSim.DSUtils.makeZeroRV;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static groupw.Network.NWUtils.ReportingLevel.Medium;
import org.apache.commons.math4.legacy.linear.RealVector;
import org.junit.Test;

/**
 *
 * @author bwise
 */
public class SimpleMissileTest {

    public SimpleMissileTest() {
    }

    /**
     * Launch one missile at one non-maneuvering aircraft
     */
    @Test
    public void test00() {
        System.out.println("\nStarting SimpleMissileTest.test00");
        int numDim = 3;
        int sd0 = DefaultSeedPRNG;
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.rLevel = High;
        //sim.rLevel = Silent;
        sim.setPRNG(sd0);

        DamageServer.makeDS(sim);
        EntityData.parameterizeDamageServer();
        DamageServer.theDS.stochasticP = false;
        DamageServer.theDS.rLevel = High;
        DamageServer.theDS.rLevel = Medium;
        //DamageServer.theDS.rLevel = Silent;

        double tNow = sim.getCurrTime();

        // Create an aircraft 20Km North, up the Y axis,
        // cruising due East, along the X axis.
        String fwaType = EntityData.fwaRed01;
        SimpleAirFrame sf1 = makeSimpleFWA(fwaType, numDim, sim);
        sf1.dtMax = 2.625; // just for testing
        System.out.printf("AirFrame %4d min speed: %7.2f \n", sf1.getID(), sf1.speedMin);
        System.out.printf("AirFrame %4d crs speed: %7.2f \n", sf1.getID(), sf1.speedCrs);
        System.out.printf("AirFrame %4d max speed: %7.2f \n", sf1.getID(), sf1.speedMax);
        RealVector c2 = makeZeroRV(numDim);
        c2.setEntry(1, 20*KILOMETER);
        RealVector v2 = makeZeroRV(numDim);
        v2.setEntry(0, sf1.speedCrs);
        sf1.setLastPosVelTime(c2, v2, tNow);
        EntEvent e1 = new EntEvent(sf1, sim, tNow);
        sim.addEvent(e1);

        // create a missile at the origin
        String missileType = EntityData.a2aMissileBlue02;
        SimpleMissile sm1 = makeSimpleMissile(missileType, numDim, sim);
        sm1.dtMax = 1.870; // just for testing
        RealVector c1 = makeZeroRV(numDim);
        sm1.setLastPosTime(c1, tNow);
        sm1.target = sf1;
        EntEvent e2 = new EntEvent(sm1, sim, tNow);
        sim.addEvent(e2);

        // I happen to know that that missile has about 54 seconds of flight
        sim.run(60.0);

        DamageServer.clearDS();
        sim = null;
    }

}

// =============================================================================
