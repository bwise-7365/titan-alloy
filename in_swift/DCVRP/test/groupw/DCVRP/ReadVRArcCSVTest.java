// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import java.io.File;
import java.util.List;
import java.util.Set;

import static groupw.BaseSim.DSUtils.NAUTICAL_MILE;
import static groupw.BaseSim.DSUtils.greatCircleDistance;
import static junit.framework.TestCase.assertTrue;

import groupw.Network.NWUtils.Tuple3;
import org.junit.Test;


/**
 *
 * @author BenWise
 */

public class ReadVRArcCSVTest {


    public ReadVRArcCSVTest() {
    }



    @Test
    public void testReadCSV_A1() {
        String ifName = "geograph-arcs-A1.csv";
        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_A1 tests file, " + ifName + ", known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test";
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadVRArcCSV.DataField> result = ReadVRArcCSV.readCSV(ifPath, ifName);
        assertTrue(null != result); // known good for this test file
        int numRec = result.size();
        System.out.printf("Found %d records (excluding comments) \n", numRec);
        assertTrue(numRec == 612); // known count for this test file
        return;
    }

    /**
     * Read pre-existing data files and print draft version of network arcs.
     * True length is in NAUTICAL_MILE
     * It is very dense and some Sea arcs cut across islands.
     * Manual edit is recommended.
     */
    //@Test
    public void buildBasicArcs() {

        // These files should have been verified elsewhere, so we just verify that they are of the expected sizes.
        Tuple3<List<ReadPortAccessCSV.DataField>, List<ReadVRNodeCSV.DataField>, List<ReadTransportDomainCSV.DataField>> pr = get_PA_GN_TT();
        List<ReadPortAccessCSV.DataField> paResult = pr.get0();
        int paNumRec = paResult.size();
        System.out.printf("Found %d port access records (excluding comments) \n", paNumRec);
        assertTrue(paNumRec == 101); // known count for this test file

        List<ReadVRNodeCSV.DataField> gnResult = pr.get1();
        int gnNumRec = gnResult.size();
        System.out.printf("Found %d geonode records (excluding comments) \n", gnNumRec);
        assertTrue(gnNumRec == 18); // known count for this test file

        List<ReadTransportDomainCSV.DataField> tdResult = pr.get2();
        int ttNumRec = tdResult.size();
        System.out.printf("Found %d transport type records (excluding comments) \n", ttNumRec);
        assertTrue(ttNumRec == 5); // known count for this test file

        int arcNumID = 0;
        for (ReadVRNodeCSV.DataField srcGNRec : gnResult) {
            String srcPN = srcGNRec.name;
            for (ReadVRNodeCSV.DataField tgtPARec : gnResult) {
                String tgtPN = tgtPARec.name;
                if (!srcPN.equals(tgtPN)) {
                    Set<String> domains = UtilsDCVR.buildArcDomains(
                            srcPN, tgtPN,
                            paResult, tdResult);
                    if (!domains.isEmpty()) {
                        //System.out.printf("Ports %s and %s have shared domains: ", srcPN, tgtPN);
                        double metersArcLength = greatCircleDistance(
                                tgtPARec.latitude, tgtPARec.longitude,
                                srcGNRec.latitude, srcGNRec.longitude);
                        for (String d : domains) {
                            arcNumID++;
                            System.out.printf("Arc%04d , %s , %s , %s , %.3f \n",
                                    arcNumID, srcPN, tgtPN, d, metersArcLength / NAUTICAL_MILE);
                        }
                    }
                }
            }
        }
        return;
    }



    static public
    Tuple3<List<ReadPortAccessCSV.DataField>,  List<ReadVRNodeCSV.DataField>, List<ReadTransportDomainCSV.DataField>>
    get_PA_GN_TT() {

        String paName = "portaccess-A1.csv";
        String gnName = "geograph-nodes-A1.csv";
        String ttName = "transportdomains-A1.csv";

        System.out.flush();
        System.out.println("\n");
        System.out.println("testReadCSV_A1 tests files, "+paName+", "+gnName+ ", "+ttName+" known to be good");
        String currentDir = System.getProperty("user.dir") + File.separator + "test" ;
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifPath = currentDir + File.separator + "Data";

        List<ReadPortAccessCSV.DataField> paResult = ReadPortAccessCSV.readCSV(ifPath, paName);
        List<ReadVRNodeCSV.DataField> gnResult = ReadVRNodeCSV.readCSV(ifPath, gnName);
        List<ReadTransportDomainCSV.DataField> ttResult = ReadTransportDomainCSV.readCSV(ifPath, ttName);

        Tuple3<List<ReadPortAccessCSV.DataField>, List<ReadVRNodeCSV.DataField>, List<ReadTransportDomainCSV.DataField>>
                pr = new Tuple3<>(paResult, gnResult, ttResult);
        return pr;
    }


}

// =============================================================================

