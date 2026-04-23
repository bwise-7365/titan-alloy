/*
 * ---------------------------------------------------
 *       Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Network.NWUtils.Tuple3;

import static java.lang.Double.max;
import static java.lang.Double.min;
import static java.lang.Math.sqrt;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * This summarizes the matrices of data for the prioritized resupply problem.
 * This assumes different priorities for each unit and each kind of supply.
 * There must be a tradeoff between fuel being delivered and fuel used to deliver.
 * Only a limited number of each kind of vehicle are available.
 * This is used in developing a resupply plan; it is not a simulation of resupply actions.
 *
 * @author BenWise
 */
public class VehicleResupplyMatrix {

    public VehicleResupplyMatrix(
            List<Double> unitPriority, List<Double> unitDistance, List<String> unitNames,
            List<Double> supplyPriority,
            List<VehicleType> vTypes, Map<String, Integer> vCount, Map<String, Double> vFrac,
            List<String> sTypes, List<Double> sCount,
            int fuelIndex) {

        // Check unit data
        int nup = unitPriority.size();
        int nud = unitDistance.size();
        int nun = unitNames.size();
        if (nun != nud) {
            throw new RuntimeException("Number of unit-names , "
                    + nun + ", does not match number of unit-distances, "
                    + nud);
        }
        if (nun != nup) {
            throw new RuntimeException("Number of unit-names , "
                    + nun + ", does not match number of unit-priorities, "
                    + nup);
        }
        for (int i = 0; i < nun; i++) {
            if (unitPriority.get(i) <= 0.0) {
                throw new RuntimeException("Non-positive priority of unit " + unitNames.get(i) + ": " + unitPriority.get(i));
            }
        }
        for (int i = 0; i < nun; i++) {
            for (int j = i + 1; j < nun; j++) {
                if (unitNames.get(i).equals(unitNames.get(j))) {
                    throw new RuntimeException("Duplicated unit name: " + unitNames.get(i));
                }
            }
        }

        // Check vehicle data
        int nvt = vTypes.size();
        int nva = vFrac.size();
        int nvc = vCount.size();
        if (nvt != nvc) {
            throw new RuntimeException("Number of vehicle-types , "
                    + nvt + ", does not match number of vehicle-available, "
                    + nvc);
        }
        if (nvt != nva) {
            throw new RuntimeException("Number of vehicle-types , "
                    + nvt + ", does not match number of vehicle-availabilities, "
                    + nva);
        }
        for (int i = 0; i < nvt; i++) {
            String vt = vTypes.get(i).vehicleTypeName;
            if (vCount.get(vt) < 0.0) {
                throw new RuntimeException("Negative amount of vehicles " + vt + ": " +vCount.get(i));
            }
            if (vFrac.get(vt) < 0.0) {
                throw new RuntimeException("Availability fraction of vehicles " + vt + " too low: " +vFrac.get(i));
            }
            if (1.0 < vFrac.get(vt)) {
                throw new RuntimeException("Availability fraction of vehicles " + vt + " too high: " +vFrac.get(i));
            }
        }
        for (int i = 0; i < nvt; i++) {
            String vt = vTypes.get(i).vehicleTypeName;
            for (int j = i + 1; j < nvt; j++) {
                if (vt.equals(vTypes.get(j))) {
                    throw new RuntimeException("Duplicated vehicle-type: " + vt);
                }
            }
        }

        // Check supply data
        int nst = sTypes.size();
        int nsa = sCount.size();
        int nsp = supplyPriority.size();
        if (nst != nsa) {
            throw new RuntimeException("Number of supply-types , "
                    + nst + ", does not match number of supply-available, "
                    + nsa);
        }
        if (nst != nsp) {
            throw new RuntimeException("Number of supply-types , "
                    + nst + ", does not match number of supply-priorities, "
                    + nsp);
        }
        for (int i = 0; i < nst; i++) {
            if (sCount.get(i) < 0.0) {
                throw new RuntimeException("Negative amount of supply " + sTypes.get(i) + ": " + sCount.get(i));
            }
        }
        for (int i = 0; i < nst; i++) {
            if (supplyPriority.get(i) <= 0.0) {
                throw new RuntimeException("Non-positive priority of supply " + sTypes.get(i) + ": " + supplyPriority.get(i));
            }
        }
        for (int i = 0; i < nst; i++) {
            for (int j = i + 1; j < nst; j++) {
                if (sTypes.get(i).equals(sTypes.get(j))) {
                    throw new RuntimeException("Duplicated supply-type name " + sTypes.get(i));
                }
            }
        }

        // Accept the (somewhat) checked data
        this.unitPriority = unitPriority;
        this.unitDistance = unitDistance;
        this.unitNames = unitNames;
        this.supplyPriority = supplyPriority;
        this.vehicleType = vTypes;
        this.vehiclesAvailableCount =vCount;
        this.vehicleAvailablity = vFrac;
        this.supplyType = sTypes;
        this.supplyAvailable = sCount;
        this.fuelIndex = fuelIndex;
    }

