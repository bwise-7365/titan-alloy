/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.BaseSim;

import org.apache.commons.math4.legacy.linear.RealVector;

/**
 *
 * @author BenWise The interface to things with vector location and velocity,
 * hence also dead-reckoned positions.
 *
 * A class implementing this interface will require a location implementation
 * object that manages the state. The minimal Location3DOFImp is provided,
 * though you are free to use others (6 DOF, second order, etc.) Units are
 * meters, meters / second, and seconds.
 *
 */
public interface LocationIntrf {

    // TODO: distinguish between geocentric, geodetic, lat/long/alt, etc.

    /**
     * Specify how many dimensions will be in position, velocity, etc.
     * Only 2 or 3 can be used.
     * @param n     Number of dimensions
     */
    public void setDim(int n);           // specify dimensions of vectors (2 or 3)

    public int getDim();

    /**
     * Use dead-reckoning to estimate the current position.
     * @param t     Time
     *
     * @return      Dead-reckoned position at the specified time
     */
    public RealVector drCurrPos(double t); // dead reckon the current position

    /**
     * Get the time of last update, mostly for checking that we have not gotten events out of order
     * @return
     */
    public double getLastTime();

    /**
     * Use dead-reckoning to estimate the current velocity.
     * @param t     Time
     *
     * @return      Dead-reckoned velocity at the specified time
     */
    public RealVector drCurrVel(double t); // dead reckon the current velocity

    /**
     * Get the dead-reckoned distance in meters to another
     * LocationIntrf at the specified time.
     *
     * @param l2    The other object to which distance should be estimated
     * @param t     time
     *
     * @return      distance in meters
     */
    public double drDist(LocationIntrf l2, double t);

    /**
     * Get the dead-reckoned distance in meters to a coordinate
     * at the specified time.
     *
     * @param l2    The other object to which distance should be estimated
     * @param t     time
     *
     * @return      distance in meters
     */
    public double drDist(RealVector p, double t);

    /**
     * Record the last exact position and velocity as well as the
     * time they were exact.
     *
     * @param pv        Position vector
     * @param vv        Velocity vector
     * @param lt        Time
     */
    public void setLastPosVelTime(RealVector pv, RealVector vv, double lt);

    /**
     * Record the last exact position  as well as the time it was exact.
     * Sets last recorded velocity to the zero vector.
     * @param pv        Position vector
     * @param lt        Time
     */
    public void setLastPosTime(RealVector pv, double lt);
}


// =============================================================================
