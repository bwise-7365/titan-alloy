/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.SimpleIADS;

import groupw.BaseSim.LocationIntrf;
import groupw.BaseSim.Hull;
import groupw.BaseSim.EntEvent;
import groupw.BaseSim.Scheduler;
import static groupw.BaseSim.DSUtils.MACH_SEA_LEVEL;
import static groupw.BaseSim.DSUtils.NAUTICAL_MILE;
import static groupw.BaseSim.DSUtils.RECOMMENDED_STEP_FRACTION;
import static groupw.BaseSim.DSUtils.showVector;
import static groupw.Network.NWUtils.ReportingLevel.Debugging;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static groupw.Network.NWUtils.rLevelLE;
import static java.lang.Math.min;
import org.apache.commons.math4.legacy.linear.RealVector;

/**
 * SimpleMissile flies toward an Entity then schedules a detonation at the
 * closest approach or rangeMax. It could use a single-state pFSM because
 * 'intercept' is its only action, but I did not bother. A non-simple missile,
 * like route-following cruise missile, could be done as a kind of
 * SimpleAirFrame. This differs from SimpleAirFrame in that it has only one
 * speed and is not expected to survive one flight.
 *
 * @author BenWise
 */
public class SimpleMissile
        extends Hull {

    // TODO: use second-order dynamics and limit turn-rate.
    // According to the unofficial South African Air Force page in 2024-12,
    // the Aim-9B can turn at 12 g's going Mach 2.5 for 6250 meter turn radius.
    // https://www.saairforce.co.za/the-airforce/weapons/4/aim-9b-sidewinder
    // The IRIS-T can turn at 60 g's going Mach 3.0 for 1800 meter turn radius.
    // https://www.saairforce.co.za/the-airforce/weapons/92/iris-t
    private SimpleMissile(String tn, double rangeMax, double speed,
            Scheduler s) {
        super(tn, s);
        this.rangeMax = rangeMax;
        this.speed = speed;
        assert (0.0 < rangeMax);
        assert (0.0 < speed);
        this.timeMax = rangeMax / speed;
        this.iGuide = new InterceptGuidance();
        this.dtMax = 5.0; // seconds, a reasonable default
    }
    public double rangeMax; // meters
    public double speed;  // meters / second
    public double detRange = 5.0; // meters, a default
    public Hull target = null; // mobile or stationary
    public InterceptGuidance iGuide = null;
    private double launchTime = -1.0;

    protected double timeMax = -1.0; // maximum flight time

    static public SimpleMissile makeSimpleMissile(String tn, int numDim, Scheduler sim) {
        double rng = 0.0;
        double spd = 0.0;
        switch (tn) {
            case EntityData.a2aMissileBlue01:
            case EntityData.a2aMissileRed01:
                rng = 60.0 * NAUTICAL_MILE;
                spd = 4.0 * MACH_SEA_LEVEL;
                break;
            case EntityData.a2aMissileBlue02:
            case EntityData.a2aMissileRed02:
                rng = 20.0 * NAUTICAL_MILE;
                spd = 2.0 * MACH_SEA_LEVEL;
                break;
            default:
                System.out.printf("Unknown missile type: %s\n", tn);
                break;
        }
        SimpleMissile sm = new SimpleMissile(tn, rng, spd, sim);
        sm.setDynamics(new LocationImp_DOF_3_Ord_1(numDim));
        return sm;
    }

    public void setTarget(Hull h) {
        assert (null != h);
        target = h;
    }

    /**
     * Make a simple detonation event at the missile's current location. As a
     * side-affect, the missile dies and is deregistered.
     *
     * @return SimpleDetonation event
     */
    public SimpleDetonation makeDetonation() {
        double t1 = mySim.getCurrTime();
        RealVector c = drCurrPos(t1);
        double t2 = t1 + SimpleDetonation.detDelay;
        SimpleDetonation d = new SimpleDetonation(typeName, c, t2, mySim);
        // notice that this self-destructs instantly upon detonation, as it clearly should
        setActive(false);
        mySim.deregisterHull(this);
        mySim.deregisterEntity(getID());
        return d;
    }

    @Override
    public void process() {
        boolean highP = rLevelLE(High, mySim.rLevel);
        boolean dbgP = rLevelLE(Debugging, mySim.rLevel);
        if (highP) {
            System.out.println("");
            System.out.printf("%s SimpleMissile.process() %4d \n",
                    mySim.timeStamp(), getID());
        }
        double tNow = mySim.getCurrTime();
        if (launchTime < 0.0) {
            launchTime = tNow;
            if (highP) {
                System.out.printf("%s SimpleMissile %4d launched at %.2f ending about %.2f\n",
                        mySim.timeStamp(), getID(), launchTime, launchTime + timeMax);
            }
        }
        RealVector p = this.drCurrPos(tNow);
        RealVector v = this.drCurrVel(tNow);
        double s1 = v.getNorm();
        RealVector q = target.drCurrPos(tNow);
        double separation = p.getDistance(q);
        double endTime = launchTime + timeMax;
        if ((endTime <= tNow) || (separation < detRange)) {
            if (highP) {
                System.out.printf("%s SimpleMissile %4d detonating after flight time %.2f/%.2f at distance %.2f/%.2f\n",
                        mySim.timeStamp(), getID(), tNow, endTime, separation, detRange);

                System.out.printf("Missile %4d position:\n", getID());
                showVector(p, " %9.3f");
                System.out.printf("Missile %4d velocity, %9.3f\n", getID(), s1);
                showVector(v, " %9.3f");
            }
            SimpleDetonation sDet1 = makeDetonation();
            sDet1.process();
            if (highP) {
                int nEnt = mySim.getNumEntities();
                System.out.printf("There are now %d entities\n", nEnt);
            }
        } else {
            RealVector w = target.drCurrVel(tNow);
            double dTol = 1.0; // meters
            double combinedSpeed = speed + w.getNorm();
            assert (0.0 < combinedSpeed);
            double tTol = dTol / (2 * combinedSpeed);
            InterceptGuidance.InterceptResult ir = iGuide.interceptCourse(
                    p, speed, q, w, tTol);

            // The next event should be after dtMax or one quarter of the
            // remaining time until closest approach, whichever is less.
            // Notice that we cannot use DSUtils.recMoverTimeStep
            // because we do not have the distance to the expected interception
            // point, just the expected time until intercept.
            double remTime = ir.tIntercept; // remaining time to closest approach
            double dt = min(dtMax, RECOMMENDED_STEP_FRACTION * remTime);
            double t2 = tNow + dt;
            RealVector v2 = ir.v; // get the intercept velocity vector
            double s2 = v2.getNorm();

            // TODO: incorporate some limit on rate of turn
            setLastPosVelTime(p, v2, tNow);
            if (highP) {
                System.out.printf("%s SimpleMissile %4d at distance %.2f/%.2f, updating pos, vel to intercept %4d\n",
                        mySim.timeStamp(), getID(),
                        separation, detRange, target.getID());

                System.out.printf("Missile %4d position:\n", getID());
                showVector(p, " %9.3f");
                System.out.printf("Missile %4d velocity, %9.3f \n", getID(), s2);
                showVector(v2, " %9.3f");
                System.out.printf("Missile %4d next event after %.4f \n", getID(), dt);
            }

            if (active) {
                EntEvent e = new EntEvent(this, mySim, t2);
                mySim.addEvent(e);
            }
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