    /**
     * Copy constructor
     *
     * @param vsm VehicleResupplyMatrix to be copied
     */
    public VehicleResupplyMatrix(VehicleResupplyMatrix vsm) {
        this.unitPriority = new ArrayList<>(vsm.unitPriority);
        this.unitDistance = new ArrayList<>(vsm.unitDistance);
        this.supplyPriority = new ArrayList<>(vsm.supplyPriority);
        this.vehicleType = new ArrayList<>(vsm.vehicleType);
        this.vehiclesAvailableCount = new HashMap<>(vsm.vehiclesAvailableCount);
        this.vehicleAvailablity = new HashMap<>(vsm.vehicleAvailablity);
        this.supplyType = new ArrayList<>(vsm.supplyType);
        this.supplyAvailable = new ArrayList<>(vsm.supplyAvailable);
        this.fuelIndex = vsm.fuelIndex;
        setUnfilled(vsm.unfilled);
        setTotal(vsm.total);
        this.setPrioritizedValue();
    }

    final public void setUnfilled(double[][] u) {
        int n = unitPriority.size();
        int m = supplyPriority.size();
        unfilled = new double[n][m];
        for (int i = 0; i < n; i++) {
            System.arraycopy(u[i], 0, unfilled[i], 0, m);
        }
    }

    final public void setTotal(double[][] t) {
        int n = unitPriority.size();
        int m = supplyPriority.size();
        total = new double[n][m];
        for (int i = 0; i < n; i++) {
            System.arraycopy(t[i], 0, total[i], 0, m);
        }
    }

    final public void setPrioritizedValue() {
        int n = unitPriority.size();
        int m = supplyPriority.size();
        pValue = new double[n][m];
        for (int i = 0; i < n; i++) {
            double pi = unitPriority.get(i);
            for (int j = 0; j < m; j++) {
                double pij = combinePriorities(pi, supplyPriority.get(j));
                double v = fulfillmentValue(unfilled[i][j], total[i][j]);
                pValue[i][j] = pij * v;
            }
        }
    }

    /**
     * Combine unit and supply priorities to determine the priority of getting that supply to that unit.
     * The geometric average satisfies two requirements that product and arithmetic averages do not.
     * First, if unit and supply priorities are in the [1.0, 10.0] range, so is the combined value.
     * Second, if unit priority is zero (which it should not be), then so is the combined value.
     *
     * @param up unit priority of 1.0 or more
     * @param sp supply priority of 1.0 or more
     * @return combined priority of that supply to that unit
     */
    public double combinePriorities(double up, double sp) {
        if (up < 1.0) {
            throw new IllegalArgumentException("Unit priority must be 1.0 or more");
        }
        if (sp < 1.0) {
            throw new IllegalArgumentException("Supply priority must be 1.0 or more");
        }
        return sqrt(up * sp);
    }

    /**
     * Returns the total 'value' to this unit, including unit and supply priorities
     *
     * @param nUnit the unit whose value is sought
     * @param pv matrix of prioritized unit-supply fulfillment values
     * @return value to this unit
     */
    public double prioritizedUnitValue(int nUnit, double[][] pv) {
        double v = 0.0;
        int m = supplyPriority.size();
        for (int j = 0; j < m; j++) {
            v = v + pv[nUnit][j];
        }
        return v;
    }

