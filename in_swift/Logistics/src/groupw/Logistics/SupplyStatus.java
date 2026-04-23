/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.Logistics;

import java.util.Set;

import static java.lang.Math.max;

public class SupplyStatus {

    public SupplyStatus() { // all zeros
        reorderLevel = new Manifest();
        maxDesiredLevel = new Manifest();
        currentLevel = new Manifest();
        consumptionRate = new Manifest();
    }

    /**
     * Without altering this one, make a new status which is the sum of this one and s1
     * @param s1 SupplyStatus to be added to this one
     * @return SupplyStatus which is the sum of this one and s1
     */
    public SupplyStatus add(SupplyStatus s1) {
        SupplyStatus s2 = new SupplyStatus();
        s2.reorderLevel    = Manifest.add(reorderLevel,    s1.reorderLevel);
        s2.maxDesiredLevel = Manifest.add(maxDesiredLevel, s1.maxDesiredLevel);
        s2.currentLevel    = Manifest.add(currentLevel,    s1.currentLevel);
        s2.consumptionRate = Manifest.add(consumptionRate, s1.consumptionRate);
        return s2;
    }

    /**
     * Without altering this one, make a new status which is a scaled version of this one.
     * A negative factor is treated like zero.
     * @param f factor by which to multiply
     * @return new status which is a scaled version of this one
     */
    public SupplyStatus makeScaled(double f) {
        SupplyStatus s2 = new SupplyStatus();
        if (0.0 < f) {
            s2.reorderLevel       = reorderLevel.makeScaled(f);
            s2.maxDesiredLevel    = maxDesiredLevel.makeScaled(f);
            s2.currentLevel       = currentLevel.makeScaled(f);
            s2.consumptionRate    = consumptionRate.makeScaled(f);
        }
        return s2;
    }

    public static SupplyStatus copy(SupplyStatus s1) {
        SupplyStatus s2 = new SupplyStatus();
        s2.reorderLevel = s1.reorderLevel;
        s2.maxDesiredLevel = s1.maxDesiredLevel;
        s2.currentLevel = s1.currentLevel;
        s2.consumptionRate = s1.consumptionRate;
        return s2;
    }

    /**
     * Build a Manifest of what shortages are anticipated dt in the future.
     *
     * @param dt how far ahead to anticipate
     * @return expected shortages, if any
     */
    public Manifest estShortages(double dt) {
        Manifest es = new Manifest();
        // because items are dropped when they hit zero,
        // make sure to get all relevant items.
        Set<String> i1 = reorderLevel.getItemNames();
        i1.addAll(currentLevel.getItemNames());
        i1.addAll(consumptionRate.getItemNames());
        for (String item : i1) {
            double reorder = reorderLevel.getAvailable(item);
            double current = currentLevel.getAvailable(item);
            double rate = consumptionRate.getAvailable(item);
            double estLevel = current - (dt * rate); // possibly negative
            double estShortage = max(0, reorder - estLevel);
            if (0.0 < estShortage) {
                es.addInventory(item, estShortage);
            }
        }
        return es; // possibly empty if no shortages anticipated
    }

    /**
     * Build a Manifest of what resupply quantities are anticipated dt in the future.
     * Note that the default, non-final policy has several parts:
     * 1: If no shortages are anticipated at time dt in the future, resupply nothing.
     * 2a: If shortages are anticipated, resupply those items back to maximum (maxima).
     * 2b: Partially resupply other items halfway to their maximum (maxima).
     *
     * @param dt how far ahead to anticipate
     * @return expected resupply quantities, if any
     */
    public Manifest estResupply(double dt) {
        Manifest er = new Manifest();
        Manifest es = estShortages(dt);
        Set<String> shortItems = es.getItemNames();
        if (!shortItems.isEmpty()) {
            // because items are dropped when they hit zero,
            // make sure to get all relevant items.
            Set<String> i1 = reorderLevel.getItemNames();
            i1.addAll(currentLevel.getItemNames());
            i1.addAll(consumptionRate.getItemNames());
            i1.addAll(maxDesiredLevel.getItemNames());
            for (String item : i1) {
                double current = currentLevel.getAvailable(item);
                double rate = consumptionRate.getAvailable(item);
                double maxDesired = maxDesiredLevel.getAvailable(item);
                double estLevel = current - (dt * rate); // possibly negative
                double estShortage = es.getAvailable(item); // i.e. reorder - estLevel, if positive
                double targetLevel = (0 < estShortage) ? maxDesired : (current + maxDesired) / 2;
                double estResupply = targetLevel - estLevel;
                er.addInventory(item, estResupply);
            }
        }
        return er; // possibly empty if no resupply anticipated
    }

    public Manifest reorderLevel = null;
    public Manifest maxDesiredLevel = null;
    public Manifest currentLevel = null;
    public Manifest consumptionRate = null;

}

// =============================================================================
