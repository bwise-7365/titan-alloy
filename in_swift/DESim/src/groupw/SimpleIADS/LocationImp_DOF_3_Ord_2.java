/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.SimpleIADS;

import groupw.BaseSim.LocationIntrf;
import org.apache.commons.math4.legacy.linear.RealVector;
import static groupw.BaseSim.DSUtils.makeZeroRV;

/**
 *
 * @author BenWise Implements 3-DOF, second order dynamics. It would also be
 * possible to extend LocationImp_DOF_3_Ord_1, but I wanted to illustrate
 * completely unrelated implementations of the same (or extended) interface
 */
public class LocationImp_DOF_3_Ord_2 {

    public LocationImp_DOF_3_Ord_2(RealVector lp, RealVector lv, double lt) {
        setDim(lp.getDimension());
        setLastPosVelTime(lp, lv, lt);
    }

    public LocationImp_DOF_3_Ord_2(RealVector lp, double lt) {
        setDim(lp.getDimension()); // also set Pos, Vel, Acc, time to zero
        setLastPosTime(lp, lt);
    }

    public LocationImp_DOF_3_Ord_2(int n) {
        setDim(n); // also set Pos, Vel, Acc, time to zero
    }


    // support LocationIntrf_DOF_3_Ord_2 interface
    public final void setDim(int n) { // specify dimensions of vectors (2, 3 or 4)
        assert ((2 <= n) && (n <= 4));
        numDim = n;
        // set everything to zeros
        lastPos = makeZeroRV(numDim);
        lastVel = makeZeroRV(numDim);
        lastAcc = makeZeroRV(numDim);
        lastTime = 0.0;
    }

    public final int getDim() {
        return numDim;
    }

    public final RealVector currPos(double t) { // dead reckon the current position
        RealVector cp = lastPos;  // meters
        assert (lastTime <= t);
        double dt = t - lastTime; // seconds
        if (0.0 < dt) {
            // we avoid 'combineToSelf' which would update lastPos (!)
            cp = cp.combine(1.0, dt, lastVel);          // cp = 1.0 * cp + dt*lv
            cp = cp.combine(1.0, (dt * dt) / 2.0, lastAcc); // cp = 1.0 * cp + (dt*dt)/2 * la
        }
        return cp;
    }
    
    public final double getLastTime() {
        return lastTime;
    }

    public final RealVector currVel(double t) { // dead reckon the current velocity
        RealVector cv = lastVel;  // meters / second
        assert (lastTime <= t);
        double dt = t - lastTime; // seconds
        if (0.0 < dt) {
            // we avoid 'combineToSelf' which would update lastVel (!)
            cv = cv.combine(1.0, dt, lastAcc);  // cv = cv + dt*la
        }
        return cv;
    }

    public final double dist(LocationIntrf l2, double t) { // distance to a LocationIntrf thing
        RealVector p1 = this.currPos(t);
        RealVector p2 = l2.drCurrPos(t);
        double d = p1.getDistance(p2); // error if dimension mismatch
        return d;
    }

    public final double dist(RealVector p2, double t) { // distance to a point
        RealVector p1 = this.currPos(t);
        double d = p1.getDistance(p2); // error if dimension mismatch
        return d;
    }

    public final void setLastAccTime(RealVector av, double lt) {
        // we cannot use v.checkVectorDimensions(numDim)
        // because the method is protected.
        assert (numDim == av.getDimension());
        assert (0.0 <= lt); // we might sometimes back-date information
        lastAcc = av;
        lastTime = lt;

    }

    public final void setLastPosVelAccTime(RealVector pv, RealVector vv, RealVector av, double lt) {
        // we cannot use v.checkVectorDimensions(numDim)
        // because the method is protected.
        assert (numDim == pv.getDimension());
        assert (numDim == vv.getDimension());
        assert (numDim == av.getDimension());
        assert (0.0 <= lt); // we might sometimes back-date information
        lastPos = pv;
        lastVel = vv;
        lastAcc = av;
        lastTime = lt;

    }

    public final void setLastPosVelTime(RealVector pv, RealVector vv, double lt) {
        // we cannot use v.checkVectorDimensions(numDim)
        // because the method is protected.
        assert (numDim == pv.getDimension());
        assert (numDim == vv.getDimension());
        assert (0.0 <= lt); // we might sometimes back-date information
        lastPos = pv;
        lastVel = vv;
        lastTime = lt;

    }

    public final void setLastPosTime(RealVector pv, double lt) {
        // we cannot use v.checkVectorDimensions(numDim)
        // because the method is protected.
        assert (numDim == pv.getDimension());
        lastPos = pv;
        lastVel = makeZeroRV(numDim); 

        assert (0.0 <= lt); // we might sometimes back-date information
        lastTime = lt;

    }

    protected RealVector lastPos;
    protected RealVector lastVel;
    protected RealVector lastAcc;
    protected double lastTime;
    protected int numDim;
}


// =============================================================================
