/*
 * ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Network.NWUtils.Tuple;
import java.io.FileReader;
import java.io.IOException;
import java.io.Reader;
import java.util.Collections;
import java.util.List;
import java.util.ArrayList;
import java.util.Map;
import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVRecord;

/**
 * Reads and parses a file of supply types
 *
 * @author JoshuaSteakelum
 */
public class LoadSupplyDefs {

    public final List<String> SupplyList;//May not want to keep this. Staying for compatability.
    public final Map<String, Tuple> supplies;
     
    
    public LoadSupplyDefs (Map<String, Tuple> suppliesMap){
        this.supplies = suppliesMap;
        this.SupplyList = new ArrayList<>(suppliesMap.keySet());
    }
    /*
    static private List<String> defaultSupplyList() {
        List<String> supvec = new ArrayList<>(5);
        supvec.add("Class I");
        supvec.add("Class II");
        supvec.add("Class III");
        supvec.add("Class IV");
        supvec.add("Class V");
        supvec.add("Class VI");
        supvec.add("Class VII");
        return supvec;
    }
     */
    public LoadSupplyDefs(String filePath) throws IOException {
        supplies = null;
        final String dir = System.getProperty("user.dir");
        String fullPath = dir + "\\" + filePath;

        System.out.println("current dir = " + dir);
        System.out.println("Loading supply list: " + fullPath);

        List<String> supvec = new ArrayList<>(5);

        // set up reading the csv containing supply names
        try {
            System.out.println(fullPath);
            Reader read = new FileReader(fullPath);
            CSVFormat Fcsv = CSVFormat.DEFAULT.builder().build();
            Iterable<CSVRecord> records = Fcsv.parse(read);

            // populate them into a list
            for (CSVRecord rec : records) {
                for (int i = 0; i < rec.size(); i++) {
                    supvec.add(rec.get(i));
                }
            }

        } catch (IOException ex) {
            System.out.println("Could not locate definition of supply types: " + filePath);
            throw (ex);
        }
        for (String f : supvec) {
            // we make leading and trailing spaces visible
            System.out.printf("Will add supply type: '%s'\n", f);
        }
        checkSupplyList(supvec, filePath);
        this.SupplyList = Collections.unmodifiableList(supvec);
        System.out.println("Loaded " + SupplyList.size() + " supply types");
    }

    private void checkSupplyList(List<String> supvec, String filePath) {
        int n = supvec.size();
        if (0 == n) {
            throw new RuntimeException("Empty supply type list in: " + filePath);
        }
        for (int i = 0; i < n; i++) {
            for (int j = i + 1; j < n; j++) {
                if (supvec.get(i).equals(supvec.get(j))) {
                    throw new RuntimeException("Duplicate supply type, " + supvec.get(i) + " in: " + filePath);
                }
            }
        }

    }

}
// =============================================================================
