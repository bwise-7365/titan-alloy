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
import java.util.*;

import static groupw.DCVRP.UtilsDCVR.MinimumNameLength;
import static groupw.DCVRP.UtilsDCVR.csvCommentChar;

/**
 * Class to read a specific format of CSV file for type of Transport in DCVRP.
 * This CSV reader is adapted from 'ReadUnitCSV', the canonical example.
 * Read the notes there.
 * 
 * @author BenWise
 */
public class ReadPortAccessCSV {


    /**
     * ReadPortAccessCSV.DataField is the static definition of what a line should be;
     * Like a normalized SQL relation, each line is exactly the same (no lists)
     *
     */
    static public class DataField {

        public DataField(String pName, String tType) {
            this.portName = pName;
            this.transType = tType;
        }

        final public static int numFields = 2;
        public String portName;
        public String transType;
    }

    /**
     * Read a CSV file and return a List of data records as ReadPortAccessCSV.DataField objects
     *
     * @param ifDir  path to directory holding the file
     * @param ifName exact name of this file
     * @return List of data records
     */
    static public List<ReadPortAccessCSV.DataField> readCSV(String ifDir, String ifName) {
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
                    String lineString = "Port Access CSV file " + ifPath + " line " + lineNum;
                    String[] fields = str.split(",");
                    if (ReadPortAccessCSV.DataField.numFields != fields.length) {
                        success = false;
                        String msg = lineString + " expected " + ReadPortAccessCSV.DataField.numFields + " fields: " + fields.length;
                        throw new IllegalArgumentException(msg);

                    }
                    if (0 < lineNum) { // skip headers, except to verify number of fields
                        for (int i = 0; i < ReadPortAccessCSV.DataField.numFields; i++) {
                            fields[i] = fields[i].trim(); // remove leading and trailing white space
                        }
                        String pName = fields[0];
                        String tType = fields[1];

                        if (pName.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " port name should have at least " + MinimumNameLength + " characters: " + pName.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (tType.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " transport type should have at least " + MinimumNameLength + " characters: " + tType.length();
                            throw new IllegalArgumentException(msg);
                        }

                        ReadPortAccessCSV.DataField df = new ReadPortAccessCSV.DataField(pName, tType);
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
     * Build the map from portName to Set of vehicle-types which can access that port
     * @param paRecs
     * @return
     */
    static public Map<String, Set<String>> makePortAccessMap(List<ReadPortAccessCSV.DataField> paRecs) {
        Map<String, Set<String>> paMap = new HashMap<>(1);
        for (ReadPortAccessCSV.DataField pa : paRecs) {
            paMap.put(pa.portName, new HashSet<>(1));
        }
        for (ReadPortAccessCSV.DataField pa : paRecs) {
            Set<String> sSet = paMap.get(pa.portName);
            sSet.add(pa.transType);
            paMap.put(pa.portName, sSet);
        }
        return paMap;
    }
}


// =============================================================================
