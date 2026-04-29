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
 * Class to read a specific format of CSV file for type of Transport in DCVRP.
 * This CSV reader is adapted from 'ReadUnitCSV', the canonical example.
 * Read the notes there.
 * 
 * @author BenWise
 */
public class ReadTransportDomainCSV {


    /**
     * ReadTransportDomainCSV.DataField is the static definition of what a line should be;
     *
     */
    static public class DataField {

        public DataField(String type, String domain) {
            this.type = type;
            this.domain = domain;
        }

        final public static int numFields = 2;
        public String type;
        public String domain;
    }

    /**
     * Read a CSV file and return a List of data records as ReadTransportDomainCSV.DataField objects
     *
     * @param ifDir  path to directory holding the file
     * @param ifName exact name of this file
     * @return List of data records
     */
    static public List<ReadTransportDomainCSV.DataField> readCSV(String ifDir, String ifName) {
        List<ReadTransportDomainCSV.DataField> result = new ArrayList<>(0);
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
                    if (ReadTransportDomainCSV.DataField.numFields != fields.length) {
                        success = false;
                        String msg = lineString + " expected " + ReadTransportDomainCSV.DataField.numFields + " fields: " + fields.length;
                        throw new IllegalArgumentException(msg);

                    }
                    if (0 < lineNum) { // skip headers, except to verify number of fields
                        for (int i = 0; i < ReadTransportDomainCSV.DataField.numFields; i++) {
                            fields[i] = fields[i].trim(); // remove leading and trailing white space
                        }
                        String type = fields[0];
                        String domain = fields[1];

                        if (type.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " transport type should have at least " + MinimumNameLength + " characters: " + type.length();
                            throw new IllegalArgumentException(msg);
                        }
                        if (domain.length() < MinimumNameLength) {
                            success = false;
                            String msg = lineString + " transport domain should have at least " + MinimumNameLength + " characters: " + domain.length();
                            throw new IllegalArgumentException(msg);
                        }

                        ReadTransportDomainCSV.DataField df = new ReadTransportDomainCSV.DataField(type, domain);
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
     * Build the map from transport type to the set of domains it can cross
     * @param tdRecs
     * @return
     */
    static public Map<String, Set<String>> makeTransportDomainMap(List<ReadTransportDomainCSV.DataField> tdRecs) {
        Map<String, Set<String>> tdMap = new HashMap<>(1);
        for (DataField tdRec : tdRecs) {
            tdMap.put(tdRec.type, new HashSet<>(1));
        }
        for (DataField tdRec : tdRecs) {
            Set<String> sSet = tdMap.get(tdRec.type);
            sSet.add(tdRec.domain);
            tdMap.put(tdRec.type, sSet);
        }
        return tdMap;
    }
}

// =============================================================================