    /**
     * Calculates, within limits of supplies available, the resupply amounts
     * to maximize the sum of the priority-weighted fulfillment values
     *
     * @return matrix of how much each unit gets of each supply
     */
    protected double[][] priorityWeightedResupply() {
        int n = unitPriority.size();
        int m = supplyPriority.size();
        double[] c = new double[n];
        double[][] r = new double[n][m];
        for (int j = 0; j < m; j++) {
            double sumUnfilled = 0.0;
            double sumRatio = 0.0;
            for (int i = 0; i < n; i++) {
                sumUnfilled = sumUnfilled + unfilled[i][j];
                if (0 < unfilled[i][j]) {
                    double pij = combinePriorities(unitPriority.get(i), supplyPriority.get(j));
                    r[i][j] = (total[i][j] * total[i][j]) / pij;
                    sumRatio = sumRatio + r[i][j];
                }
            }
            if (sumUnfilled > supplyAvailable.get(j)) {
                c[j] = (sumUnfilled - supplyAvailable.get(j)) / sumRatio;
            } else {
                c[j] = 0.0;
            }
        }
        double[][] s = new double[n][m];
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                if (0 < unfilled[i][j]) {
                    s[i][j] = unfilled[i][j] - (c[j] * r[i][j]);
                    // algebraically, by construction, 's' can never be negative.
                    // But numerically, tiny round-off errors happen.
                    s[i][j] = max(0.0, s[i][j]);
                }
            }
        }
        return s;
    }

    /**
     * Greedy heuristic to determine which, if any, is the best feasible load to send to which unit
     * TODO: fix 'bestLoad' variable, which was an index into a list which no longer exists.
     * @param vLoads
     * @return tuple of best unit and best load (if any)
     */
    public Tuple3<Integer, Integer, Double> bestUnitToLoad(LinkedHashMap<VehicleLoad, VehicleType> vLoads) {
        double bestGain = 0.0; // we only want positive gains
        int bestUnit = -1;
        int bestLoad = -1;
        double bestFrac = 1.0; // the usual result
        List<Double> fractions = new ArrayList<>(3);
        fractions.add(1.00);
        fractions.add(0.75);
        fractions.add(0.50);
        fractions.add(0.25);
        int n = unitPriority.size();
        int m = vLoads.size();
        int j = 0;
        for (int i = 0; i < n; i++) {
            double baseValue = prioritizedUnitValue(i, pValue);
            double di = unitDistance.get(i);
            //for (int j = 0; j < m; j++) {
            for(Map.Entry<VehicleLoad, VehicleType> entry : vLoads.entrySet()){
                VehicleLoad vLoad = entry.getKey();
                VehicleType vType = entry.getValue();
                boolean fsbl = feasible(vLoad, vType, di);
                if (fsbl) {
                    for (double frc : fractions) {
                        double vij = newPrioritizedUnitValue(i, vLoad, frc, unfilled);
                        // Prioritized value per gallon is the metric to balance the two criteria of "use less fuel" and "deliver more"
                        double gallons = di / vType.fuelEfficiency;
                        double valuePerGallon = (vij - baseValue) / gallons;
                        if (bestGain < valuePerGallon) {
                            bestGain = valuePerGallon;
                            bestUnit = i;
                            bestLoad = j;
                            bestFrac = frc;
                        }
                    }
                }
                j++;
            }
        }
        Tuple3<Integer, Integer, Double> bestUnitLoad = new Tuple3<>(bestUnit, bestLoad, bestFrac);
        return bestUnitLoad;
    }

    public double prioritizedUnitValue(int i) {
        return prioritizedUnitValue(i, pValue);
    }

    /**
     * If the units have the 'unfulfilled' matrix 'u', what would be the
     * new value to unit 'nUnit' of receiving this particular vehicle-load?
     *
     * @param nUnit unit to be resupplied
     * @param vl hypothetical vehicle load which it might receive
     * @param frac what fraction of a load to use
     * @param u current state of unfulfilled supplies
     * @return new (higher) value, including unit and supply priorities
     */
    public double newPrioritizedUnitValue(int nUnit, VehicleLoad vl, double frac, double[][] u) {
        double f = min(1.0, max(0.0, frac));
        double pUnit = unitPriority.get(nUnit);
        double v = 0.0;
        int m = supplyPriority.size();
        for (int j = 0; j < m; j++) {
            double u2 = u[nUnit][j] - (f * vl.load.get(j));
            //u2 = max(0.0, u2);
            double vj = fulfillmentValue(u2, total[nUnit][j]);
            v = v + vj * combinePriorities(pUnit, supplyPriority.get(j));
        }
        return v;
    }

    boolean feasible(VehicleLoad vl, VehicleType vt, double distance) {
        boolean sAvailable = true;
        for (int i = 0; i < supplyAvailable.size(); i++) {
            sAvailable = sAvailable && (vl.load.get(i) <= supplyAvailable.get(i));
        }
        String vtName = vl.vehicleType;
        if (null == vtName) {
        }
        int vac = vehiclesAvailableCount.get(vtName);
        boolean vehicleAvailableP = (1 <= vac);
        double vehicleFuel = (distance / vt.fuelEfficiency);
        double deliveredFuel = vl.load.get(fuelIndex);
        boolean sufficientFuel = (vehicleFuel + deliveredFuel <= supplyAvailable.get(fuelIndex));
        return (sAvailable && vehicleAvailableP && sufficientFuel);
    }

    /**
     * Modify this VehicleSupplyMatrix by decrementing supplies available,
     * vehicles available, and unfilled supplies of the unit.
     * This operation is not thread safe.
     *
     * @param ndxUnit which unit to be resupplied in this plan
     * @param vl which combined vehicle/load for that vehicle in this
     * @param frac what fraction of the v/l to apply
     */
    public void applyVehicleLoad(int ndxUnit, VehicleLoad vl, VehicleType vt, double frac) {
        double roundOffTolerance = 1e-6;
        for (int j = 0; j < vl.load.size(); j++) {
            double sj = frac * vl.load.get(j);
            { // take supplies from VSM in this plan, decrement supplies available
                double a = supplyAvailable.get(j) - sj;
                if (a < -roundOffTolerance) {
                    System.out.printf("**** Negative VSM supply %d: %.4f \n",
                            j, a);
                }
                supplyAvailable.set(j, max(0.0, a));
            }
            { // credit supplies to unit in this plan, decrement unfilled need
                double u = unfilled[ndxUnit][j] - sj;
                if (u < -roundOffTolerance) {
                    // this is presumably a slight over-fulfillment
                    System.out.printf("Negative unit unfilled level, unit %d, supply %d: %.4f \n",
                            ndxUnit, j, u);
                }
                unfilled[ndxUnit][j] = u; //max(0.0, u);
            }
        } // end of loop over j

        // recalculate what the prioritized values would be,
        // if those transfers were made.
        setPrioritizedValue();

        { // decrement fuel used by vehicle in this plan
            double fuelCost = unitDistance.get(ndxUnit) / vt.fuelEfficiency;
            double f = supplyAvailable.get(fuelIndex) - fuelCost;
            if (f < -roundOffTolerance) {
                System.out.printf("**** Negative VSM fuel level: %.4f \n", f);
            }
            supplyAvailable.set(fuelIndex, max(0.0, f));
        }
        { // decrement vehiclesAvailable available in this plan
            int av = vehiclesAvailableCount.get(vl.vehicleType) - 1;
            if (av < 0) {
                System.out.printf("*** Negative VSM %s vehicles available: %d \n",
                        vl.vehicleType, av);
            }
            vehiclesAvailableCount.put(vl.vehicleType, (int) max(0, av));
        }

    }

    public void showAvailable() {
        int nSupplyTypes = supplyAvailable.size();
        System.out.printf("There are %2d types of supply: \n", nSupplyTypes);
        for (int i = 0; i < nSupplyTypes; i++) {
            System.out.printf(" %8s %10.2f \n", supplyType.get(i), supplyAvailable.get(i));
        }
        System.out.println();

        int nVehicleTypes = vehiclesAvailableCount.size();
        System.out.printf("There are %2d kinds of vehicles: \n", nVehicleTypes);
        for (int i = 0; i < nVehicleTypes; i++) {
            VehicleType vti = vehicleType.get(i);
            String vtName = vti.vehicleTypeName;
            System.out.printf(" %8s %4d \n", vtName, vehiclesAvailableCount.get(vtName));
            //System.out.printf(" %8s %4d \n", vehicleType.get(i), vehiclesAvailableCount.get(vehicleType.get(i)));
        }
        System.out.println();
    }

    public void showPriorities() {
        int nUnits = unitPriority.size();
        int nSupplyTypes = supplyAvailable.size();

        System.out.print("Unit priorities:\n");
        for (int i = 0; i < nUnits; i++) {
            System.out.printf(" %8s %5.2f\n", unitNames.get(i), unitPriority.get(i));
        }
        System.out.println();

        System.out.print("Supply priorities:");
        for (int i = 0; i < nSupplyTypes; i++) {
            System.out.printf(" %5.2f", supplyPriority.get(i));
        }
        System.out.println();

        System.out.print("Combined priorities:\n");
        for (int i = 0; i < nUnits; i++) {
            for (int j = 0; j < nSupplyTypes; j++) {
                double pij = combinePriorities(unitPriority.get(i), supplyPriority.get(j));
                System.out.printf(" %6.3f", pij);
            }
            System.out.println();
        }
    }

    public void showUnitSupplies() {
        int nUnits = unitPriority.size();
        int nSupplyTypes = supplyAvailable.size();

        System.out.println("Unit unfilled:");
        for (int i = 0; i < nUnits; i++) {
            System.out.printf(" %2d, %8s: ", i, unitNames.get(i));
            for (int j = 0; j < nSupplyTypes; j++) {
                System.out.printf(" %9.2f", unfilled[i][j]);
            }
            System.out.println();
        }
        System.out.println();
        System.out.flush();

        System.out.println("Unit total:");
        for (int i = 0; i < nUnits; i++) {
            System.out.printf(" %2d: ", i);
            for (int j = 0; j < nSupplyTypes; j++) {
                System.out.printf(" %9.2f", total[i][j]);
            }
            System.out.println();
        }
        System.out.println();
        System.out.flush();

        System.out.println("Basic value:");
        for (int i = 0; i < nUnits; i++) {
            System.out.printf(" %2d: ", i);
            for (int j = 0; j < nSupplyTypes; j++) {
                double pij = combinePriorities(unitPriority.get(i), supplyPriority.get(j));
                double bv = pValue[i][j] / pij;
                System.out.printf(" %8.4f", bv);
            }
            System.out.println();
        }
        System.out.println();
        System.out.flush();

        System.out.println("Prioritized value:");
        for (int i = 0; i < nUnits; i++) {
            System.out.printf(" %2d: ", i);
            for (int j = 0; j < nSupplyTypes; j++) {
                System.out.printf(" %9.2f", pValue[i][j]);
            }
            System.out.println();
        }
        System.out.println();
        System.out.flush();
    }

    /**
     * Compares unfilled versus total to determine pValue.
     * The lower the amount unfilled, the higher the value.
     * It is quadratic, not linear.
     *
     * @param u unfilled amount for this type of supply
     * @param t total amount for this type of supply
     * @return one if unfilled is zero, zero if unfilled is total, curved between.
     */
    public double fulfillmentValue(double u, double t) {
        double v = 1.0 - (u * u) / (t * t);
        return v;
    }

    public String getUnitName(int i) {
        return unitNames.get(i);
    }

    public String getSupplyType(int i) {
        return supplyType.get(i);
    }

    public String getVehicleType(int i) {
        return vehicleType.get(i).vehicleTypeName;
    }

    protected List<String> unitNames = null;
    protected List<Double> unitDistance = null;
    protected List<Double> unitPriority = null;

    protected List<VehicleType> vehicleType = null;
    protected Map<String, Integer> vehiclesAvailableCount = null;
    protected Map<String, Double> vehicleAvailablity = null;

    protected List<String> supplyType = null;
    protected List<Double> supplyPriority = null;
    protected List<Double> supplyAvailable = null; // available in the dispatcher

    protected double[][] unfilled; // total - onHand = unfilled
    protected double[][] total; // size of requirement, whether met or not.
    protected double[][] pValue;
    protected int fuelIndex; // which supply is fuel for transport vehiclesAvailable
}


// =============================================================================
