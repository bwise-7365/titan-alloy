// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import junit.framework.TestCase;
import org.junit.Test;

import java.io.File;
import java.util.List;


/**
 * See ReadUnitCSVTest for guidance.
 * 
 * @author BenWise
 */
public class ReadSerialCSVTest extends TestCase {
    @Test
    public void testReadCSV_A1() {
        String ifName = "serial-A1.csv";
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_A1 tests file, "+ifName+", known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test" ;
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadSerialCSV.DataField> result = ReadSerialCSV.readCSV(ifPath, ifName);
        assertTrue(null != result); // known good for this test file
        System.out.printf("Found %d records (excluding comments) \n", result.size());
        assertTrue(result.size() == 9); // known count for this test file
    }

}
// =============================================================================
