// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;


import java.io.BufferedReader;
import java.io.File;
import java.io.FileReader;
import java.io.IOException;
import java.util.*;

import static groupw.DCVRP.UtilsDCVR.MinimumNameLength;
import static groupw.DCVRP.UtilsDCVR.csvCommentChar;


/**
 * Class to read a specific format of CSV file for GeoGraph Arc in DCVRP.
 * This CSV reader is adapted from 'ReadUnitCSV', the canonical example.
 * Read the notes there.
 *
 * @author BenWise
 */
public class ReadVRArcCSV {

    public ReadVRArcCSV() {
    }
    /**
     * ReadGeoArcCSV.DataField is the static definition of what a line should be.
     */
    static public class DataField {
        public DataField(String an, String snn, String tgtNodeName, String d, double tl) {
            this.arcName = an;
            this.srcNodeName = snn;
            this.tgtNodeName = tgtNodeName;
            this.domain = d;
            this.trueLength = tl;
        }

        final public static int numFields = 5;
        public String arcName;
        public String srcNodeName;
        public String tgtNodeName;
        public String domain;
        public double trueLength; // nautical miles

    }

    /**
     * Read a CSV file and return a List of data records as ReadUnitCSV.DataField objects
     *
     * @param ifDir  path to directory holding the file
     * @param ifName exact name of this file
     * @return List of data records
     */
    static public List<ReadVRArcCSV.DataField> readCSV(String ifDir, String ifName) {
        List<ReadVRArcCSV.DataField> result = new ArrayList<>(0);
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
                    String lineString = "GeoGraph Arc CSV file " + ifPath + " line " + lineNum;
                    String[] fields = str.split(",");
                    if (ReadVRArcCSV.DataField.numFields != fields.length) {
                        success = false;
                        String msg = lineString + " expected " + ReadVRArcCSV.DataField.numFields + " fields: " + fields.length;
                        throw new IllegalArgumentException(msg);

                    }
                    if (0 < lineNum) { // skip headers, except to verify number of fields
                        for (int i = 0; i < ReadVRArcCSV.DataField.numFields; i++) {
                            fields[i] = fields[i].trim(); // remove leading and trailing white space
                        }
                        String arcName = fields[0];
                        String srcNodeName = fields[1];
                        String tgtNodeName = fields[2];
                        String domain = fields[3];
                        double tLength = Double.parseDouble(fields[4]);


                        if (arcName.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " arc name should have at least " + MinimumNameLength + " characters: " + arcName.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (srcNodeName.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " arc src-name should have at least " + MinimumNameLength + " characters: " + srcNodeName.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (tgtNodeName.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " arc tgt-name should have at least " + MinimumNameLength + " characters: " + tgtNodeName.length();
                            throw new IllegalArgumentException(msg);
                        }
                        // we do allow self-edges as some road networks will have them.
                        if (domain.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " arc domain-name should have at least " + MinimumNameLength + " characters: " + domain.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (tLength <= 0.0) {
                            success = false;
                            String msg = lineString + " should have positive true length: " + tLength;
                            throw new IllegalArgumentException(msg);
                        }

                        ReadVRArcCSV.DataField df = new ReadVRArcCSV.DataField(arcName, srcNodeName, tgtNodeName, domain, tLength);
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
