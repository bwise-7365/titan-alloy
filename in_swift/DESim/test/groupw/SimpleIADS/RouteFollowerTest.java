/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package groupw.SimpleIADS;

import static groupw.Network.NWUtils.DefaultSeedPRNG;

import groupw.BaseSim.Scheduler;
import static groupw.SimpleIADS.SimpleAirFrame.makeSimpleFWA;
import static groupw.BaseSim.DSUtils.makeZeroRV;
import static groupw.Network.NWUtils.ReportingLevel.High;

import org.apache.commons.math4.legacy.linear.RealVector;
import org.junit.Test;

/**
 *
 * @author BenWise
 */
public class RouteFollowerTest {

    public RouteFollowerTest() {
    }

    /**
     * Run a simple air frame along a simple 3-dim Route in DESim
     */
    @Test
    public void RFTest02() {
        System.out.println("\n Starting RFLTest02\n");
        int numDim = 3;
        int sd0 = DefaultSeedPRNG;
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.schedRLevel = High;
        //sim.rLevel = Silent;
        sim.setPRNG(sd0);

        // no DamageServer
        RealVector p0 = makeZeroRV(numDim);
        RealVector v0 = makeZeroRV(numDim);

        RealVector wp0 = makeZeroRV(numDim);
        wp0.setEntry(0, 5000.0);
        wp0.setEntry(1, 5000.0);
        wp0.setEntry(2, 8000.0);

        RealVector wp1 = makeZeroRV(numDim);
        wp1.setEntry(0, 10000.0);
        wp1.setEntry(1, 0.0);
        wp1.setEntry(2, 6000.0);

        RealVector wp2 = makeZeroRV(numDim);
        wp2.setEntry(0, 15000.0);
        wp2.setEntry(1, 5000.0);
        wp2.setEntry(2, 8000.0);

        RealVector wp3 = makeZeroRV(numDim);
        wp3.setEntry(0, 20000.0);
        wp3.setEntry(1, 0.0);
        wp3.setEntry(2, 6000.0);

        Route routeWPS = new Route(numDim);
        routeWPS.appendPoint(wp0);
        routeWPS.appendPoint(wp1);
        routeWPS.appendPoint(wp2);
        routeWPS.appendPoint(wp3);

        double speed = 100.0; // 100 m/sec ~~ 223.7 miles/hr
        double dtMax = 5.0; // seconds
        double timeStep = dtMax;
        double distTol = 1.00; // meters

        RouteFollower rf0 = new RouteFollower(speed, distTol, routeWPS);
        rf0.rl = High;
        rf0.setDynamics(new LocationImp_DOF_3_Ord_1(numDim));

        //System.out.printf("a LT: %.4f , CT:  %.4f \n", rf0.getLastTime(), rf0.currTime);
        rf0.currTime = 0.0;
        rf0.setLastPosVelTime(p0, v0, rf0.currTime);

        String fwaType = EntityData.fwaRed01;
        SimpleAirFrame sf1 = makeSimpleFWA(fwaType, numDim, sim);
        sf1.dtMax = 2.625; // just for testing
        sf1.addRouteFollower(rf0);

        //System.out.printf("b LT: %.4f , CT:  %.4f \n", rf0.getLastTime(), rf0.currTime);
        //rf0.step(); // set desired course even at time 0.0
        sf1.process(); // set desired course even at time 0.0

        double cSpeed = rf0.drCurrVel(rf0.currTime).getNorm();
      
        // I happen to know that it completes
        // the route in about 327 seconds
        sim.run(350);

    }

}

// =============================================================================
