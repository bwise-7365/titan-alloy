// Copyright Group W, SPA. All Rights Reserved.


package groupw.DCVRP;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static groupw.DCVRP.UtilsDCVR.MinimumNameLength;
import static groupw.DCVRP.UtilsDCVR.csvCommentChar;

/**
 * Class to read a specific format of CSV file for specific Transport vehicles in DCVRP.
 * This CSV reader is adapted from 'ReadUnitCSV', the canonical example.
 * Read the notes there.
 * 
 * @author BenWise
 */
public class ReadTransportVehicleCSV {


    /**
     * ReadTransportVehicleCSV.DataField is the static definition of what a line should be;
     *
     */
    static public class DataField {

        public DataField(String name, String type, String homeBase) {
            this.name = name;
            this.type = type;
            this.homeBase = homeBase;
        }

        final public static int numFields = 3;
        public String name;
        public String type;
        public String homeBase;
    }

    /**
     * Read a CSV file and return a List of data records as ReadTransportVehicleCSV.DataField objects
     *
     * @param ifDir  path to directory holding the file
     * @param ifName exact name of this file
     * @return List of data records
     */
    static public List<ReadTransportVehicleCSV.DataField> readCSV(String ifDir, String ifName) {
        List<DataField> result = new ArrayList<>(0);
        boolean success = true;
        int lineNum = 0;
        int commentNum = 0;
        String ifPath = ifDir + File.separator + ifName;
        try (BufferedReader in = new BufferedReader(new FileReader(ifPath))) {
            String str;
            while ((str = in.readLine()) != null) {
                if ((str.isEmpty()) || (csvCommentChar == str.charAt(0))) {
                    commentNum++;
                    //System.out.printf("Skipping comment %d on line %d\n", commentNum, lineNum);
                } else {
                    String lineString = "Transport Vehicle CSV file " + ifPath + " line " + lineNum;
                    String[] fields = str.split(",");
                    if (ReadTransportVehicleCSV.DataField.numFields != fields.length) {
                        success = false;
                        String msg = lineString + " expected " + ReadTransportVehicleCSV.DataField.numFields + " fields: " + fields.length;
                        throw new IllegalArgumentException(msg);

                    }
                    if (0 < lineNum) { // skip headers, except to verify number of fields
                        for (int i = 0; i < ReadTransportVehicleCSV.DataField.numFields; i++) {
                            fields[i] = fields[i].trim(); // remove leading and trailing white space
                        }
                        String name = fields[0];
                        String type = fields[1];
                        String homeBase = fields[2];

                        if (name.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " transport name should have at least " + MinimumNameLength + " characters: " + name.length();
                            throw new IllegalArgumentException(msg);
                        }

                        if (type.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " transport type should have at least " + MinimumNameLength + " characters: " + type.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (homeBase.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " transport homebase should have at least " + MinimumNameLength + " characters: " + homeBase.length();
                            throw new IllegalArgumentException(msg);
                        }

                        ReadTransportVehicleCSV.DataField df = new ReadTransportVehicleCSV.DataField(name, type, homeBase);
                        result.add(df);
                    }
                }
                lineNum++;
            }
        } catch (IOException e) {
            success = false;
            System.out.println("File Read Error: " + ifPath);
        }
        if (!success) { // if not success, it should have thrown an exception
            result = null; // drop partial results
        }
        return result;
    }

    /**
     * Build the Map from vehicle names to their data records
     *
     * @param vRecords
     * @return
     */
    static public Map<String, ReadTransportVehicleCSV.DataField>
    makeVehicleDataMap(List<ReadTransportVehicleCSV.DataField> vRecords) {
        Map<String, ReadTransportVehicleCSV.DataField> vrMap = new HashMap<>(vRecords.size());
        for (DataField vRec : vRecords) {
            vrMap.put(vRec.name, vRec);
        }
        return vrMap;
    }

    static public Map<String, Transport> makeVehicleMap(Map<String, ReadTransportVehicleCSV.DataField> vRecMap,
                                                        Map<String, ReadTransportTypeCSV.DataField> vTypeMap,
                                                        VRController vrc) {
        Map<String, Transport> vMap = new HashMap<>();
        for (Map.Entry<String, ReadTransportVehicleCSV.DataField> e : vRecMap.entrySet()) {
            String vName = e.getKey();
            String vType = e.getValue().type;
            Transport t = new Transport(vName, vType);
            ReadTransportTypeCSV.DataField typeData = vTypeMap.get(vType);
            t.cargoArea = typeData.cargoArea;
            t.cargoWeight = typeData.cargoWeight;
            vMap.put(vName, t);
        }
        return vMap;
    }

}

// =============================================================================
