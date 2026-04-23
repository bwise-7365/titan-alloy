/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */

package groupw.SimpleIADS;

import groupw.BaseSim.Hull;
import groupw.BaseSim.ItemRegistry;
import groupw.BaseSim.Scheduler;
import groupw.Network.NWUtils.ReportingLevel;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static groupw.Network.NWUtils.ReportingLevel.Medium;
import static groupw.Network.NWUtils.ReportingLevel.Silent;
import static groupw.Network.NWUtils.rLevelLE;
import java.util.Map;
import java.util.ArrayList;
import org.apache.commons.math4.legacy.linear.RealVector;

/**
 * DamageServer is a singleton which assesses damage to Hulls near a Detonation,
 * either stochastically or deterministically.
 * @author bwise
 */
public class DamageServer {

    static public void makeDS(Scheduler s) {
        DamageServer ds = new DamageServer(s);
    }

    static public void clearDS() {
        theDS = null; // should be GC'ed soon
    }

    private DamageServer(Scheduler s) {
        System.out.println("Making DamageServer");
        assert(null == theDS);
        theDS = this;

        assert (null != s);
        mySim = s;
    }

    public void applyDetonationDamage(String warheadType,
            RealVector center) {
        double tNow = mySim.getCurrTime();
        String tStamp = mySim.timeStamp();
        
        double sRange = DetonationScanRange.get(warheadType);
        Map<String, Double> sspk = DetonationSSPK.get(warheadType);
        ArrayList<Long> hullIDs = mySim.getHullsInRange(center, sRange, tNow);
        if (rLevelLE(High, rLevel)) {
            System.out.printf("%s Out of %d Entities and %d Hulls, %d were within scan range %.2f\n",
                    mySim.timeStamp(),
                    ItemRegistry.numItems(), mySim.hulls.size(),
                    hullIDs.size(), sRange);
        }
        for (long id : hullIDs) {
            Hull h = mySim.getHull(id);
            RealVector pos = h.drCurrPos(tNow);
            double rng = center.getDistance(pos);
            String trgt = h.getTypeName(); // TODO: get real target type name
            double p = pk(trgt, rng, sspk);
            if (rLevelLE(High, rLevel)) {
                System.out.printf("Distance to  Hull %d %s is %.2f meters, pk = %.3f\n",
                        id, trgt, rng, p);
            }
            boolean hit = (0.50 < p); // deterministic result
            if (stochasticP) {
                hit = mySim.prob(p);
            }
            if (hit) {
                if (rLevelLE(Medium, rLevel)) {
                    System.out.printf("%s Hull %d '%s' was hit by '%s' and will be deregistered\n",
                            tStamp, id, trgt, warheadType);
                }
                mySim.eliminateHull(h);
            } else {
                if (rLevelLE(High, rLevel)) {
                    System.out.printf("%s Hull %d '%s' was not hit by '%s'\n",
                            tStamp, id, trgt, warheadType);
                }
            }
        }
    }

    /**
     *
     * @param targetType name of target type
     * @param range distance between detonation and target, meters
     * @param sspk Map relating targetType to 50% pk range
     * @return probability of kill
     */
    protected double pk(String targetType,
            double range,
            Map<String, Double> sspk) {
        double p = 1.0;
        if (0.0 < range) {
            // TODO: handle unknown targetType
            double r50 = sspk.get(targetType);
            double r2 = r50 * r50;
            p = r2 / (r2 + (range * range));
        }
        return p;
    }

    /**
     * DetonationSSPK specifies the SSPK tables for each known kind of
     * detonation
     */
    public static Map<String, Map<String, Double>> DetonationSSPK = null;

    /**
     * DetonationScanRange specifies how far (in meters) to scan for affected
 entities
     */
    public static Map<String, Double> DetonationScanRange = null;

    public boolean stochasticP = true;
    public ReportingLevel rLevel = Silent;
    protected Scheduler mySim = null;
    protected static DamageServer theDS = null;
}

// =============================================================================
