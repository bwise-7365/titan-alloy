/*
 *  ---------------------------------------------------
 *         Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 * 
 */
package groupw.DCVRP;

import org.jgrapht.alg.clique.BaseBronKerboschCliqueFinderTest;
import org.junit.Test;

import java.util.*;

import static groupw.DCVRP.ItineraryBuilder.TheIB;
import static groupw.DCVRP.ReadDCVRScenarioCSV.*;
import static groupw.DCVRP.VRController.TheVRC;
import static groupw.Network.NWUtils.DefaultSeedPRNG;
import static java.lang.Math.abs;

/**
 *
 * @author BenWise
 */
public class VRControllerTest {

    public VRControllerTest() {
    }

    @Test
    public void testMatchSerialsToBacklogs() {
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readScenarioC1(); // readStandardTestCase(); // readHighCapacityTestCase();
        int sd = DefaultSeedPRNG;
        sd = 259522115; // 0;
        VRController.initialize(sRec, sd);
        ItineraryBuilder.initialize();
        // the current time is chosen so that some of the high-priority serials
        // (average transit time suggests they might be late from current location)
        // can get there on time using some edges but will be late using others.
        double currTime = 43.50; // hours
        boolean useMinTimeP = false; // use the average

        // In standard test case, all serials start in Cuba: Havana , Juragua , Palma , Santiago
        String hbName = "Juragua";

        // I happen to know the expected results for the standard test case and this PRNG-Seed
        Map<String, Integer> expectedNumSerials = new HashMap<>();
        expectedNumSerials.put("Havana", 1158);
        expectedNumSerials.put("Juragua", 1138);
        expectedNumSerials.put("Palma", 683);
        expectedNumSerials.put("Santiago", 683);


        boolean randomTransportOrder = true;
        boolean randomSerialOrder = true;
        List<String> transportNames = TheVRC.transportsAtHomeBase(hbName, randomTransportOrder);
        List<String> serialNames = TheVRC.serialsAtNode(hbName, randomSerialOrder);
        Map<String, Backlog> vehicleBacklogMap = TheVRC.matchSerialsToBacklogs(serialNames, transportNames, TheIB, currTime, useMinTimeP);


        assert (null != vehicleBacklogMap);

        int numR = 0;
        for (Map.Entry<String, Backlog> e : vehicleBacklogMap.entrySet()) {
            Backlog b = e.getValue();
            int nt = b.numTrips();
            int nr = b.numReservations();
            numR = numR + nr;
            System.out.printf("Transport %14s has %3d reservations for %3d trips\n",
                    e.getKey(), nr, nt);
        }
        assert (expectedNumSerials.get(hbName) == numR);
    }

