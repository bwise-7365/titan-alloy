// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import java.io.File;
import java.util.List;

import org.junit.Test;

import static junit.framework.TestCase.assertTrue;

/**
 * See ReadUnitCSVTest for guidance.
 * 
 * @author BenWise
 */
public class ReadTransportVehicleCSVTest {
    
    public ReadTransportVehicleCSVTest() {
    }

    @Test
    public void testReadCSV_A1() {
        String ifName = "transportvehicle-A1.csv";
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_A1 tests file, "+ifName+", known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test" ;
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadTransportVehicleCSV.DataField> result = ReadTransportVehicleCSV.readCSV(ifPath, ifName);
        assertTrue(null != result); // known good for this test file
        int numRec = result.size();
        System.out.printf("Found %d records (excluding comments) \n", numRec);
        assertTrue(numRec == 130); // known count for this INCOMPLETE test file


    }
    
}

// =============================================================================
