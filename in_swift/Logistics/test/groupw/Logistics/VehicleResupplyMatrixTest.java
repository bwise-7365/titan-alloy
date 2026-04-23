/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Network.NWUtils.Tuple2;
import groupw.Network.NWUtils.Tuple3;
import groupw.Network.NWUtils.Tuple4;

import java.util.*;

import org.junit.Test;
import static org.junit.Assert.*;

/**
 * NOTE WELL: 'case A' is also in 'prioritized-loads.xlsx'
 *
 * @author BenWise
 */
public class VehicleResupplyMatrixTest {

    public VehicleResupplyMatrixTest() {
    }

    @Test
    public void testBestUnitToLoad() {
        System.out.println("\nStarting testBestUnitToLoad");
        VehicleResupplyMatrix rsm = makeCase(saWater);
        rsm.showAvailable();

        LinkedHashMap<VehicleLoad, VehicleType> vl = makeVLListA();
        Tuple3<Integer, Integer, Double> b = rsm.bestUnitToLoad(vl);
        int bestUnit = b.get0();
        int bestVL = b.get1();
        double bestFrac = b.get2();
        assertTrue(0.0 <= bestFrac);
        assertTrue(bestFrac <= 1.0);
        System.out.printf("Best Unit: %2d, Best VL: %3d BestFrac: %5.3f \n",
                bestUnit, bestVL, bestFrac);
    }

    @Test
    public void testRSExcess() {
        System.out.println("\nStarting testRSExcess");
        VehicleResupplyMatrix rsm = makeCase(saExcess);
        testRS(rsm);
    }

    @Test
    public void testRSWater() {
        System.out.println("\nStarting testRSWater");
        VehicleResupplyMatrix rsm = makeCase(saWater);
        testRS(rsm);
    }

    @Test
    public void testRSAll() {
        System.out.println("\nStarting testRSAll");
        VehicleResupplyMatrix rsm = makeCase(saAll);
        testRS(rsm);
    }

