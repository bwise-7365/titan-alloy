// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import org.junit.Test;

import java.io.File;
import java.util.List;

import static junit.framework.TestCase.assertTrue;

/**
 *
 * @author BenWise
 */
public class ReadTransportDomainCSVTest {
    
    public ReadTransportDomainCSVTest() {
    }


    @Test
    public void testReadCSV_A1() {
        String ifName = "transportdomains-A1.csv";
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_A1 tests file, " + ifName + ", known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadTransportDomainCSV.DataField> result = ReadTransportDomainCSV.readCSV(ifPath, ifName);
        assertTrue(null != result); // known good for this test file
        int numRec = result.size();
        System.out.printf("Found %d records (excluding comments) \n", numRec);
        assertTrue(numRec == 9); // known count for this test file
        return;
    }
}
// =============================================================================
