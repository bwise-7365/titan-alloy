// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import static groupw.DCVRP.UtilsDCVR.MinimumNameLength;
import static groupw.DCVRP.UtilsDCVR.csvCommentChar;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

//import groupw.Logistics.LogDistNW;

/**
 * Class to read the master configuration CSV.
 * This CSV reader is adapted from 'ReadUnitCSV', the canonical example.
 * Read the notes there.
 * 
 * @author JoshuaSteakelum
 */
public class ReadConfigurationCSV {

    /**
     * ReadConfigurationCSV.DataField is the static definition of what a line should be.
     */
    static public class DataField {

        public DataField(String fName, String fPath) {
            this.fName = fName;
            this.fPath = fPath;
        }

        final public static int numFields = 2; // how many columns are required
        final public static int numRecords = 8; // how many lines are required
        public String fName;
        public String fPath;
    }
    
    final public static String arcFileName = "arcFileName";
    final public static String nodeFileName = "nodeFileName";
    final public static String unitFileName = "unitFileName";
    final public static String serialFileName = "serialFileName";
    final public static String transportFileName = "transportFileName";
    final public static String transTypeFileName = "transTypeFileName";
    final public static String transDomainFileName = "transDomainFileName";
    final public static String portAccessFileName = "portAccessFileName";
    
    final public static List<String> FileTypes = Arrays.asList(
        arcFileName,
        nodeFileName,
        unitFileName,
        serialFileName,
        transportFileName,
        transTypeFileName,
        transDomainFileName,
        portAccessFileName);

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
                    String lineString = "Config CSV file " + ifPath + " line " + lineNum;
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
                        String fileName = fields[0];
                        String filePath = fields[1];


                        if (fileName.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " name should have at least " + MinimumNameLength + " characters: " + fileName.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (filePath.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " path should have at least " + MinimumNameLength + " characters: " + filePath.length();
                            throw new IllegalArgumentException(msg);
                        }
                        
                        DataField df = new DataField(fileName, filePath);
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
     * Ensure that at the least, the 8 required fields exist by name
     * Verifies list of DataFields, returning names of fields still required
     * Returns null if all fields present
     * @param uRecords
     * @return 
     */
    static public List<String> getMissingFields(List<DataField> uRecords) {
        List<String> missing = null;
        for (String field : FileTypes) {
            boolean exists = false;
            for (DataField r : uRecords) {
                if (field.equals(r.fName)) {
                    exists = true;
                }
            }
            if (!exists) {
                if (missing == null) {
                    missing = new ArrayList<>();
                }
                missing.add(field);
            }
        }
        return missing;
    }

    /**
     * Build the Map from config file to their path
     * @param uRecords
     * @return
     */
    static public Map<String, String> makeConfigMap(List<DataField> uRecords) {
        Map<String, String> cfMap = new HashMap<>(uRecords.size());
        for (DataField uRec : uRecords) {
            cfMap.put(uRec.fName, uRec.fPath);
        }
        return cfMap;
    }
}

// =============================================================================
