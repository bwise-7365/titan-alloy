/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */


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
 * Class to read a specific format of CSV file for type of Transport in DCVRP.
 * This CSV reader is adapted from 'ReadUnitCSV', the canonical example.
 * Read the notes there.
 * 
 * @author BenWise
 */
public class ReadTransportTypeCSV {


    /**
     * ReadTransportTypeCSV.DataField is the static definition of what a line should be;
     *
     */
    static public class DataField {

        final public static int numFields = 7;
        public String type;
        public double oneWayRange; // at reasonable load, not "max possible to lift off and fly 100 feet"
        public double cargoArea;
        public double cargoWeight;
        public double transferTime;
        public double cruiseSpd;
        public double cruiseAlt;

        public DataField(String type, double oneWayRange, double cargoArea, double cargoWeight, double transferTime, double cruiseSpd, double cruiseAlt) {
            this.type = type;
            this.oneWayRange = oneWayRange;
            this.cargoArea = cargoArea;
            this.cargoWeight = cargoWeight;
            this.transferTime = transferTime;
            this.cruiseSpd = cruiseSpd;
            this.cruiseAlt = cruiseAlt;
        }
    }

    /**
     * Read a CSV file and return a List of data records as ReadTransportTypeCSV.DataField objects
     *
     * @param ifDir  path to directory holding the file
     * @param ifName exact name of this file
     * @return List of data records
     */
    static public List<ReadTransportTypeCSV.DataField> readCSV(String ifDir, String ifName) {
        List<DataField> result = new ArrayList<>(0);
        boolean success = true;
        int lineNum = 0;
        int commentNum = 0;
        String ifPath = ifDir + File.separator + ifName;
        try (BufferedReader in = new BufferedReader(new FileReader(ifPath))) {
            String str;
            while ((str = in.readLine()) != null) {
                if (csvCommentChar == str.charAt(0)) {
                    commentNum++;
                    //System.out.printf("Skipping comment %d on line %d\n", commentNum, lineNum);
                } else {
                    String lineString = "Transport Type CSV file " + ifPath + " line " + lineNum;
                    String[] fields = str.split(",");
                    if (ReadTransportTypeCSV.DataField.numFields != fields.length) {
                        success = false;
                        String msg = lineString + " expected " + ReadTransportTypeCSV.DataField.numFields + " fields: " + fields.length;
                        throw new IllegalArgumentException(msg);

                    }
                    if (0 < lineNum) { // skip headers, except to verify number of fields
                        for (int i = 0; i < ReadTransportTypeCSV.DataField.numFields; i++) {
                            fields[i] = fields[i].trim(); // remove leading and trailing white space
                        }
                        String type = fields[0];
                        double oneWayRange = Double.parseDouble(fields[1]);
                        double cargoArea = Double.parseDouble(fields[2]);
                        double cargoWeight = Double.parseDouble(fields[3]);
                        double transferTime = Double.parseDouble(fields[4]);
                        double cruiseSpd = Double.parseDouble(fields[5]);
                        double cruiseAlt = Double.parseDouble(fields[6]);

                        if (type.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " transport type should have at least " + MinimumNameLength + " characters: " + type.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (oneWayRange <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive oneWayRange: " + oneWayRange;
                            throw new IllegalArgumentException(msg);
                        }
                        if (cargoArea <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive cargoArea: " + cargoArea;
                            throw new IllegalArgumentException(msg);
                        }
                        if (cargoWeight <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive cargoWeight: " + cargoWeight;
                            throw new IllegalArgumentException(msg);
                        }
                        if (transferTime < 0.0) {
                            success = false;
                            String msg = lineString + " should have zero or positive transferTime: " + transferTime;
                            throw new IllegalArgumentException(msg);
                        }
                        if (cruiseSpd <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive cruiseSpd: " + cruiseSpd;
                            throw new IllegalArgumentException(msg);
                        }
                        if (cruiseAlt < 0.0) { // TODO: Allow submarines?
                            success = false;
                            String msg = lineString + " should have zero or positive cruiseAlt: " + cruiseAlt;
                            throw new IllegalArgumentException(msg);
                        }

                        ReadTransportTypeCSV.DataField df = new ReadTransportTypeCSV.DataField(type, oneWayRange, cargoArea, cargoWeight, transferTime, cruiseSpd, cruiseAlt);
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

    public static Map<String, DataField > makeVTypeMap(List<DataField> tTypeRecords) {
        Map<String, DataField > vtMap = new HashMap<>();
        for (DataField vtr : tTypeRecords){
            vtMap.put(vtr.type, vtr);
        }
        return vtMap;
    }
}

// =============================================================================
