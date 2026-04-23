/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */

package groupw.SimpleIADS;

import groupw.BaseSim.*;

import static groupw.BaseSim.DSUtils.KILOMETER;
import static groupw.BaseSim.DSUtils.MACH_SEA_LEVEL;
import static groupw.BaseSim.DSUtils.makeUnitBoxRandomRV;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static groupw.Network.NWUtils.rLevelLE;

import org.apache.commons.math4.legacy.linear.RealVector;

/**
 * SimpleAirFrame represents (at low resolution) FWA and RWA. This differs from
 * SimpleMissile in that its speed is variable and is expected to usually
 * survive a flight.
 *
 * @author BenWise
 */
public class SimpleAirFrame
        extends Hull {

    private SimpleAirFrame(String tn, double rangeMax, double speed,
            Scheduler s) {
        super(tn, s);
        this.rangeMax = rangeMax;
        this.speed = speed;
    }

    public double rangeMax = 0.0; // one-way, unrefueled range, meters
    public double speed = 0.0;  // speed at this moment, meters / second
    public double speedMin = 0.0;  // minimum speed in level flight at typical altitude, meters / second
    public double speedCrs = 0.0;  // crusing speed in level flight at typical altitude, meters / second
    public double speedMax = 0.0;  // maximum speed in level flight at typical altitude, meters / second

    static public SimpleAirFrame makeSimpleFWA(String tn, int numDim, Scheduler sim) {
        double rng = 0.0;
        double spdMin = 0.0;
        double spdCrs = 0.0;
        double spdMax = 0.0;
        switch (tn) {
            case EntityData.fwaBlue01:
            case EntityData.fwaRed01:
                rng = 2000.0 * KILOMETER;
                spdCrs = 0.75 * MACH_SEA_LEVEL;
                break;
            case EntityData.fwaBlue02:
            case EntityData.fwaRed02:
                rng = 2200.0 * KILOMETER;
                spdCrs = 1.2 * MACH_SEA_LEVEL;
                break;
            default:
                System.out.printf("Unknown SimpleAirFrame type: %s\n", tn);
                break;
        }
        SimpleAirFrame sm = new SimpleAirFrame(tn, rng, spdCrs, sim);

        // These are notional numbers for FWA.
        // RWA would have min speed of zero to represent hovering.
        sm.speedCrs = spdCrs;
        sm.speedMin = spdCrs / 4.0;
        sm.speedMax = spdCrs * 1.5;
        sm.setDynamics(new LocationImp_DOF_3_Ord_1(numDim));
        return sm;
    }

    /**
     * Without a PFSM, SimpleAirFrame.process() continues in a line, with some
     * random wobble.
     */
    protected void wobble() {
        double tNow = mySim.getCurrTime();
        RealVector p0 = drCurrPos(tNow);
        RealVector v0 = drCurrVel(tNow);
        double dtElapsed = tNow - this.getLastTime();
        double f1 = 1.0; // control size of noise
        int numDim = this.dynamics.numDim;
        RealVector a = makeUnitBoxRandomRV(numDim, mySim.prng);
        a = a.mapMultiply(f1 * dtElapsed);
        RealVector v1 = v0.add(a);
        double f2 = this.speedCrs / v1.getNorm();
        v1 = v1.mapMultiply(f2);
        setLastPosVelTime(p0, v1, tNow);
        double dt = dtMax;
        showPVDT(dt);
        EntEvent e = new EntEvent(this, mySim, tNow + dt);
        mySim.addEvent(e);
    }

    public void addRouteFollower(RouteFollower rf) {
        assert (null != dynamics);
        rf.setDynamics(dynamics);
        myFSM = rf;
        rf.myHull = this;
    }

    @Override
    public void process() {
        if (rLevelLE(High, mySim.rLevel)) {
            System.out.println("");
            System.out.printf("%s SimpleAirframe.process() %4d \n",
                    mySim.timeStamp(), getID());
        }
        if (null == myFSM) {
            wobble();
        } else {
            double tNow = mySim.getCurrTime();
            RouteFollower rf0 = (RouteFollower) myFSM;
            rf0.step(dtMax); // reset desired course
        }
    }

    // TODO: copy this boilerplate to a single location in code, e.g. Mover31
    // Dynamics to implement the Location interface
    private LocationImp_DOF_3_Ord_1 dynamics = null;

    public void setDynamics(LocationImp_DOF_3_Ord_1 d) {
        assert (null != d);
        this.dynamics = d;
    }

    @Override
    public void setDim(int n) {
        assert (null != dynamics);
        dynamics.setDim(n);
    }

    @Override
    public int getDim() {
        return dynamics.getDim();
    }

    @Override
    public RealVector drCurrPos(double t) {
        assert (null != dynamics);
        return dynamics.currPos(t);
    }

    @Override
    public double getLastTime() {
        return dynamics.lastTime;
    }

    @Override
    public RealVector drCurrVel(double t) {
        assert (null != dynamics);
        return dynamics.currVel(t);
    }

    @Override
    public double drDist(LocationIntrf l2, double t) {
        assert (null != dynamics);
        return dynamics.dist(l2, t);
    }

    @Override
    public double drDist(RealVector p, double t) {
        assert (null != dynamics);
        return dynamics.dist(p, t);
    }

    @Override
    public void setLastPosVelTime(RealVector pv, RealVector vv, double lt) {
        assert (null != dynamics);
        dynamics.setLastPosVelTime(pv, vv, lt);
    }

    @Override
    public void setLastPosTime(RealVector pv, double lt) {
        assert (null != dynamics);
        dynamics.setLastPosTime(pv, lt);
    }

}


// =============================================================================