    @Test
    public void testRandomBacklog() {
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readStandardTestCase(); // readStandardTestCase OR readHighCapacityTestCase
        int sd = 259522115; // 0, DefaultSeedPRNG;
        VRController.initialize(sRec, sd);
        String hbName = "Havana";
        Map<String, Backlog> vehicleBacklogMap = TheVRC.makeRandomBacklogs(hbName);
        for (Map.Entry<String, Backlog> entry : vehicleBacklogMap.entrySet()) {
            Backlog b = entry.getValue();
            int nr = b.numReservations();
            double artt = b.averageRTT(nr - 1);
            double aTotal = b.totalArea(nr - 1);
            double wTotal = b.totalWeight(nr - 1);
            int nTrips = b.numTrips();
            System.out.printf("Vehicle %12s from %8s has backlog of %3d serials for %2d trips",
                    entry.getKey(), hbName, nr, nTrips);
            System.out.printf(" with average RTT %5.1f hours, A %5.1f sqr ft, W %7.1f lbs\n",
                    artt, aTotal / nr, wTotal / nr);
        }

        ItineraryBuilder.initialize();
        // the current time is chosen so that some of the high-priority serials
        // (average transit time suggests they might be late from current location)
        // can get there on time using some edges but will be late using others.
        double currTime = 43.50; // hours
        boolean useMinTimeP = false; // use the average

        for (Map.Entry<String, Backlog> entry : vehicleBacklogMap.entrySet()) {
            String vehicleName = entry.getKey();

            final String vehicleType = TheVRC.getVehicleDataMap().get(vehicleName).type;
            final ReadTransportTypeCSV.DataField vtRec = TheVRC.getVehicleTypeMap().get(vehicleType);
            Backlog b = entry.getValue();
            Itinerary it = TheIB.itineraryFromBacklog(vehicleName, b, currTime, useMinTimeP);
            TheIB.setTimeTable(it, vehicleName, currTime);
            double twd = it.totalWeightDistance(TheVRC.getSerialMap());
            System.out.printf("Itinerary for %s, WD=%.4E: %s\n", vehicleName, twd, it.listLegNodes());
            System.out.printf("Itinerary ends at %.5f\n", it.finalDropOffTime());
            //it.displayManifests();
            Itinerary it2 = TheIB.reorderCircularItinerary(it, vtRec);
            if (null != it2) {
                TheIB.setTimeTable(it2, vehicleName, currTime);
                double twd2 = it2.totalWeightDistance(TheVRC.getSerialMap());
                System.out.printf("Reordered for %s, WD=%.4E: %s\n", vehicleName, twd2, it2.listLegNodes());

                // This will pass only for the regular capacity test case, seed == 259522115
                if (5 == it2.numLegs()) {
                    double endTime = it2.finalDropOffTime();
                    double expEndTime = 199.002;
                    double timeTol = 0.001;
                    System.out.printf("Reordered 5-leg itinerary ends at %.5f\n", endTime);
                    assert(abs(endTime - expEndTime) < timeTol);
                }
            }
            System.out.println("");
            System.out.flush();
        }

        return;
    }

    @Test
    public void testResetHomeBase() {
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readStandardTestCase(); // readHighCapacityTestCase();
        int sd = 259522115; // 0, DefaultSeedPRNG;
        VRController.initialize(sRec, sd);
        String vName = "LST-04-HVN";
        String homeBase1 = "Havana";
        String homeBase2 = "Dominica";

        ReadTransportVehicleCSV.DataField vr1 = TheVRC.getVehicleDataMap().get(vName);
        assert(homeBase1.equals(vr1.homeBase));
        int s1 = TheVRC.getVehicleDataMap().size();
        assert(130 == s1);

        TheVRC.resetHomeBase(vName, homeBase2);
        ReadTransportVehicleCSV.DataField vr2 = TheVRC.getVehicleDataMap().get(vName);
        assert(homeBase2.equals(vr2.homeBase));
        int s2 = TheVRC.getVehicleDataMap().size();
        assert(130 == s2);

        System.out.flush();
    }


    @Test
    public void testPortAccess() {
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readStandardTestCase(); // readHighCapacityTestCase();
        int sd = 259522115; // 0, DefaultSeedPRNG;
        VRController.initialize(sRec, sd);
        String homeBase1 = "Dominica";

        Set<String> pa1 = TheVRC.getPortAccessMap().get(homeBase1);
        int s1 = TheVRC.getPortAccessMap().size();
        System.out.printf("%s \n", pa1.toString());
        assert(18 == s1);
        assert(4 == pa1.size());

        Set<String> pta = new HashSet<>();
        pta.add("Planes");
        pta.add("Trains");
        pta.add("Automobiles");

        TheVRC.resetPortAccess(homeBase1, pta);

        Set<String> pa2 = TheVRC.getPortAccessMap().get(homeBase1);
        int s2 = TheVRC.getPortAccessMap().size();
        System.out.printf("%s \n", pa2.toString());
        assert(3 == pa2.size());
        assert(18 == s2);
        System.out.flush();
    }
}

// =============================================================================
