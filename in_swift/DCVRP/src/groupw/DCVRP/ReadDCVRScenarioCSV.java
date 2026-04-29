// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import groupw.Network.NWUtils;

import javax.swing.*;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;


/**
 * The ReadScenarioCSV class performs these basic functions.
 * First, it reads the required CSV files.
 * Second, it builds the required Lists of records (including those which depend on each other),
 * Third, it bundles the records into a single structure that can be passed to later functions.
 */
public class ReadDCVRScenarioCSV {
    public static class ScenarioRecord {
        public List<ReadUnitCSV.DataField> unitRecords;
        public List<ReadTransportTypeCSV.DataField> vtRecords;
        public List<ReadTransportVehicleCSV.DataField> vRecords;
        public List<ReadPortAccessCSV.DataField> paRecords;
        public List<ReadTransportDomainCSV.DataField> tdRecords;
        public List<ReadSerialCSV.DataField> sRecords;
        public VRGraph vrg;

        public ScenarioRecord(List<ReadUnitCSV.DataField> unitRecords,
                               List<ReadTransportTypeCSV.DataField> vtRecords,
                               List<ReadTransportVehicleCSV.DataField> vRecords,
                               List<ReadPortAccessCSV.DataField> paRecords,
                               List<ReadTransportDomainCSV.DataField> tdRecords,
                               List<ReadSerialCSV.DataField> sRecords,
                               VRGraph vrg) {
            this.unitRecords = unitRecords;
            this.vtRecords = vtRecords;
            this.vRecords = vRecords;
            this.paRecords = paRecords;
            this.tdRecords = tdRecords;
            this.sRecords = sRecords;
            this.vrg = vrg;
        }

        public void resetHomeBase(String vehicleName, String nodeName) {
            for (ReadTransportVehicleCSV.DataField vr : vRecords) {
                if (vehicleName.equals(vr.name)) {
                    vr.homeBase = nodeName;
                }
            }
        }

        public void resetPortAccess(String portName, Set<String> vehicleTYpes){
            List<ReadPortAccessCSV.DataField> par2 = new ArrayList<>();
            for (ReadPortAccessCSV.DataField df : paRecords) {
                if (!portName.equals(df.portName)) {
                    par2.add(df);
                }
            }
            for (String vt : vehicleTYpes) {
                ReadPortAccessCSV.DataField df = new ReadPortAccessCSV.DataField(portName, vt);
                par2.add(df);
            }
            paRecords = par2;
        }
    }

    /**
     * Given the file names, read them and build the ScenarioRecord object
     * @param ifPath
     * @param arcFileName
     * @param nodeFileName
     * @param unitFileName
     * @param serialFileName
     * @param transportFileName
     * @param transTypeFileName
     * @param transDomainFileName
     * @param portAccessFileName
     * @return
     */
    public static ScenarioRecord readCSVs(String ifPath,
                                          String arcFileName, String nodeFileName,
                                          String unitFileName, String serialFileName,
                                          String transportFileName, String transTypeFileName, String transDomainFileName,
                                          String portAccessFileName) {
        VRGraph g = VRGraph.readCSV(ifPath, arcFileName, nodeFileName);
        List<ReadUnitCSV.DataField> unitRecords = ReadUnitCSV.readCSV(ifPath, unitFileName);
        List<ReadSerialCSV.DataField> sRecords = ReadSerialCSV.readCSV(ifPath, serialFileName);
        List<ReadTransportVehicleCSV.DataField> vRecords = ReadTransportVehicleCSV.readCSV(ifPath, transportFileName);
        List<ReadTransportTypeCSV.DataField> vtRecords = ReadTransportTypeCSV.readCSV(ifPath, transTypeFileName);
        List<ReadTransportDomainCSV.DataField> tdRecords = ReadTransportDomainCSV.readCSV(ifPath, transDomainFileName);
        List<ReadPortAccessCSV.DataField> paRecords = ReadPortAccessCSV.readCSV(ifPath, portAccessFileName);
        ScenarioRecord rslt = new ScenarioRecord(unitRecords, vtRecords, vRecords, paRecords, tdRecords, sRecords, g);
        return rslt;
    }

    public static ScenarioRecord readConfigCSV (String ifPath,
                                                String ifName) {
        List<ReadConfigurationCSV.DataField> result = ReadConfigurationCSV.readCSV(ifPath, ifName);
        if (null == result) {
            System.out.println("Some error occurred");
            return null;
        }
        System.out.printf("Found %d records \n", result.size());
        assert(ReadConfigurationCSV.DataField.numRecords == result.size()); // I know how many are required
        Map<String, String> cfMap = ReadConfigurationCSV.makeConfigMap(result);
        ScenarioRecord rslt = readCSVs(ifPath,
                cfMap.get(ReadConfigurationCSV.arcFileName),
                cfMap.get(ReadConfigurationCSV.nodeFileName),
                cfMap.get(ReadConfigurationCSV.unitFileName),
                cfMap.get(ReadConfigurationCSV.serialFileName),
                cfMap.get(ReadConfigurationCSV.transportFileName),
                cfMap.get(ReadConfigurationCSV.transTypeFileName),
                cfMap.get(ReadConfigurationCSV.transDomainFileName),
                cfMap.get(ReadConfigurationCSV.portAccessFileName));
        return rslt;
    }


