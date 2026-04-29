// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import static groupw.DCVRP.UtilsDCVR.MinimumNameLength;
import static groupw.DCVRP.UtilsDCVR.csvCommentChar;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

//import groupw.Logistics.LogDistNW;

/**
 * Class to read a specific format of CSV file for Unit in DCVRP.
 * This CSV reader is the canonical example of how to do it.
 * 
 * Creating an interface or base class for scanning the file was more complicated
 * than duplicating a few lines of basic scanning code.
 * 
 * (*) The first line of CSV, number 0, is just headers
 * (*) Names (unit, Serial, domain, transport) have a minimum length, e.g. two characters
 * (*) Except for the headers on the first line, order of lines does not matter.
 * (*) It is self-contained and does not rely on any global variables, particular order of evaluating functions, etc.
 * (*) Every line of the CSV has the same structure.
 * (*) No blank lines allowed.
 * (*) Leading and trailing whitespace will be trimmed on each line.
 * (*) It returns a List of objects where each represents a correctly parsed line
 * (*) It throws exceptions when it gets illegal data. Only local (within this line) checks are performed here.
 * (*) We use specific types (even generic 'T') everywhere and never 'Object'.
 * The design guidance is to rely on the compiler to check types.
 * Do not rely on the first programmer's memory or the fifth programmer's guesses.
 * "Strong typing is for weak minds" does not apply here.
 * 
 * @author BenWise
 */
public class ReadUnitCSV {

    /**
     * ReadUnitCSV.DataField is the static definition of what a line should be.
     */
    static public class DataField {

        public DataField(String name, String startNodeName, double dp, double dt, double dw, String dnn) {
            this.name = name;
            this.startNodeName = startNodeName;
            this.deliveryPriority = dp;
            this.deliveryTime = dt;
            this.deliveryWindow = dw;
            this.deliveryNodeName = dnn;
        }

        final public static int numFields = 6;
        public String name;
        public String startNodeName;
        public double deliveryPriority;
        public double deliveryTime;  // seconds since scenario start, presumably converted from DD:HH
        public double deliveryWindow; // seconds, presumably as hours * 3600
        public String deliveryNodeName;
    }

    /**
     * Read a CSV file and return a List of data records as ReadUnitCSV.DataField objects
     *
     * @param ifDir  path to directory holding the file
     * @param ifName exact name of this file
     * @return List of data records
     */
    static public List<DataField> readCSV(String ifDir, String ifName) {
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
                    String lineString = "Unit CSV file " + ifPath + " line " + lineNum;
                    String[] fields = str.split(",");
                    if (DataField.numFields != fields.length) {
                        success = false;
                        String msg = lineString + " expected " + DataField.numFields + " fields: " + fields.length;
                        throw new IllegalArgumentException(msg);

                    }
                    if (0 < lineNum) { // skip headers, except to verify number of fields
                        for (int i = 0; i < DataField.numFields; i++) {
                            fields[i] = fields[i].trim(); // remove leading and trailing white space
                        }
                        String name = fields[0];
                        String startNodeName = fields[1];
                        double priority = Double.parseDouble(fields[2]);
                        double deliveryTime = Double.parseDouble(fields[3]);
                        double deliveryWindow = Double.parseDouble(fields[4]);
                        String deliveryNodeName = fields[5];


                        if (name.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " unit name should have at least " + MinimumNameLength + " characters: " + name.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (startNodeName.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " start node name should have at least " + MinimumNameLength + " characters: " + startNodeName.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (deliveryNodeName.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " delivery node name should have at least " + MinimumNameLength + " characters: " + deliveryNodeName.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (priority <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive priority: " + priority;
                            throw new IllegalArgumentException(msg);
                        }
                        if (deliveryTime <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive deliveryTime: " + deliveryTime;
                            throw new IllegalArgumentException(msg);
                        }
                        if (deliveryWindow <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive deliveryWindow: " + deliveryWindow;
                            throw new IllegalArgumentException(msg);
                        }

                        DataField df = new DataField(name, startNodeName, priority, deliveryTime, deliveryWindow, deliveryNodeName);
                        result.add(df);
                    }
                }
                lineNum++;
            }

        } catch (IOException e) {
            success = false;
            System.out.println("Caught exception, file read error: " + ifPath);
        }
        if (!success) { // if not success, it should have thrown an exception
            result = null; // drop partial results
        }
        return result;
    }

    /**
     * Build the Map from Unit names to their data records
     * @param uRecords
     * @return
     */
    static public Map<String, DataField> makeUnitMap(List<DataField> uRecords) {
        Map<String, DataField> uMap = new HashMap<>(uRecords.size());
        for (DataField uRec : uRecords) {
            uMap.put(uRec.name, uRec);
        }
        return uMap;
    }
}

// =============================================================================
