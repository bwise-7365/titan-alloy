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
 * @author BenWise
 */
public class LocationImp_DOF_3_Ord_1 {

    public LocationImp_DOF_3_Ord_1(RealVector lp, RealVector lv, double lt) {
        setDim(lp.getDimension());
        setLastPosVelTime(lp, lv, lt);
    }

    public LocationImp_DOF_3_Ord_1(RealVector lp, double lt) {
        setDim(lp.getDimension()); // also set Pos, Vel, time to zero
        setLastPosTime(lp, lt);
    }

    public LocationImp_DOF_3_Ord_1(int n) {
        setDim(n); // also set Pos, Vel, time to zero
    }

    // To support LocationIntrf, provide bodies for the required functions.
    /*

    public void setDim(int n);           // specify dimensions of vectors (2 or 3)
    public int getDim();
    public RealVector drCurrPos(double t); // dead reckon the current position
    public RealVector drCurrVel(double t); // dead reckon the current velocity
    public double drDist(LocationIntrf l2, double t); // distance to a LocationIntrf thing

    public void setLastPosVelTime(RealVector pv, RealVector vv, double lt);
    public void setLastPosTime(RealVector pv, double lt);
     */
    // support LocationIntrf interface
    public final void setDim(int n) {  // specify dimensions of vectors (2, 3 or 4)
        assert ((2 <= n) && (n <= 4));
        numDim = n;
        // set everything to zeros
        lastPos = makeZeroRV(numDim);
        lastVel = makeZeroRV(numDim);
        lastTime = 0.0;
    }

    public final int getDim() {
        return numDim;
    }

    public final RealVector currPos(double t) { // dead reckon the current position
        RealVector cp = lastPos;  // meters
        if (lastTime > t){
            System.out.printf("Invalid times: %.4f and %.4f \n",
                    lastTime, t);
            System.out.flush();
            System.out.flush();
        }
        assert (lastTime <= t);
        double dt = t - lastTime; // seconds
        if (0.0 < dt) {
            // we avoid 'combineToSelf' which would update lastPos (!)
            cp = cp.combine(1.0, dt, lastVel); // cp = 1.0 * cp + dt*lv
        }
        return cp;
    }

    public final double getLastTime() {
        return lastTime;
    }

    public final RealVector currVel(double t) { // dead reckon the current velocity
        return lastVel;
    }

    public final double dist(LocationIntrf l2, double t) { // distance to a LocationIntrf thing
        RealVector p1 = this.currPos(t);
        RealVector p2 = l2.drCurrPos(t);
        double d = p1.getDistance(p2); // error if dimension mismatch
        return d;
    }

    public final double dist(RealVector p2, double t) { // distance to a point
        RealVector p1 = this.currPos(t);
        /*
        System.out.println("LocationImp_DOF_3_Ord_1 dist");
        System.out.println("Pstn:");
        DSUtils.showVector(p1, " %10.3f");
        System.out.println("Trgt:");
        DSUtils.showVector(p2, " %10.3f");
        */
        
        double d = p1.getDistance(p2); // error if dimension mismatch
        /*
        System.out.printf("perceived distance %.3f \n", d);
        System.out.flush();
        */
        
        return d;
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
    protected double lastTime;
    protected int numDim;
}


// =============================================================================
