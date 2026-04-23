/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.SimpleIADS;

import groupw.BaseSim.LocationIntrf;
import org.apache.commons.math4.legacy.linear.RealVector;

/**
 *
 * @author BenWise Extend the basic Location interface by making it possible to
 * reset (P, V, A, t) or (A, t). Second order dynamics are good for things that
 * move in smooth arcs, like fixed wing aircraft and ballistic missiles.
 * The setLastAccTime method is for controllers that can only exert forces.
 */
public interface LocationIntrf_DOF_3_Ord_2
        extends LocationIntrf {

    /**
     * Record the last acceleration as well as the time it was exact.
     * This is most useful for controllers that can only set acceleration by exerting force.
     * @param av    Acceleration vector
     * @param lt    Time
     */
    public void setLastAccTime(RealVector av, double lt);

    /**
     * Record the last exact position, velocity and acceleration as well as the time they were exact.
     * @param pv    Position vector
     * @param vv    Velocity vector
     * @param av    Acceleration vector
     * @param lt    Time
     */
    public void setLastPosVelAccTime(RealVector pv, RealVector vv, RealVector av, double lt);

}


// =============================================================================
