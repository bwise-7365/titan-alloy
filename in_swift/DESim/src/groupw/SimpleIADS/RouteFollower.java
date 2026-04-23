/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.SimpleIADS;

import static groupw.SimpleIADS.InterceptGuidance.interceptCourse;
import static groupw.BaseSim.DSUtils.makeZeroRV;
import static java.lang.Math.abs;

import groupw.BaseSim.*;
import org.apache.commons.math4.legacy.linear.RealVector;

/**
 * Guide a Hull along a route using PFSM to update course and schedule events
 *
 * @author BenWise
 */
public class RouteFollower
        extends PFSM
        implements LocationIntrf {

    public RouteFollower(double s, double dTol, Route wps) {
        super();
        assert (0.0 < s); // if moving, need positive speed
        speed = s;
        assert (0.0 < dTol);
        wpDistTol = dTol;
        int numWP = wps.numPoints();
        assert (0 < numWP); // at least head to one point
        wayPoints = wps;
        currWP = 0; // initially aim at first

        // Initially aim at the first waypoint
        State s0 = WPState.makeTransit("InitialApproach", 0, this);
        addState(s0);

        // Then aim at the second waypoint, but
        // NB: the waypoint will be reset multiple times as the WP index advances.
        // TODO: Make this iterate over 0 ... N-2 building and adding states
        State s1 = WPState.makeTransit("TransitLeg", 1, this);
        addState(s1);

        State s2 = WPState.makeStop("Stop", numWP - 1, this);
        addState(s2);

        WPTest t0 = WPTest.makeChange("ReachFirst", this);
        t0.wpNdx = 0;
        t0.dTol = dTol;
        s0.addTrans(t0, s1);

        WPTest t1a = WPTest.makeStop("ReachLast", this);
        t1a.wpNdx = numWP - 1; // this will never be updated
        t1a.dTol = dTol;
        s1.addTrans(t1a, s2); // go to end state

        WPTest t1b = WPTest.makeChange("ReachNext", this);
        t1b.wpNdx = 1; // this will be updated
        t1b.dTol = dTol;
        s1.addTrans(t1b, s1); // loop back to self

        // no test on final state, because we stop there
        setCurrState(s0);
    }

    public boolean step(double dtMax) {
        boolean r = super.step(); // Do the pure pFSM part
        Scheduler mhms = myHull.getSim();
        double tNow = mhms.getCurrTime();
        currTime = tNow;
        updatePV(tNow);
        double currDist = drDist(wayPoints.getWP(currWP), tNow);
        System.out.printf("%s At distance %8.2f from waypoint %2d\n",
                    mhms.timeStamp(), currDist, currWP);
        double dt = DSUtils.recMoverTimeStep(currDist,
                speed,
                myHull.dtMax);
        myHull.showPVDT(dt);
        EntEvent e = new EntEvent(myHull, mhms, tNow + dt);
        mhms.addEvent(e);
        return r;
    }

    /**
     *
     */
    protected static class WPTest extends PFSM.Test {

        private WPTest(String n, PFSM f) {
            super(n, f);
            myCheck = null;
            myAction = null;
            wpNdx = -1;
            dTol = -1.0;
        }

        static public WPTest makeChange(String n, PFSM f) {
            WPTest wpt = new WPTest(n, f);
            wpt.myCheck = () -> {
                return wpt.checkChange();
            };
            wpt.myAction = () -> {
                wpt.actionChange();
            };
            return wpt;
        }

        static public WPTest makeStop(String n, PFSM f) {
            WPTest wpt = new WPTest(n, f);
            wpt.myCheck = () -> {
                return wpt.checkStop();
            };
            wpt.myAction = () -> {
                wpt.actionStop();
            };
            return wpt;
        }

        private boolean checkChange() {
            RouteFollower rf = (RouteFollower) (myPFSM);
            System.out.printf("Checking change in WPTest %2d %s \n", getID(), getName());
            System.out.printf("check currWP: %2d  and wpn: %2d \n", rf.currWP, wpNdx);
            // WP coords are in meters, time in seconds
            //RealVector wpi = rf.wayPoints.get(wpNdx);
            boolean rslt = rf.closeEnough(wpNdx, rf.currTime);
            return rslt;
        }

        private void actionChange() {
            RouteFollower rf = (RouteFollower) (myPFSM);
            // WP coords are in meters, time in seconds
            int numWP = rf.wayPoints.numPoints();
            int wpNDX = rf.currWP;
            if (wpNDX + 1 < numWP) { // won't run off end
                rf.currWP = wpNDX + 1; // advance to next, if not at end
                System.out.printf("Advanced rf and test WP ndx to %d\n", rf.currWP);
                wpNdx = rf.currWP;
            } else {
                System.out.printf("Kept WP ndx at %d\n", rf.currWP);
            }
        }

        private boolean checkStop() {
            RouteFollower rf = (RouteFollower) (myPFSM);
            System.out.printf("Checking stop in WPTest %2d %s \n", getID(), getName());
            System.out.printf("check currWP: %2d  and wpn: %2d \n", rf.currWP, wpNdx);
            // WP coords are in meters, time in seconds
            //RealVector wpi = rf.wayPoints.get(wpNdx);
            boolean rslt = rf.closeEnough(wpNdx, rf.currTime);
            return rslt;
        }

        private void actionStop() {
            RouteFollower rf = (RouteFollower) (myPFSM);
            rf.currWP = wpNdx; // keep at the end point
            System.out.printf("Advanced rf WP ndx to end %d\n", rf.currWP);
        }

        protected int wpNdx = -1;
        protected double dTol = -1.0;
    }

    protected static class WPState extends PFSM.State {

        private WPState(String n, int wpn, PFSM f) {
            super(n, f);
            setWP(wpn);
            myAction = null;
        }

        static public WPState makeTransit(String n, int wpn, PFSM f) {
            WPState wps = new WPState(n, wpn, f);
            wps.myAction = () -> {
                wps.actionTransit();
            };
            return wps;
        }

        static public WPState makeStop(String n, int wpn, PFSM f) {
            WPState wps = new WPState(n, wpn, f);
            wps.myAction = () -> {
                wps.actionStop();
            };
            return wps;
        }
        protected int wpNDX;

        public void setWP(int wpn) {
            System.out.printf("WPState %5d %s is setting waypoint index: %d\n",
                    getID(), getName(), wpn);
            wpNDX = wpn;
        }

        private void actionTransit() {
            RouteFollower rf = (RouteFollower) (myPFSM);
            int wpn = rf.currWP;
            if (wpn != wpNDX) {
                setWP(wpn);
            }
            System.out.printf("WPState %5d is setting course at time %.4f toward WP%02d: \n",
                    getID(), rf.currTime, wpNDX);
            DSUtils.showVector(rf.wayPoints.getWP(wpNDX), "  %9.2f");
            rf.setCourse(rf.wayPoints.getWP(wpNDX), rf.currTime);
            System.out.println("from current location:");
            RealVector cp = rf.drCurrPos(rf.currTime);
            DSUtils.showVector(cp, "  %9.2f");
        }

        private void actionStop() {
            RouteFollower rf = (RouteFollower) (myPFSM);
            int wpn = rf.currWP;
            if (wpn != wpNDX) {
                setWP(wpn);
            }
            System.out.printf("WPState %5d %s is stopping at time %.4f at WP%02d: \n",
                    getID(), getName(), rf.currTime, wpNDX);
            DSUtils.showVector(rf.wayPoints.getWP(wpNDX), "  %9.2f");
            rf.setLastPosTime(rf.wayPoints.getWP(wpNDX), rf.currTime);
        }
    }

    /**
     * Calculate new velocity then update position and velocity accordingly
     *
     * @param q Stationary waypoint at which we should aim
     * @param t Current simulation time
     */
    protected void setCourse(RealVector q, double t) {
        RealVector p = drCurrPos(t);  // extrapolate from last data
        int numDim = q.getDimension();
        RealVector w = makeZeroRV(numDim);  // waypoints are stationary
        double distTol = 1.0; // meters
        double timeTol = distTol / (4.0 * speed); // see interceptCourse Javadoc
        InterceptGuidance.InterceptResult ir = interceptCourse(p, speed,
                q, w,
                timeTol);
        RealVector v = ir.v;

        System.out.printf("setCourse: v in meters/sec at time %.2f \n", t);
        DSUtils.showVector(v, " %9.2f");

        setLastPosVelTime(p, v, t);
    }

    protected boolean closeEnough(int wpn, double t) {
        System.out.printf("closeEnough currWP: %2d  and wpn: %2d \n", currWP, wpn);
        RealVector p = wayPoints.getWP(wpn);
        RealVector cp = drCurrPos(t);
        double d0 = (cp.subtract(p)).getNorm();
        double d1 = cp.getDistance(p);
        double d2 = drDist(p, t); // dead-reckon distance to the point P at time T
        System.out.printf("At time %.3f, distance %.3f meters between POS and WP%02d \n",
                t, d2, wpn);
        System.out.println("POS:");
        DSUtils.showVector(cp, " %10.3f");
        System.out.printf("WP:%02d\n", wpn);
        DSUtils.showVector(p, " %10.3f");

        double vTol = 0.001; // max tolerable round-off error in m/s

        assert (abs(d0 - d1) < vTol);
        assert (abs(d1 - d2) < vTol);
        assert (abs(d2 - d0) < vTol);

        boolean rslt = (d2 < wpDistTol);
        if (rslt) {
            System.out.printf(" it is within wp-dist tolerance %.3f meters \n", wpDistTol);
        } else {
            System.out.printf(" it is not within wp-dist tolerance %.3f meters \n", wpDistTol);
        }
        return rslt;
    }

    /**
     * Implement simple straight-line dynamics: copy dead-reckoned position
     *
     * @param t Current simulation time
     */
    protected void updatePV(double t) {
        RealVector p = drCurrPos(t);  // extrapolate from last data
        RealVector v = drCurrVel(t);  // extrapolate from last data
        setLastPosVelTime(p, v, t);
    }

    protected double speed = 0.0; // current speed
    protected double wpDistTol = -1.0; // how close to a WP is close enough to turn to next
    protected int currWP = -1;
    protected double currTime = -1.0; // from DESim.Scheduler
    protected Route wayPoints;
    protected Hull myHull = null;

    // TODO: copy this boilerplate to a single location in code, e.g. Mover31
    // Dynamics to implement the Location interface
    private LocationImp_DOF_3_Ord_1 dynamics = null;

    public void setDynamics(LocationImp_DOF_3_Ord_1 d) {
        assert (null != d);
        this.dynamics = d;
        this.setDim(d.getDim());
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
        return dynamics.getLastTime();
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
