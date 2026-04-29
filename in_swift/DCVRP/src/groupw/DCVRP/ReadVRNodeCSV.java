// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import static groupw.DCVRP.UtilsDCVR.MinimumNameLength;
import static groupw.DCVRP.UtilsDCVR.csvCommentChar;

import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;

//import groupw.Logistics.LogDistNW;

/**
 * Class to read a specific format of CSV file for GeoGraph Node in DCVRP.
 * This CSV reader is adapted from 'ReadUnitCSV', the canonical example.
 * Read the notes there.
 * 
 * @author BenWise
 */
public class ReadVRNodeCSV {

    /**
     * ReadUnitCSV.DataField is the static definition of what a line should be.
     */
    static public class DataField {

        public DataField(String name, double latitude, double longitude) {
            this.name = name;
            this.latitude = latitude;
            this.longitude = longitude;
        }


        final public static int numFields = 3;
        public String name;
        public double latitude;  // degrees
        public double longitude; // degrees
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
                    String lineString = "GeoGraph Node CSV file " + ifPath + " line " + lineNum;
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
                        double latitude = Double.parseDouble(fields[1]);
                        double longitude = Double.parseDouble(fields[2]);


                        if (name.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " unit name should have at least " + MinimumNameLength + " characters: " + name.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (latitude < -90.0) {
                            success = false;
                            String msg = lineString + " should have latitude no less than -90: " + latitude;
                            throw new IllegalArgumentException(msg);
                        }
                        if (+90.0 < latitude) {
                            success = false;
                            String msg = lineString + " should have latitude no more than +90: " + latitude;
                            throw new IllegalArgumentException(msg);
                        }
                        if (longitude < -180.0) {
                            success = false;
                            String msg = lineString + " should have longitude no less than -180: " + longitude;
                            throw new IllegalArgumentException(msg);
                        }
                        if (+180.0 < longitude) {
                            success = false;
                            String msg = lineString + " should have longitude no more than +180: " + longitude;
                            throw new IllegalArgumentException(msg);
                        }

                        DataField df = new DataField(name, latitude, longitude);
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
}


// =============================================================================
