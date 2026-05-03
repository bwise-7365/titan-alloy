// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import org.junit.Test;

import groupw.DCVRP.ReadDCVRScenarioCSV;
import groupw.LogAdj.LogisticalAdjudicator.SerialRecord;
import groupw.LogAdj.LogisticalAdjudicator.TransportRecord;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import static groupw.DCVRP.ReadDCVRScenarioCSV.readScenarioC1;
import static groupw.DCVRP.ReadDCVRScenarioCSV.readStandardTestCase;
import static groupw.Network.NWUtils.DefaultSeedPRNG;

public class LogisticalAdjudicatorTest {

    @Test
    public void testCreate() {
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readStandardTestCase();
        double currTime = 17.2020430980028; // hours, which is 17 hours and 12 minutes and 7.35515281 seconds
        List<String> serials = new ArrayList<>();
        serials.add("testSerial_A");
        serials.add("testSerial_B");
        LogisticalAdjudicator la = new LogisticalAdjudicator(sRec, DefaultSeedPRNG);
        SerialRecord srA = la.new SerialRecord(currTime, "testSerial_A", LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.load, "trans_A", "node1");
        SerialRecord srB = la.new SerialRecord(currTime, "testSerial_B", LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.load, "trans_A", "node1");
        TransportRecord tr1 = la.new TransportRecord(currTime, "trans_A", LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.load, "node1", 0, 0, serials);

        System.out.println(srA);
        System.out.println(srB);
        System.out.println(tr1);
    }

    /**
     * This runs the DCVRP adjudication using the DESim API
     *
     * The SEntity and TEntity classes are the 'bridge' between the DESim API and the DCVRP API.
     * The most interesting time-frames for the standardTestCase are 150 and 650 hours.
     * I did not bother to convert units, so they print as seconds from DESim.
     * @throws IOException
     */
    @Test
    public void testItnry() throws IOException {
        double runTimeHours = 24 * 60; // scenario C1 finishes around 1191.6763 hours ~49.7 days)
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readScenarioC1(); // must be synchronized with graph generator test
        LogisticalAdjudicator la = new LogisticalAdjudicator(sRec, DefaultSeedPRNG);
        la.numCompleted = 0;
        List<LogisticalAdjudicator.LogRecord> records = la.adjudicate(runTimeHours);
        records.sort((a, b) -> Double.compare(a.time, b.time));
        try (PrintWriter pw = new PrintWriter(new FileWriter("logrun.txt"))) {
            for (LogisticalAdjudicator.LogRecord r : records) {
                pw.println(r);
            }
        }

        System.out.printf("Completed %d deliveries\n", la.numCompleted);
    }
}
// Copyright Group W, SPA. All Rights Reserved.
