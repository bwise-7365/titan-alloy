/*
 *  ---------------------------------------------------
 *         Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 * 
 */
package groupw.DCVRP;

import java.io.File;
import java.util.ArrayList;
import java.util.List;
import javax.swing.JDialog;
import javax.swing.JFrame;
import org.junit.Test;
import static org.junit.Assert.*;

import groupw.Logistics.Parsers.DuplicateItemException;
import groupw.Network.NWUtils.Tuple2;

/**
 * This is the canonical style of testing.
 * (*) Detailed test of known-correct examples, including specific numbers of each component.
 * (*) Test of known-incorrect examples, including throwing the appropriate error.
 * (*) Ideally, test on semi-randomly generated example data structures.
 * (*) Use the same processing code in every test (e.g. ReadUnitCSV.readCSV)
 * (*) No global variables
 * (*) No special order among top-level functions.
 * (*) If two functions do have to happen in a particular order
 * (e.g. read nodes, then read arcs between nodes), then have a top-level
 * function that calls them in the right order.
 * (*) The design philosophy is to have function-definition determine the order of invocation.
 * Do not rely on the first programmer's memory or the fifth programmer's guesses.
 * 
 * @author BenWise
 */
public class ReadUnitCSVTest {

    public ReadUnitCSVTest() {
    }

    @Test
    public void testReadCSV_bad_files() {
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_bad_files demonstrates various errors");
        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<String> ifNames = new ArrayList<>(0);
        ifNames.add("unit-A0.csv");
        ifNames.add("unit-A2.csv");
        ifNames.add("unit-A3.csv");
        ifNames.add("unit-A4.csv");
        ifNames.add("unit-A5.csv");

        for (String ifn : ifNames) {
            boolean failed = false;
            try {
                System.out.printf("Trying file %s ... ", ifn);
                List<ReadUnitCSV.DataField> result = ReadUnitCSV.readCSV(ifPath, ifn);
                assertTrue(null == result); // known bad test file A0
                failed = true;
            } catch (Exception e) {
                System.out.printf("Caught exception: %s \n", e.getMessage());
                failed = true;
            }
            assert (failed);
        }
        System.out.flush();
    }

    @Test
    public void testReadCSV_A1() {
        String ifName = "unit-A1.csv";
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_A1 tests file, "+ifName+", known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test" + File.separator;
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadUnitCSV.DataField> result = ReadUnitCSV.readCSV(ifPath, ifName);
        assertTrue(null != result); // known good for this test file
        System.out.printf("Found %d records (excluding comments) \n", result.size());
        assertTrue(result.size() == 3); // known count for this A1 file
    }
    @Test
    public void testReadCSV_B1() {
        String ifName = "unit-B1.csv";
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_B1 tests file, "+ifName+", known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test" + File.separator;
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadUnitCSV.DataField> result = ReadUnitCSV.readCSV(ifPath, ifName);
        assertTrue(null != result); // known good for this test file
        System.out.printf("Found %d records (excluding comments) \n", result.size());
        assertTrue(result.size() == 17); // known count for this B1 file
    }


    @Test
    public void initWithFileChooser() {
        System.out.flush();
        System.out.println("\n");
        System.out.println("initWithFileChooser tests reading file which might or might not be good");
        JDialog dialog = new JDialog(new JFrame(), "Select CSV", true); // unused title

        // attempt to prevent the generic dialog from hiding behind main window
        dialog.toFront(); // this alone is not enough
        dialog.requestFocus();

        // Tell them which one to choose for this test?
        String title = "Choose AI, B1 or B2 Unit CSV data file";
        Tuple2<String, String> filePair = UtilsDCVR.chooseCsvFile(title, dialog);
        if (null != filePair) {
            String ifPath = filePair.get0();
            String ifName = filePair.get1();
            List<ReadUnitCSV.DataField> result = ReadUnitCSV.readCSV(ifPath, ifName);
            if (null == result) {
                System.out.println("Some error occurred");
            } else {
                System.out.printf("Found %d records \n", result.size());
                assertTrue(0 < result.size());
            }
        }
        else {
            System.out.println("Operation cancelled.");
        }
    }

}
// =============================================================================
