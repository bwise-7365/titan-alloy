// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import org.junit.Test;

import groupw.DCVRP.ReadDCVRScenarioCSV;
import groupw.LogAdj.LogisticalAdjudicator.SerialRecord;
import groupw.LogAdj.LogisticalAdjudicator.TransportRecord;

import java.util.ArrayList;
import java.util.List;

public class LogisticalAdjudicatorTest {

    @Test
    public void testCreate() {
        ReadDCVRScenarioCSV.ScenarioRecord sRec = ReadDCVRScenarioCSV.readStandardTestCase();
        double currTime = 17.2020430833333; // hours, i.e. 17 hours and 12 minutes and 7.3551 seconds
        List<String> serials = new ArrayList<>();
        serials.add("testSerial_A");
        serials.add("testSerial_B");
        LogisticalAdjudicator la = new LogisticalAdjudicator(sRec);
        SerialRecord srA = la.new SerialRecord(currTime, "testSerial_A", "load", "trans_A", "node1");
        SerialRecord srB = la.new SerialRecord(currTime, "testSerial_B", "load", "trans_A", "node1");
        TransportRecord tr1 = la.new TransportRecord(currTime, "trans_A", "start", "load", serials, "node1");

        System.out.println(srA);
        System.out.println(srB);
        System.out.println(tr1);
    }
}
// Copyright Group W, SPA. All Rights Reserved.
