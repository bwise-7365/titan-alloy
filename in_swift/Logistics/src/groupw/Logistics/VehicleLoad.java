/*
 * ---------------------------------------------------
 *       Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import java.util.ArrayList;
import java.util.List;

/**
 * A VehicleLoad summarizes data for an analyst-defined load of supplies
 * on an analyst-selected vehicle. This is treated as a constant because
 * there are complex constraints on what supplies can be carried together
 * and what vehicles can carry them. It also allows access-by-index.
 *
 * @author BenWise
 */
public class VehicleLoad {

    /**
     * Create a vehicle-load object, initialized for a number of supplies
     *
     * @param sts
     */
    /*
    public VehicleLoad(int n) {
        sTypes = null;
        load = new ArrayList<>(4);
        for (int i = 0; i < n; i++) {
            load.add(0.0);
        }
        milesPerGallon = 0.0;
        vehicleType = -1;
    }
    */
    public VehicleLoad(){}
    
    public VehicleLoad(List<String> sts){
        sTypes = sts;
        int n = sTypes.size();
        load = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            load.add(0.0);
        }
    }

    public Manifest makeManifest(double frac) {
        int n = load.size();
        int m = sTypes.size();
        if (1.0 < frac){
            throw new RuntimeException("Fraction of supply , " + frac + ", cannot be above 1.0");
        }
        if (frac < 0.0){
            throw new RuntimeException("Fraction of supply , " + frac + ", cannot be below 0.0");
        }
        if (n != m) {
            throw new RuntimeException("Number of supply-quantities , " + n + ", does not match number of supply-names, " + m);
        }
        Manifest mnfst = new Manifest();
        for (int i = 0; i < n; i++) {
            mnfst.addInventory(sTypes.get(i), frac * load.get(i));
        }
        return mnfst;
    }

    /**
     * This is the list of how much of each item is in a full load.
     * Units depend on the kind of supplies in the manifest.
     */
    public List<Double> load;
    /**
     * List the names of each kind/class/etc of supply
     */
    public List<String> sTypes;

    public String name;
    public String vehicleType;
    
}


// =============================================================================
