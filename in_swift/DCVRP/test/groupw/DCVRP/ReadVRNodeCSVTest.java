// Copyright Group W, SPA. All Rights Reserved.
package groupw.DCVRP;

import java.io.File;
import java.util.List;

import static groupw.BaseSim.DSUtils.greatCircleDistance;
import static java.lang.Math.abs;
import static junit.framework.TestCase.assertTrue;
import org.junit.Test;

/**
 *
 * @author BenWise
 */
public class ReadVRNodeCSVTest {

    public ReadVRNodeCSVTest() {
    }


    @Test
    public void testReadCSV_A1() {
        String ifName = "geograph-nodes-A1.csv";
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_A1 tests file, " + ifName + ", known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadVRNodeCSV.DataField> result = ReadVRNodeCSV.readCSV(ifPath, ifName);
        assertTrue(null != result); // known good for this test file
        int numRec = result.size();
        System.out.printf("Found %d records (excluding comments) \n", numRec);
        assertTrue(numRec == 18); // known count for this test file

        int nodeCounter = 1;
        double degreeTol = 0.01; // degrees, only check distance if not co-located
        double meterTol = 0.1; // meters round-off tolerance
        for (int i = 0; i < numRec; i++) {
            ReadVRNodeCSV.DataField recI = result.get(i);
            double latI = recI.latitude;
            double lngI = recI.longitude;
            String nameI = recI.name;
            for (int j = 0; j < numRec; j++) { // consider arcs in both directions
                ReadVRNodeCSV.DataField recJ = result.get(j);
                double latJ = recJ.latitude;
                double lngJ = recJ.longitude;
                String nameJ = recJ.name;

                double distIJ = greatCircleDistance(latI, lngI, latJ, lngJ); // meters
                double distJI = greatCircleDistance(latJ, lngJ, latI, lngI); // meters
                assertTrue(abs(distIJ - distJI) < meterTol);
                if (i == j) {
                    assertTrue(abs(distIJ) < meterTol);
                } else if ((abs(latI - latJ) < degreeTol) && (abs(lngI - lngJ) < degreeTol)) {
                    //System.out.printf("Co-Located: %5.2f meters, %18s , %18s \n", distIJ, nameI, nameJ);
                    assertTrue(abs(distIJ) < meterTol);
                } else {
                    //System.out.printf("GC Dist %18s -> %18s: %7.2f Km \n", nameI, nameJ, distIJ / 1000.0);
                    if (i < j) {  // output all potential undirected links in CSV format
                        System.out.printf("DistArc%03d , %s , %s , %.2f \n",
                                nodeCounter, nameI, nameJ, distIJ / 1000.0);
                        nodeCounter++;
                    }
                    assertTrue(meterTol < distIJ);
                }
            }
        }
    }

}
// =============================================================================
