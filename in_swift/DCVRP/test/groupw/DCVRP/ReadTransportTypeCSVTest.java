/*
 *  ---------------------------------------------------
 *         Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 *
 */
package groupw.DCVRP;

import java.io.File;
import java.util.List;

import static groupw.BaseSim.DSUtils.NAUTICAL_MILE;
import static junit.framework.TestCase.assertTrue;

import org.junit.Test;

/**
 * See ReadUnitCSVTest for guidance.
 * 
 * @author BenWise
 */
public class ReadTransportTypeCSVTest {
    
    public ReadTransportTypeCSVTest() {
    }

    @Test
    public void testReadCSV_A1() {
        String ifName = "transporttype-A1.csv";
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_A1 tests file, "+ifName+", known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test" ;
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadTransportTypeCSV.DataField> result = ReadTransportTypeCSV.readCSV(ifPath, ifName);
        assertTrue(null != result); // known good for this test file
        int numRec = result.size();
        System.out.printf("Found %d records (excluding comments) \n", numRec);
        assertTrue(numRec == 5); // known count for this test file

        for (int i=0; i<numRec; i++){
            ReadTransportTypeCSV.DataField recI = result.get(i);
            double nmRange = recI.oneWayRange; // in nautical miles
            double mRange =  nmRange * NAUTICAL_MILE;
            double kmRadius = (mRange/1000.0)/2.0;
            System.out.printf("Transport type %8s has combat radius %7.2f Km \n", recI.type, kmRadius);
        }
    }
    
}

// =============================================================================