    private void testRS(VehicleResupplyMatrix rsm) {
        rsm.showUnitSupplies();
        System.out.println();
        rsm.showAvailable();
        System.out.println();
        rsm.showPriorities();
        System.out.println();
        double[][] s = rsm.priorityWeightedResupply();
        int n = rsm.unitPriority.size();
        int m = rsm.supplyPriority.size();
        double[] ps = new double[m];
        System.out.println("Resupply amounts by unit:");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                System.out.printf("  %8.2f", s[i][j]);
                ps[j] = ps[j] + s[i][j];
                assertTrue(0 <= s[i][j]);
            }
            System.out.println();
        }
        System.out.println();
        System.out.println("Resupply amounts total:");
        for (int i = 0; i < m; i++) {
            System.out.printf("  %8.2f", ps[i]);
        }
        System.out.println("\n");
        System.out.println("Remaining fraction unfilled:");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                double rij = (rsm.unfilled[i][j] - s[i][j]) / rsm.total[i][j];
                System.out.printf("  %.3ef", rij);
            }
            System.out.println();
        }
        System.out.println();
    }

    @Test
    public void testLSAll() {
        System.out.println("\nStarting testLSAll");
        LinkedHashMap<VehicleLoad, VehicleType> vls = makeVLListA();
        VehicleResupplyMatrix rsm = makeCase(saAll);
        testLS(vls, rsm);
    }

    @Test
    public void testLSFuel() {
        System.out.println("\nStarting testLSFuel");
        LinkedHashMap<VehicleLoad, VehicleType> vls = makeVLListA();
        VehicleResupplyMatrix rsm = makeCase(saFuel);
        testLS(vls, rsm);
    }

    @Test
    public void testLSAmmo() {
        System.out.println("\nStarting testLSAmmo");
        LinkedHashMap<VehicleLoad, VehicleType> vls = makeVLListA();
        VehicleResupplyMatrix rsm = makeCase(saAmmo);
        testLS(vls, rsm);
    }

    @Test
    public void testLSWater() {
        System.out.println("\nStarting testLSWater");
        LinkedHashMap<VehicleLoad, VehicleType> vls = makeVLListA();
        VehicleResupplyMatrix rsm = makeCase(saWater);
        testLS(vls, rsm);
    }

    /**
     * Notice that in the 'excess' case, it plans to use about 6478 of fuel.
     * The units needed 4112 and are planned to get 4230 (slight over-delivery),
     * so it plans to use 2248 to do the deliveries
     */
    @Test
    public void testLSExcess() {
        System.out.println("\nStarting testLSExcess");
        LinkedHashMap<VehicleLoad, VehicleType> vls = makeVLListA();
        VehicleResupplyMatrix rsm = makeCase(saExcess);
        testLS(vls, rsm);
    }

    @Test
    public void testLSNothing() {
        System.out.println("\nStarting testLSNothing");
        LinkedHashMap<VehicleLoad, VehicleType> vls = makeVLListA();
        VehicleResupplyMatrix rsm = makeCase(saNothing);
        testLS(vls, rsm);
    }

    private void testLS(LinkedHashMap<VehicleLoad,VehicleType> vls, VehicleResupplyMatrix rsm) {

        rsm.showAvailable();
        rsm.showUnitSupplies();
        System.out.printf("There are %d vehicle-load combinations defined\n",
                vls.size());
        //for (int i=0; i<vls.size(); i++){
        for(Map.Entry<VehicleLoad, VehicleType> entry : vls.entrySet()){
            String v = entry.getValue().vehicleTypeName;
            System.out.printf("%8s ", v);
            for (int j = 0; j<entry.getKey().load.size(); j++){
             
                System.out.printf(" %8.2f", entry.getKey().load.get(j));
            }
            System.out.println();
        }
        System.out.println("Initial (null) plan");

        List<Tuple2<String, Tuple4<String, Manifest, Double, List<Integer>>>> plan;

        int nUnits = rsm.unitPriority.size();
        List<List<Integer>> plans = new ArrayList<>(nUnits); // list of what VL went to each unit
        List<List<Double>> fracs = new ArrayList<>(nUnits); // how much of each VL goes
        for (int i = 0; i < nUnits; i++) {
            plans.add(new ArrayList<>(1));
            fracs.add(new ArrayList<>(1));
        }

        int bestUnit = 1;
        int bestVL = 1;
        double bestFrac = 1.0;

        List<VehicleLoad> vlList = new ArrayList<>(vls.keySet());
        while ((0 <= bestUnit) && (0 <= bestVL)) {
            Tuple3<Integer, Integer, Double> b = rsm.bestUnitToLoad(vls);
            bestUnit = b.get0();
            bestVL = b.get1();
            bestFrac = b.get2();
            if ((0 <= bestUnit) || (0 <= bestVL)) {
                assertTrue(0 <= bestUnit);
                assertTrue(0 <= bestVL);
                assertTrue(bestVL < vlList.size());
                assertTrue(0.0 <= bestFrac);
                assertTrue(bestFrac <= 1.0);
                System.out.printf("Best Unit: %2d, Best VL: %3d, BestFrac: %5.3f \n",
                        bestUnit, bestVL, bestFrac);
                List<Integer> m = plans.get(bestUnit);
                List<Double> f = fracs.get(bestUnit);
                m.add(bestVL);
                f.add(bestFrac);
                plans.set(bestUnit, m);
                fracs.set(bestUnit, f);
                VehicleLoad vl = vlList.get(bestVL); 
                rsm.applyVehicleLoad(bestUnit, vl, vls.get(vl), bestFrac);
                rsm.showAvailable();
            } else {
                System.out.println("No more feasible and useful deliveries.\n");
            }
        }
        System.out.println("Prioritized fulfillment value of plan");
        rsm.showUnitSupplies();

        for (int i = 0; i < nUnits; i++) {
            int m = plans.get(i).size();
            System.out.printf("Unit %2d, %8s, will achieve value %8.4f and receive %3d vehicle-loads:  ",
                    i, rsm.getUnitName(i), rsm.prioritizedUnitValue(i), m);
            for (int j = 0; j < m; j++) {
                double f = fracs.get(i).get(j);
                if (f < 1.0) {
                    System.out.printf(" (%2d, %5.3f)", plans.get(i).get(j), f);
                } else {
                    System.out.printf(" %2d", plans.get(i).get(j));
                }
            }
            System.out.println();
        }
    }

    // how much of each type of supply is available: fuel, ammo, water, food
    private final List<Double> saExcess = new ArrayList<>(Arrays.asList(9300.0, 7350.0, 2100.0, 1100.0)); // slight excess of everything
    private final List<Double> saFuel = new ArrayList<>(Arrays.asList(2790.0, 7350.0, 2100.0, 1100.0)); // only short of fuel
    private final List<Double> saAmmo = new ArrayList<>(Arrays.asList(9300.0, 0.0, 2100.0, 1100.0)); // only short of ammo
    private final List<Double> saWater = new ArrayList<>(Arrays.asList(9300.0, 7350.0, 0.0, 1100.0)); // only short of water
    private final List<Double> saFood = new ArrayList<>(Arrays.asList(9300.0, 7350.0, 2100.0, 330.0)); // only short of food
    private final List<Double> saAll = new ArrayList<>(Arrays.asList(3000.0, 5100.0, 1000.0, 850.0)); // short of all supplies
    private final List<Double> saNothing = new ArrayList<>(Arrays.asList(0.0, 0.0, 0.0, 0.0)); // no supplies

    private VehicleResupplyMatrix makeCase(List<Double> sAvailable) {
        List<String> uNames = new ArrayList<>(4);
        uNames.add("Unit-A");
        uNames.add("Unit-B");
        uNames.add("Unit-C");
        uNames.add("Unit-D");

        List<Double> uPriority = new ArrayList<>(uNames.size());
        uPriority.add(10.0);
        uPriority.add(8.0);
        uPriority.add(5.0);
        uPriority.add(1.0);

        List<Double> uDistance = new ArrayList<>(4);
        uDistance.add(215.0);
        uDistance.add(172.0);
        uDistance.add(55.0);
        uDistance.add(118.0);

        List<Double> sPriority = new ArrayList<>(4);
        sPriority.add(11.0); // fuel
        sPriority.add(8.0);  // ammo
        sPriority.add(4.0);  // water
        sPriority.add(3.0);  // food
        List<String> supplyTypes = new ArrayList<>(4);
        supplyTypes.add("fuel");
        supplyTypes.add("ammo");
        supplyTypes.add("water");
        supplyTypes.add("food");

        List<VehicleType> vehicleTypes = makeVehicleListA() ;
        int numVehicles = vehicleTypes.size();

        Map<String, Double> vehicleFrac = new HashMap<>(numVehicles);
        for (VehicleType v : vehicleTypes){
            vehicleFrac.put(v.vehicleTypeName, 1.0);
        }

        // 'vehicleCount' records the count of each type
        // of vehicle. For example,
        // vehicleCount.get(6) is 100 because we have 100 vehicleCount of type 6.
        Map<String, Integer> vehicleCount = new HashMap<>(numVehicles);
        vehicleCount.put(vehicleTypes.get(0).vehicleTypeName, 3);  // rail 00, 3000 tons fuel
        vehicleCount.put(vehicleTypes.get(1).vehicleTypeName, 4);  // rail 01, 1000 tons fuel
        vehicleCount.put(vehicleTypes.get(2).vehicleTypeName, 2);  // rail 02, 3000 tons water
        vehicleCount.put(vehicleTypes.get(3).vehicleTypeName, 2);  // rail 03, 1000 tons water
        vehicleCount.put(vehicleTypes.get(4).vehicleTypeName, 2);  // rail 04, 4000 tons ammo or food
        vehicleCount.put(vehicleTypes.get(5).vehicleTypeName, 2);  // rail 05, 2000 tons ammo or food

        vehicleCount.put(vehicleTypes.get(6).vehicleTypeName, 100);   // truck 06, 20 ton fuel
        vehicleCount.put(vehicleTypes.get(7).vehicleTypeName, 100);   // truck 07, 10 ton fuel
        vehicleCount.put(vehicleTypes.get(8).vehicleTypeName, 100);   // truck 08, 2 ton anything
        vehicleCount.put(vehicleTypes.get(9).vehicleTypeName, 100);   // truck 09, 20 ton water
        vehicleCount.put(vehicleTypes.get(10).vehicleTypeName, 100);   // truck 10, 10 ton water
        vehicleCount.put(vehicleTypes.get(11).vehicleTypeName, 100);   // truck 11, 20 ton ammo, food or mix
        vehicleCount.put(vehicleTypes.get(12).vehicleTypeName, 100);   // truck 12, 15 ton ammo, food or mix
        vehicleCount.put(vehicleTypes.get(13).vehicleTypeName, 100);   // truck 13, 10 ton ammo, food or mix

        VehicleResupplyMatrix rsm = new VehicleResupplyMatrix(
                uPriority,
                uDistance,
                uNames,
                sPriority,
                vehicleTypes,
                vehicleCount,
                vehicleFrac,
                supplyTypes,
                sAvailable,
                0 // index to which supply is fuel
        );

        // some need nothing, some have nothing: both kinds of zeros will be checked.
        double[][] u = {
            {192.0, 271.0, 120.0, 37.0},
            {36.0, 43.0, 12.0, 0.0},
            {3881.0, 5457.0, 1756.0, 976.0},
            {3.0, 4.0, 1.0, 1.0}
        };

        rsm.setUnfilled(u);

        double[][] t = {
            {255.00, 360.00, 120.00, 60.00},
            {45.0, 64.0, 21.0, 10.80},
            {5100.00, 7200.00, 2400.00, 1200.00},
            {4, 5.0, 2.0, 1.0}
        };

        rsm.setTotal(t);

        rsm.setPrioritizedValue();

        return rsm;
    }

    private VehicleResupplyMatrix makeCaseWater() {
        return makeCase(saWater);
    }

    /**
     * These values are from the 'prioritized-loads.xlsx' spreadsheet
     * @return
     */
    private List<VehicleType> makeVehicleListA() {
        List<String> vehicleTypes = new ArrayList<>(14);
        vehicleTypes.add("rail 00");
        vehicleTypes.add("rail 01");
        vehicleTypes.add("rail 02");
        vehicleTypes.add("rail 03");
        vehicleTypes.add("rail 04");
        vehicleTypes.add("rail 05");
        vehicleTypes.add("truck 06");
        vehicleTypes.add("truck 07");
        vehicleTypes.add("truck 08");
        vehicleTypes.add("truck 09");
        vehicleTypes.add("truck 10");
        vehicleTypes.add("truck 11");
        vehicleTypes.add("truck 12");
        vehicleTypes.add("truck 13");
        List<VehicleType> vehicleTypeList = new ArrayList<>();
        for(String vehicleType : vehicleTypes) {
            VehicleType vt = new VehicleType();
            vt.vehicleTypeName = vehicleType;
            vt.fuelType = "fuel";
            if (vehicleType.contains("truck")) {
                vt.availability = 0.75;
                vt.fuelEfficiency = 0.5; // fuel / KM
                vt.maxRange = 500.0; // KM
            }
            else { // must be rail
                vt.availability = 0.95;
                vt.fuelEfficiency = 0.25; // fuel / KM
                vt.maxRange = 1000.0; // KM
            }
            vt.maxSpeed = 100.0; // KM/hr
            vt.domains = new HashSet<>(1);
            vt.domains.add("Land");
            vehicleTypeList.add(vt);
        }
        return vehicleTypeList;
    }

    private LinkedHashMap<VehicleLoad, VehicleType> makeVLListA() {
        LinkedHashMap<VehicleLoad, VehicleType> vls = new LinkedHashMap<>(); 
        List<String> sTypes = new ArrayList<>(4);
        sTypes.add("fuel");
        sTypes.add("ammo");
        sTypes.add("water");
        sTypes.add("food");

        int numberSupplyTypes = sTypes.size();
        VehicleLoad vl;
        VehicleType vt;
        List<VehicleType> vehicleTypes = makeVehicleListA() ;
        // ---------------------
        //        FUEL
        // ---------------------
        // Fuel trains
        vl = new VehicleLoad(sTypes); // 00
        vt = vehicleTypes.get(0);
        vt.fuelEfficiency = 0.40;
        vl.load.set(0, 3000.0); // fuel
        vl.name = "VLC-00";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 01
        vt = vehicleTypes.get(1);
        vt.fuelEfficiency = 1.00;
        vl.load.set(0, 1000.0); // fuel
        vl.name = "VLC-01";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        // Fuel trucks
        vl = new VehicleLoad(sTypes); // 02
        vt = vehicleTypes.get(6);
        vt.fuelEfficiency = 5.00;
        vl.load.set(0, 20.0); // fuel
        vl.name = "VLC-02";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 03
        vt = vehicleTypes.get(7);
        vt.fuelEfficiency = 8.00;
        vl.load.set(0, 10.0); // fuel
        vl.name = "VLC-03";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 04
        vt = vehicleTypes.get(8);
        vt.fuelEfficiency = 15.00;
        vl.load.set(0, 2.0); // fuel
        vl.name = "VLC-04";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        // ---------------------
        //       WATER
        // ---------------------
        // Water trains
        vl = new VehicleLoad(sTypes); // 05
        vt = vehicleTypes.get(2);
        vt.fuelEfficiency = 0.40;
        vl.load.set(2, 3000.0); // water
        vl.name = "VLC-05";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 06
        vt = vehicleTypes.get(3);
        vt.fuelEfficiency = 1.00;
        vl.load.set(2, 1000.0); // water
        vl.name = "VLC-06";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        // Water trucks
        vl = new VehicleLoad(sTypes); // 07
        vt = vehicleTypes.get(9);
        vt.fuelEfficiency = 5.00;
        vl.load.set(2, 20.0); // water
        vl.name = "VLC-07";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 08
        vt = vehicleTypes.get(10);
        vt.fuelEfficiency = 8.00;
        vl.load.set(2, 10.0); // water
        vl.name = "VLC-08";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 09
        vt = vehicleTypes.get(8);
        vt.fuelEfficiency = 15.00;
        vl.load.set(2, 2.0); // water
        vl.name = "VLC-09";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        // ---------------------
        //       AMMO
        // ---------------------
        // Ammo trains
        vl = new VehicleLoad(sTypes); // 10
        vt = vehicleTypes.get(4);
        vt.fuelEfficiency = 0.32;
        vl.load.set(1, 4000.0); // ammo
        vl.name = "VLC-10";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 11
        vt = vehicleTypes.get(5);
        vt.fuelEfficiency = 0.55;
        vl.load.set(1, 2000.0); // ammo
        vl.name = "VLC-11";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        // Ammo trucks
        vl = new VehicleLoad(sTypes); // 12
        vt = vehicleTypes.get(11);
        vt.fuelEfficiency = 5.00;
        vl.load.set(1, 20.0); // ammo
        vl.name = "VLC-12";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 13
        vt = vehicleTypes.get(12);
        vt.fuelEfficiency = 5.50;
        vl.load.set(1, 15.0); // ammo
        vl.name = "VLC-13";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 14
        vt = vehicleTypes.get(13);
        vt.fuelEfficiency = 6.00;
        vl.load.set(1, 10.0); // ammo
        vl.name = "VLC-14";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 15
        vt = vehicleTypes.get(8);
        vt.fuelEfficiency = 15.00;
        vl.load.set(1, 2.0); // ammo
        vl.name = "VLC-15";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        // ---------------------
        //       FOOD
        // ---------------------
        // Food trains
        vl = new VehicleLoad(sTypes); // 16
        vt = vehicleTypes.get(4);
        vt.fuelEfficiency = 0.32;
        vl.load.set(3, 4000.0); // food
        vl.name = "VLC-16";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 17
        vt = vehicleTypes.get(5);
        vt.fuelEfficiency = 0.55;
        vl.load.set(3, 2000.0); // food
        vl.name = "VLC-17";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        // Food trucks
        vl = new VehicleLoad(sTypes); // 18
        vt = vehicleTypes.get(11);
        vt.fuelEfficiency = 5.00;
        vl.load.set(3, 20.0); // food
        vl.name = "VLC-18";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 19
        vt = vehicleTypes.get(12);
        vt.fuelEfficiency = 5.50;
        vl.load.set(3, 15.0); // food
        vl.name = "VLC-19";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 20
        vt = vehicleTypes.get(13);
        vt.fuelEfficiency = 6.00;
        vl.load.set(3, 10.0); // food
        vl.name = "VLC-20";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 21
        vt = vehicleTypes.get(8);
        vt.fuelEfficiency = 15.00;
        vl.load.set(3, 2.0); // food
        vl.name = "VLC-21";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        // ---------------------
        //     AMMO + FOOD
        // ---------------------
        // Ammo+Food trains
        vl = new VehicleLoad(sTypes); // 22
        vt = vehicleTypes.get(4);
        vt.fuelEfficiency = 0.32;
        vl.load.set(1, 2000.0); // ammo
        vl.load.set(3, 2000.0); // food
        vl.name = "VLC-22";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 23
        vt = vehicleTypes.get(5);
        vt.fuelEfficiency = 0.55;
        vl.load.set(1, 1000.0); // ammo
        vl.load.set(3, 1000.0); // food
        vl.name = "VLC-23";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        // Ammo+Food trucks
        vl = new VehicleLoad(sTypes); // 24
        vt = vehicleTypes.get(11);
        vt.fuelEfficiency = 5.00;
        vl.load.set(1, 10.0); // ammo
        vl.load.set(3, 10.0); // food
        vl.name = "VLC-24";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 25
        vt = vehicleTypes.get(12);
        vt.fuelEfficiency = 5.50;
        vl.load.set(1, 8.0); // ammo
        vl.load.set(3, 7.0); // food
        vl.name = "VLC-25";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 26
        vt = vehicleTypes.get(13);
        vt.fuelEfficiency = 6.00;
        vl.load.set(1, 5.0); // ammo
        vl.load.set(3, 5.0); // food
        vl.name = "VLC-26";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        vl = new VehicleLoad(sTypes); // 27
        vt = vehicleTypes.get(8);
        vt.fuelEfficiency = 15.00;
        vl.load.set(1, 1.0); // ammo
        vl.load.set(3, 1.0); // food
        vl.name = "VLC-27";
        vl.vehicleType = vt.vehicleTypeName;
        vls.put(vl,vt);

        return vls;
    }

}

// =============================================================================