    public static ReadDCVRScenarioCSV.ScenarioRecord initWithFileChooser() {
        System.out.flush();
        System.out.println("\n");
        System.out.println("initWithFileChooser tests reading configuration file");
        JDialog dialog = new JDialog(new JFrame(), "Select CSV", true); // unused title

        // attempt to prevent the generic dialog from hiding behind main window
        dialog.toFront(); // this alone is not enough
        dialog.requestFocus();

        ReadDCVRScenarioCSV.ScenarioRecord sRec = null;
        // Tell them which one to choose for this test?
        String title = "Choose configuration CSV data file";
        NWUtils.Tuple2<String, String> filePair = UtilsDCVR.chooseCsvFile(title, dialog);
        if (null != filePair) {
            String ifPath = filePair.get0();
            String ifName = filePair.get1();
            sRec = ReadDCVRScenarioCSV.readConfigCSV(ifPath, ifName);
            assert(null != sRec);

        }
        else {
            System.out.println("Operation cancelled.");
        }
        return sRec;
    }

    /**
     * This was the first testing scenario
     * @return
     */
    public static ReadDCVRScenarioCSV.ScenarioRecord readStandardTestCase() {

        // The next section reads a consistent set of files,
        // checks that they have the expected number of records,
        // and builds Lists of DataField
        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        // This is a consistent set of files which
        // (as of 25 February 2026) were verified by other tests.
        String arcFileName = "geograph-arcs-A1.csv";
        String nodeFileName = "geograph-nodes-A1.csv";
        String unitFileName = "unit-B2.csv";
        String serialFileName = "serial-B2.csv";
        String transportFileName = "transportvehicle-A1.csv";
        String transTypeFileName = "transporttype-A1.csv";
        String transDomainFileName = "transportdomains-A1.csv";
        String portAccessFileName = "portaccess-A1.csv";

        ReadDCVRScenarioCSV.ScenarioRecord rslt = ReadDCVRScenarioCSV.readCSVs(ifPath, arcFileName, nodeFileName, unitFileName,
                serialFileName, transportFileName, transTypeFileName, transDomainFileName,
                portAccessFileName);
        return rslt;
    }

    /**
     * Network from the 'standard scenario', everything else version B2
     * @return
     */
    public static ReadDCVRScenarioCSV.ScenarioRecord readScenarioB2() {

        // The next section reads a consistent set of files,
        // checks that they have the expected number of records,
        // and builds Lists of DataField
        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        // This is a consistent set of files which
        // (as of 25 February 2026) were verified by other tests.
        String arcFileName = "geograph-arcs-A1.csv";
        String nodeFileName = "geograph-nodes-A1.csv";
        String unitFileName = "unit-B2.csv";
        String serialFileName = "serial-B2.csv";
        String transportFileName = "transportvehicle-B2.csv";
        String transTypeFileName = "transporttype-B2.csv";
        String transDomainFileName = "transportdomains-B2.csv";
        String portAccessFileName = "portaccess-B2.csv";

        ReadDCVRScenarioCSV.ScenarioRecord rslt = ReadDCVRScenarioCSV.readCSVs(ifPath, arcFileName, nodeFileName, unitFileName,
                                                                               serialFileName, transportFileName, transTypeFileName, transDomainFileName,
                                                                               portAccessFileName);
        return rslt;
    }


    /**
     * Network from the 'standard scenario', everything else version C1.
     * No LST, more LSM-100, more KC-130 in Cuba.
     * @return
     */
    public static ReadDCVRScenarioCSV.ScenarioRecord readScenarioC1() {

        // The next section reads a consistent set of files,
        // checks that they have the expected number of records,
        // and builds Lists of DataField
        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        // This is a consistent set of files which
        // (as of 25 February 2026) were verified by other tests.
        String arcFileName = "geograph-arcs-A1.csv";
        String nodeFileName = "geograph-nodes-A1.csv";
        String unitFileName = "unit-C1.csv";
        String serialFileName = "serial-C1.csv";
        String transportFileName = "transportvehicle-C1.csv";
        String transTypeFileName = "transporttype-C1.csv";
        String transDomainFileName = "transportdomains-C1.csv";
        String portAccessFileName = "portaccess-C1.csv";

        ReadDCVRScenarioCSV.ScenarioRecord rslt = ReadDCVRScenarioCSV.readCSVs(ifPath, arcFileName, nodeFileName, unitFileName,
                                                                               serialFileName, transportFileName, transTypeFileName, transDomainFileName,
                                                                               portAccessFileName);
        return rslt;
    }


    public static ReadDCVRScenarioCSV.ScenarioRecord readHighCapacityTestCase() {

        // The next section reads a consistent set of files,
        // checks that they have the expected number of records,
        // and builds Lists of DataField
        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        // This is a consistent set of files which
        // (as of 25 February 2026) were verified by other tests.
        String arcFileName = "geograph-arcs-A1.csv";
        String nodeFileName = "geograph-nodes-A1.csv";
        String unitFileName = "unit-B2.csv";
        String serialFileName = "serial-B2.csv";
        String transportFileName = "transportvehicle-A1.csv";
        String transTypeFileName = "transporttype-A2.csv"; // This includes very large KC-130
        String transDomainFileName = "transportdomains-A1.csv";
        String portAccessFileName = "portaccess-A1.csv";

        ReadDCVRScenarioCSV.ScenarioRecord rslt = ReadDCVRScenarioCSV.readCSVs(ifPath, arcFileName, nodeFileName, unitFileName,
                serialFileName, transportFileName, transTypeFileName, transDomainFileName,
                portAccessFileName);
        return rslt;
    }



}


// =============================================================================
