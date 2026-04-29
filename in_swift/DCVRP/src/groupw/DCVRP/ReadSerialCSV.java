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
 * Class to read a specific format of CSV file for Serial in DCVRP.
 * This CSV reader is adapted from 'ReadUnitCSV', the canonical example.
 * Read the notes there.
 * 
 * @author BenWise
 */
public class ReadSerialCSV {


    /**
     * ReadSerialCSV.DataField is the static definition of what a line should be;
     *
     */
    static public class DataField {
// notice that we have the prctCap as a double. 120 serials adding to 100% must have some fractions.
        public DataField(String name, String unitName, double area, double weight, double prctCap) {
            this.name = name;
            this.unitName = unitName;
            this.area = area;
            this.weight = weight;
            this.prctCap = prctCap;
        }

        final public static int numFields = 5;
        public String name;
        public String unitName;
        public double area;
        public double weight;  //
        public double prctCap; // percentage of unit's capability
    }

    /**
     * Read a CSV file and return a List of data records as ReadSerialCSV.DataField objects
     *
     * @param ifDir  path to directory holding the file
     * @param ifName exact name of this file
     * @return List of data records
     */
    static public List<ReadSerialCSV.DataField> readCSV(String ifDir, String ifName) {
        List<ReadSerialCSV.DataField> result = new ArrayList<>(0);
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
                    String lineString = "Serial CSV file " + ifPath + " line " + lineNum;
                    String[] fields = str.split(",");
                    if (ReadSerialCSV.DataField.numFields != fields.length) {
                        success = false;
                        String msg = lineString + " expected " + ReadSerialCSV.DataField.numFields + " fields: " + fields.length;
                        throw new IllegalArgumentException(msg);

                    }
                    if (0 < lineNum) { // skip headers, except to verify number of fields
                        for (int i = 0; i < ReadSerialCSV.DataField.numFields; i++) {
                            fields[i] = fields[i].trim(); // remove leading and trailing white space
                        }
                        String name = fields[0];
                        String unitName = fields[1];
                        double area = Double.parseDouble(fields[2]);
                        double weight = Double.parseDouble(fields[3]);
                        double prctCap = Double.parseDouble(fields[4]);

                        if (name.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " Serial name should have at least " + MinimumNameLength + " characters: " + name.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (unitName.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " unit name should have at least " + MinimumNameLength + " characters: " + name.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (prctCap <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive prctCap: " + prctCap;
                            throw new IllegalArgumentException(msg);
                        }
                        if (100.0 <= prctCap) {
                            success = false;
                            String msg = lineString + " should not have prctCap over 100: " + prctCap;
                            throw new IllegalArgumentException(msg);
                        }
                        if (area <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive area: " + area;
                            throw new IllegalArgumentException(msg);
                        }
                        if (weight <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive weight: " + weight;
                            throw new IllegalArgumentException(msg);
                        }

                        ReadSerialCSV.DataField df = new ReadSerialCSV.DataField(name, unitName, area, weight, prctCap);
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
     *
     * @param sRecords list of Serial records
     * @param unitMap map of unit-name to Unit record
     * @return map of serial-name to Serial objects
     */
    static public Map<String, Serial> makeSerialMap(List<ReadSerialCSV.DataField> sRecords,
                                                    Map<String, ReadUnitCSV.DataField> unitMap){
        Map<String, Serial> sMap = new HashMap<>(sRecords.size());
        for (ReadSerialCSV.DataField sRec : sRecords) {
            ReadUnitCSV.DataField uRec = unitMap.get(sRec.unitName);
            Serial s = new Serial(sRec.name, sRec.unitName);
            s.weight = sRec.weight;
            s.area = sRec.area;
            s.deliveryPriority = uRec.deliveryPriority;
            s.deliveryWindow = uRec.deliveryWindow;;
            s.deliveryTime = uRec.deliveryTime;
            s.deliveryNodeName = uRec.deliveryNodeName;
            s.currentNodeName = uRec.startNodeName; // not unreasonable default

            // leave the rest of the values at reasonable initial defaults

            sMap.put(sRec.name, s);
        }
        return sMap;
    }
}

// =============================================================================
