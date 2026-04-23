/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.SimpleIADS;

import groupw.BaseSim.Event;
import groupw.BaseSim.Scheduler;
import org.apache.commons.math4.legacy.linear.RealVector;

/**
 *
 * @author BenWise
 */
public class SimpleDetonation
        extends Event {

    public SimpleDetonation(String tn, RealVector center, double pTime,
            Scheduler s) {
        super(s, pTime);
        this.typeName = tn;
        this.center = center;
    }

    @Override
    public void process() {
        assert (null != DamageServer.theDS);
        DamageServer.theDS.applyDetonationDamage(typeName, center);
    }

    protected String typeName;
    public RealVector center;

    // An ICBM moves at about Mach 25.
    // Two things converging at that rate close the gap at
    // Mach 50, or about 17,150 meters / second.
    // They close a 1m gap in about 1/17,150 seconds,
    // so I chose 1/40,000 sec as a delay time that will
    // not cause a miss.
    static public double detDelay = 0.000025; // sec
}


// =============================================================================
