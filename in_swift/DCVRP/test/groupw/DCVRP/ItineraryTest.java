// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import groupw.DCVRP.VRGraph.VREdge;
import groupw.DCVRP.VRGraph.VRNode;
import groupw.Logistics.Manifest;
import org.junit.Test;

import java.util.Map;
import java.util.Set;

import static groupw.DCVRP.ReadDCVRScenarioCSV.readStandardTestCase;
import static java.lang.Math.abs;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertTrue;

/**
 *
 * @author BenWise
 */
public class ItineraryTest {

    public ItineraryTest() {
    }

    /**
     * Test a specific hard-coded Itinerary, using only CSV files known to be good.
     */
    @Test
    public void testItineraryMethod_V00() {
        System.out.printf("Running testItineraryMethod_V00 \n");

        ItemCounter.reset(1000);
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readStandardTestCase();
        VRGraph g = sRec.vrg;
        Map<String, ReadUnitCSV.DataField> uMap = ReadUnitCSV.makeUnitMap(sRec.unitRecords);
        Map<String, ReadTransportTypeCSV.DataField> vtMap = ReadTransportTypeCSV.makeVTypeMap(sRec.vtRecords);
        Map<String, ReadTransportVehicleCSV.DataField> vMap = ReadTransportVehicleCSV.makeVehicleDataMap(sRec.vRecords);
        Map<String, Set<String>> paMap = ReadPortAccessCSV.makePortAccessMap(sRec.paRecords);
        Map<String, Serial> sMap = ReadSerialCSV.makeSerialMap(sRec.sRecords, uMap);
        Map<String, Set<String>> tdMap = ReadTransportDomainCSV.makeTransportDomainMap(sRec.tdRecords);

        // count all the bits to make sure we got the numbers expected of standard (verified) test files
        Map<String, VRNode> nodeMap = g.makeNodeMap();
        Map<String, VREdge> edgeMap = g.makeEdgeMap();
        assertTrue(18 == nodeMap.size());
        assertTrue(612 == edgeMap.size());
        assertTrue(17 == uMap.size());
        assertTrue(4282 == sMap.size());
        assertTrue(130 == vMap.size());
        assertTrue(5 == vtMap.size());
        assertTrue(18 == paMap.size());
        assertTrue(7 == tdMap.size());


        // Two Battalions start in Havana and need to go to far Eastern islands.
        // Both are due in 120 hours, or 5 days. Each has 90 serials of Squads, and only Squads.
        // The battalion has 450 serials of Squads and 538 Serials of equipment
        // 413-INF-BN, Havana, 1030, 8.0 , 120.0 , 12.0, Lucia
        // 217-INF-BN, Havana, 1040, 8.0 , 120.0 , 12.0, Vincent
        // 9-INF-BDE, Havana, 1140, 5.0 , 96.0 , 12.0, Dominica
        // Example equipment (KC-130 carries 42,000 lbs in 410 square feet):
        // 9-INF-BDE-SQD-0030, 1498, 9-INF-BDE, 70.8, 3840.0, 0.0889
        // 9-INF-BDE-JLTV-020, 1938, 9-INF-BDE, 200.0, 22500.0, 0.1230
        // 9-INF-BDE-HMMWV-017, 2010, 9-INF-BDE, 128.0, 8000.0, 0.1230
        // 9-INF-BDE-MTBR-042, 2185, 9-INF-BDE, 250.0, 49100.0, 0.1230
        // 9-INF-BDE-LAV-25-002, 2395, 9-INF-BDE, 200.0, 32000.0, 0.1230
        // 9-INF-BDE-M777-002, 2400, 9-INF-BDE, 340.0, 9300.0, 0.1230
        // 9-INF-BDE-Viper-004, 2405, 9-INF-BDE, 650.0, 14000.0, 0.1230

        // The next section builds and checks one simple Itinerary

        // This test itinerary will be for this particular KC130 from Havana.
        // Havana -> Palma -> Plata -> Esperanza -> Havana
        String transportName = "KC130-01-HVN";
        ReadTransportVehicleCSV.DataField vRec = vMap.get(transportName);
        assertNotNull(vMap.get(transportName));
        ReadTransportTypeCSV.DataField vtRec = vtMap.get(vRec.type);

        // A real itinerary builder would have to decide separately which
        // domain to use on each edge, given that a vehicle might traverse
        // multiple domains. See "Two-Ton-Truck"
        // For this initial test, I know these files give
        // only one domain per USED vehicle type,
        // so I take one element from this one-element set.
        Set<String> vDomains = tdMap.get(vtRec.type);
        assert (1 == vDomains.size());
        String vDomain = "";
        for (String d : vDomains) {
            vDomain = d;
        }

        // Notice that we avoid shared structure:
        // Stop at dst of LegA is not the Stop at src of LegB
        Itinerary.Stop srcA = new Itinerary.Stop(nodeMap.get("Havana"));
        Itinerary.Stop dstA = new Itinerary.Stop(nodeMap.get("Palma"));

        Itinerary.Stop srcB = new Itinerary.Stop(nodeMap.get("Palma"));
        Itinerary.Stop dstB = new Itinerary.Stop(nodeMap.get("Plata"));

        Itinerary.Stop srcC = new Itinerary.Stop(nodeMap.get("Plata"));
        Itinerary.Stop dstC = new Itinerary.Stop(nodeMap.get("Esperanza"));

        Itinerary.Stop srcD = new Itinerary.Stop(nodeMap.get("Esperanza"));
        Itinerary.Stop dstD = new Itinerary.Stop(nodeMap.get("Havana"));

        Itinerary.Leg legA = new Itinerary.Leg(srcA, dstA, vDomain, g);
        Itinerary.Leg legB = new Itinerary.Leg(srcB, dstB, vDomain, g);
        Itinerary.Leg legC = new Itinerary.Leg(srcC, dstC, vDomain, g);
        Itinerary.Leg legD = new Itinerary.Leg(srcD, dstD, vDomain, g);

        Itinerary itnry = new Itinerary(g);
        System.out.printf("Built itinerary %d\n", itnry.getID());

        itnry.append(legA);
        itnry.append(legB);
        itnry.append(legC);
        itnry.append(legD);

        System.out.printf("Nodes visited: %s \n", itnry.visitedNodes());
        System.out.printf("Nodes in Legs: %s \n", itnry.listLegNodes());

        // verify that this no-cargo itinerary fits in very tight space and weight limits
        assertTrue(itnry.checkWellFormed());
        boolean itFits00 = itnry.checkAreaWeightLimits(vtRec.cargoArea / 100.0, vtRec.cargoWeight / 100.0, sMap);
        assertTrue(itFits00);
        double itLength01 = itnry.getLength();System.out.printf("Itinerary has %d legs and is %.4f nautical miles long\n",
                itnry.legs.size(), itLength01);
        assertTrue(abs(itLength01 - 2046.1230) < 0.001); // known length


        final double oneSerial = 1.0;

        Serial serial01 = sMap.get("9-INF-BDE-SQD-0030");
        Serial serial02 = sMap.get("9-INF-BDE-HMMWV-017");
        Serial serial03 = sMap.get("9-INF-BDE-JLTV-020");
        Serial serial04 = sMap.get("9-INF-BDE-LAV-25-002");

        // pickup three in Havana.
        legA.src.transfer = new Manifest();
        legA.src.transfer.addInventory(serial01.name, oneSerial);
        legA.src.transfer.addInventory(serial02.name, oneSerial);
        legA.src.transfer.addInventory(serial03.name, oneSerial);

        // Drop off two in Palma
        legA.dst.transfer = new Manifest();
        legA.dst.transfer.addInventory(serial02.name, oneSerial);
        legA.dst.transfer.addInventory(serial03.name, oneSerial);

        // Pickup one in Palma
        legB.src.transfer = new Manifest();
        legB.src.transfer.addInventory(serial04.name, oneSerial);

        // Drop both in Esperanza, fly back empty
        legC.dst.transfer = new Manifest();
        legC.dst.transfer.addInventory(serial01.name, oneSerial);
        legC.dst.transfer.addInventory(serial04.name, oneSerial);

        boolean itFits01 = itnry.checkAreaWeightLimits(vtRec.cargoArea / 10.0, vtRec.cargoWeight / 10.0, sMap);
        assertTrue(!itFits01);
        boolean itFits02 = itnry.checkAreaWeightLimits(vtRec.cargoArea, vtRec.cargoWeight, sMap);
        assertTrue(itFits02);

        int numUndefined = itnry.undefinedCargo(sMap).size();
        assert (0 == numUndefined);

        boolean feasible01 = itnry.checkFeasible(vtRec, sMap, tdMap, paMap);
        assert (feasible01);

        itnry.spliceOneLeg(2,
                nodeMap.get("IslaDeMona"),
                "Air", null, null);

        System.out.printf("Nodes visited: %s \n", itnry.visitedNodes());
        System.out.printf("Nodes in Legs: %s \n", itnry.listLegNodes());

        assertTrue(itnry.checkWellFormed());
        double itLength02 = itnry.getLength();
        System.out.printf("Itinerary %d has %d legs and is %.4f nautical miles long\n",
                itnry.getID(), itnry.legs.size(), itLength02);
        assertTrue(abs(itLength02 - 2078.028) < 0.001); // known length


        boolean feasible02 = itnry.checkFeasible(vtRec, sMap, tdMap, paMap);
        assert (feasible02);

        return;
    }


}

// =============================================================================
