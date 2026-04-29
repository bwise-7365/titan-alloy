// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import groupw.DCVRP.VRGraph.VREdge;
import groupw.DCVRP.VRGraph.VRNode;
import groupw.Network.NWUtils;
import org.junit.Test;

import java.util.List;

import static groupw.DCVRP.ItineraryBuilder.TheIB;
import static groupw.DCVRP.ReadDCVRScenarioCSV.initWithFileChooser;
import static groupw.DCVRP.ReadDCVRScenarioCSV.readStandardTestCase;
import static groupw.DCVRP.ReadDCVRScenarioCSV.readHighCapacityTestCase;
import static groupw.DCVRP.VRController.TheVRC;
import static groupw.Network.NWUtils.DefaultSeedPRNG;
import static java.lang.Math.abs;
import static java.lang.Math.sqrt;
import static org.junit.Assert.*;

public class ItineraryBuilderTest {


    public ItineraryBuilderTest() {
    }

    @Test
    public void testPotentialIntermediateEdges() {
        boolean useDialogP = true;
        ReadDCVRScenarioCSV.ScenarioRecord sRec = useDialogP ? initWithFileChooser() : readStandardTestCase();
        VRController.initialize(sRec, DefaultSeedPRNG);
        ItineraryBuilder.initialize();

        String transportName = "KC130-01-HVN";
        String serialName = "9-INF-BDE-HMMWV-017";
        double currTime = 0.0;

        List<VREdge> potentialEdges = TheIB.potentialIntermediateEdges(transportName, serialName, currTime);

        int numPotentialEdges = potentialEdges.size();
        System.out.printf("Number of potential edges: %d\n", numPotentialEdges);
        assertTrue(9 == numPotentialEdges); // known result for this test case
        return;
    }

    @Test
    public void testEstTransitTime() {
        double expMinHours = 2.849; // known answer for the standard
        double expAvrgHours = 24.854; // known answer for the standard
        double timeTol = 0.01; // hours

        int expectedNumSorted = 903; // known result for this test data

        double expectedS1 = 1.90298 ; // expected mean estQWL
        double expectedS2 = 5.03498 ; // expected RMS estQEWL
        double sumTol = 0.00001;

        boolean useDialogP = false;
        ReadDCVRScenarioCSV.ScenarioRecord sRec = useDialogP ? initWithFileChooser() : readStandardTestCase();

        int sd = DefaultSeedPRNG;
        //sd = 0;
        VRController.initialize(sRec, sd);
        ItineraryBuilder.initialize();
        VRNode srcNode = TheVRC.getNodeMap().get("Havana");
        VRNode tgtNode = TheVRC.getNodeMap().get("Esperanza");
        assertNotNull(srcNode);
        assertNotNull(tgtNode);
        //String unitName = "9-INF-BDE";
        String serialName = "9-INF-BDE-HMMWV-017"; // a vehicle that fits on KC-130
        Serial s = TheVRC.getSerialMap().get(serialName);

        double minHours = TheIB.estMinTransitTime(s, srcNode, tgtNode);
        double avrgHours = TheIB.estAverageTransitTime(s, srcNode, tgtNode);
        assertTrue(abs(minHours - expMinHours) < timeTol);
        assertTrue(abs(avrgHours - expAvrgHours) < timeTol);

        String vName = "KC130-02-HVN";
        //ReadTransportVehicleCSV.DataField vRec = TheVRC.getVehicleDataMap().get(vName);
        //String transType =  vRec.type; //  TheVRC.getVehicleDataMap().get(vName);
        double currTime = 23.5; // hours, late enough to make some serials arrive late
        boolean useMinTimeP = false; // use the average
        TheVRC.getVehicleDataMap(); // initialize it
        List<NWUtils.Tuple2<String, Double>> sortedSerials = TheIB.sortHomeBaseSerialsByPrioritizedEstQWL(vName, currTime, useMinTimeP);
        assertEquals(sortedSerials.size(), expectedNumSorted);

        double sum1 = 0.0;
        double sum2 = 0.0;
        for (int i = 0; i < sortedSerials.size(); i++) {
            NWUtils.Tuple2<String, Double> s1 = sortedSerials.get(i);
            sum1 = sum1 + s1.get1();
            sum2 = sum2 + (s1.get1() * s1.get1());
            if (i < 50) {
                System.out.printf("%4d: %22s  %.4f \n", i, s1.get0(), s1.get1());
            }
            if (0 < i) {
                assertTrue(s1.get1() <= sortedSerials.get(i - 1).get1());
            }
        }
        sum1 = sum1 / sortedSerials.size();
        sum2 = sqrt(sum2 / sortedSerials.size());
        assert (abs(sum1 - expectedS1) < sumTol);
        assert (abs(sum2 - expectedS2) < sumTol);

        return;
    }

    @Test
    public void testBuildHomeBaseItinerary() {
        ItemCounter.reset(1000);
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readHighCapacityTestCase();
        int sd = DefaultSeedPRNG;
        //sd = 0;
        VRController.initialize(sRec, sd);
        ItineraryBuilder.initialize();

        // These might some interesting vehicles (homebase) to try
        // KC130-02-HVN, KC-130, Havana
        // LSM-04-HVN, LSM, Havana
        // KC130-12-PLT, KC-130, Plata
        // CH53-05-PLT, CH-53K, Plata
        //
        // MV22-12-GDL, MV-22, Guadeloupe
        // MV22-15-USV, MV-22, USVI
        //
        final String vehicleName = "KC130-02-HVN"; // absurdly large capacity
        final String vehicleType = TheVRC.getVehicleDataMap().get(vehicleName).type;
        final ReadTransportTypeCSV.DataField vtRec = TheVRC.getVehicleTypeMap().get(vehicleType);

        // the current time is chosen so that some of the high-priority serials
        // (average transit time suggests they might be late from current location)
        // can get there on time using some edges but will be late using others.
        double currTime = 43.50; // hours
        boolean useMinTimeP = false; // use the average

        Itinerary it = TheIB.buildHomeBaseItinerary(vehicleName, currTime, useMinTimeP);
        double twd = it.totalWeightDistance(TheVRC.getSerialMap());
        System.out.printf("Created itinerary   %d, WD=%.4E: %s\n", it.getID(), twd, it.listLegNodes());
        TheIB.setTimeTable(it, vehicleName, currTime);
        //it.displayManifests();

        assert (it.checkWellFormed());
        assert (it.checkFeasible(vtRec,
                TheVRC.getSerialMap(),
                TheVRC.getVehicleDomainMap(),
                TheVRC.getPortAccessMap()));

        Itinerary it2 = TheIB.reorderCircularItinerary(it, vtRec);
        if (null == it2){
            System.out.println("No reordering produced.");
        }
        else {
            double twd2 = it2.totalWeightDistance(TheVRC.getSerialMap());
            System.out.printf("Reordered itinerary %d, WD=%.4E: %s\n", it2.getID(), twd2, it2.listLegNodes());
            // the only reason to reorder is to reduce total weight-distance,
            // and sometimes reordering just recreates the same plan
            assert (twd2 <= twd);
            assert (it2.checkWellFormed());
            assert (it2.checkFeasible(vtRec,
                    TheVRC.getSerialMap(),
                    TheVRC.getVehicleDomainMap(),
                    TheVRC.getPortAccessMap()));
        }
        return;
    }


}



// =============================================================================
