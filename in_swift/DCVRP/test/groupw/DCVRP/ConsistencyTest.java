// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import org.junit.Test;

import java.io.File;
import java.util.*;

import static java.lang.Math.abs;
import static junit.framework.TestCase.assertNotSame;
import static junit.framework.TestCase.assertTrue;

/**
 *
 * @author BenWise
 */
public class ConsistencyTest {

    public ConsistencyTest() {
    }

    private void checkUsage(String label, String n, List<String> names, Map<String, Boolean> used) {
        if (!names.contains(n)) {
            System.out.printf("Undefined %s: ->%s<- \n", label, n);
        }
        assertTrue(names.contains(n));
        used.put(n, true);
    }

    private void allUsed(String label, List<String> names, Map<String, Boolean> used) {
        boolean allUsed = true;
        for (String gnn : names) {
            if (!used.get(gnn)) {
                System.out.printf("%s ->%s<- was never used\n", label, gnn);
                allUsed = false;
            }
        }
        assertTrue(allUsed);
    }

    private void noDups(String label, List<String> sSet){
        int n = sSet.size();
        for (int i=0; i<n; i++) {
            String si = sSet.get(i);
            for (int j=i+1; j<n; j++) {
                String sj = sSet.get(j);
                if (si == sj) {
                    System.out.printf("%s have duplicate value: %s \n", label, sj);
                }
                assertNotSame(si, sj);
            }
        }
    }

    /**
     * Make sure that every defined GeoNode is used and no undefined ones are used.
     * Make sure that every defined Transport type is used and no undefined ones are used.
     * This assumes the individual files have passed tests.
     */
    @Test
    public void testGeoNodeTransportConsistency() {
        System.out.printf("Running testGeoNodeTransportConsistency \n");

        String gnLabel = "GeoNode name";
        String ttLabel = "Transport type";

        // of course, these must be a synchronized, consistent set
        String geoNodeFileName = "geograph-nodes-A1.csv";
        String transTypeFileName = "transporttype-A1.csv";
        String unitFileName = "unit-B1.csv";
        String portAccessFileName = "portaccess-A1.csv";
        String transVehicleFileName = "transportvehicle-A1.csv";

        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        // get the Set of GeoNode names. Each should be used at least once in at least one file.
        List<ReadVRNodeCSV.DataField> gNodeRecords = ReadVRNodeCSV.readCSV(ifPath, geoNodeFileName);
        List<String> gNodeNames = new ArrayList<>(gNodeRecords.size());
        noDups(gnLabel, gNodeNames);
        Map<String, Boolean> gNodeUsed = new HashMap<>(gNodeRecords.size());
        for (ReadVRNodeCSV.DataField r : gNodeRecords) {
            gNodeNames.add(r.name);
            gNodeUsed.put(r.name, false);
        }

        // get the Set of transport types. Each should be used at least once in at least one file.
        List<ReadTransportTypeCSV.DataField> tTypeRecords = ReadTransportTypeCSV.readCSV(ifPath, transTypeFileName);
        List<String> tTypes = new ArrayList<>(tTypeRecords.size());
        noDups(ttLabel, tTypes);
        Map<String, Boolean> tTypeUsed = new HashMap<>(tTypeRecords.size());
        for (ReadTransportTypeCSV.DataField r : tTypeRecords) {
            tTypes.add(r.type);
            tTypeUsed.put(r.type, false);
        }

        List<ReadPortAccessCSV.DataField> portAccessRecords = ReadPortAccessCSV.readCSV(ifPath, portAccessFileName);
        List<ReadTransportVehicleCSV.DataField> vehicleRecords = ReadTransportVehicleCSV.readCSV(ifPath, transVehicleFileName);
        List<ReadUnitCSV.DataField> unitRecords = ReadUnitCSV.readCSV(ifPath, unitFileName);


        // every port-access record must have valid location and valid transport
        List<String> pNames = new ArrayList<>();
        for (ReadPortAccessCSV.DataField pRec : portAccessRecords) {
            pNames.add(pRec.portName);
            checkUsage(gnLabel, pRec.portName, gNodeNames, gNodeUsed);
            checkUsage(ttLabel, pRec.transType, tTypes, tTypeUsed);
        }
        noDups("Port name", pNames);
        pNames = null;

        // every vehicle must have valid home base and type
        List<String> tNames = new ArrayList<>();
        for (ReadTransportVehicleCSV.DataField tRec : vehicleRecords) {
            tNames.add(tRec.name);
            checkUsage(gnLabel, tRec.homeBase, gNodeNames, gNodeUsed);
            checkUsage(ttLabel, tRec.type, tTypes, tTypeUsed);
        }
        noDups("Transport", tNames);
        tNames = null;

        // every unit must have valid start and destination
        List<String> uNames = new ArrayList<>();
        for (ReadUnitCSV.DataField uRec : unitRecords) {
            uNames.add(uRec.name);
            checkUsage(gnLabel, uRec.startNodeName, gNodeNames, gNodeUsed);
            checkUsage(gnLabel, uRec.deliveryNodeName, gNodeNames, gNodeUsed);
        }
        noDups("Unit", uNames);

        allUsed(gnLabel, gNodeNames, gNodeUsed);
        allUsed(ttLabel, tTypes, tTypeUsed);

        return;
    }

    /**
     * Make sure that every Unit has at least one Serial.
     * Make sure that every Serial is from a defined Unit.
     * Make sure that the capabilities of a Unit's Serials add up to 100 percent.
     * This assumes the individual files have passed tests.
     */
    @Test
    public void testUnitSerialConsistency() {

        System.out.printf("Running testGeoNodeTransportConsistency \n");

        String uLabel = "Unit name";

        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        // of course, these must be a synchronized, consistent set
        String unitFileName = "unit-B2.csv";
        String serialFileName = "serial-B2.csv";

        List<ReadUnitCSV.DataField> unitRecords = ReadUnitCSV.readCSV(ifPath, unitFileName);
        System.out.printf("Found %d unit records\n", unitRecords.size());
        List<String> uNames = new ArrayList<>(unitRecords.size());
        Map<String, Boolean> uNameUsed = new HashMap<>(unitRecords.size());
        Map<String, Double> uCapacity = new HashMap<>(unitRecords.size());
        for (ReadUnitCSV.DataField r : unitRecords) {
            uNames.add(r.name);
            uNameUsed.put(r.name, false);
            uCapacity.put(r.name, 0.0);
        }
        noDups("Unit", uNames);

        List<ReadSerialCSV.DataField> serialRecords = ReadSerialCSV.readCSV(ifPath, serialFileName);
        System.out.printf("Found %d serial records\n", serialRecords.size());
        List<String> sNames = new ArrayList<>(serialRecords.size());
        for (ReadSerialCSV.DataField s : serialRecords) {
            sNames.add(s.unitName);
            checkUsage(uLabel, s.unitName, uNames, uNameUsed);
            uCapacity.put(s.unitName, s.prctCap + uCapacity.get(s.unitName)); // at this point, s must be a valid key (see previous line)
        }
        noDups("Serial", sNames);

        double prctTol = 0.1;
        for (String u : uNames) {
            double pc = uCapacity.get(u);
            if (prctTol < abs(100.0 - pc)) {
                System.out.printf("Serials of %s do not add up to 100 percent: %d \n", u, pc);
            }
            assertTrue(abs(100.0 - pc) < prctTol);
        }
    }
}

// =============================================================================
