/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.DCVRP;

import org.junit.Test;
import java.io.File;
import static junit.framework.TestCase.assertTrue;


public class ReadDCVRPNWTest {
    public ReadDCVRPNWTest() {
    }



    @Test
    public void testReadDCVRP() {
        // This test assumes the two files have been verified elsewhere,
        // so we just test that the final graph has the expected number of edges and nodes.

        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);

        String ifPath = currentDir + File.separator + "Data";
        String arcFileName = "geograph-arcs-A1.csv";
        String nodeFileName = "geograph-nodes-A1.csv";
        VRGraph g = VRGraph.readCSV(ifPath, arcFileName, nodeFileName);

        int numEdges = g.graph.edgeSet().size();
        int numNodes = g.graph.vertexSet().size();

        assertTrue(numEdges == 612);
        assertTrue(numNodes == 18);
        return;
    }
}

// =============================================================================
