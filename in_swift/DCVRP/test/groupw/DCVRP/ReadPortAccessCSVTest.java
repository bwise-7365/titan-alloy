/*
 *  ---------------------------------------------------
 *         Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 *
 */
package groupw.DCVRP;

import java.io.File;
import java.util.List;
import static junit.framework.TestCase.assertTrue;

import org.junit.Test;

/**
 * See ReadUnitCSVTest for guidance.
 * 
 * @author BenWise
 */
public class ReadPortAccessCSVTest {
    
    public ReadPortAccessCSVTest() {
    }

    @Test
    public void testReadCSV_A1() {
        String ifName = "portaccess-A1.csv";
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_A1 tests file, "+ifName+", known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test" ;
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadPortAccessCSV.DataField> result = ReadPortAccessCSV.readCSV(ifPath, ifName);
        assertTrue(null != result); // known good for this test file
        int numRec = result.size();
        System.out.printf("Found %d records (excluding comments) \n", numRec);
        assertTrue(numRec == 100); // known count for this test file

    }
    
}

// =============================================================================
