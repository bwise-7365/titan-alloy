// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import org.junit.Test;

import groupw.DCVRP.ReadDCVRScenarioCSV;
import groupw.LogAdj.LogisticalAdjudicator.SerialRecord;
import groupw.LogAdj.LogisticalAdjudicator.TransportRecord;
import groupw.LogAdj.LogisticalAdjudicator.LogRecord;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;

import static groupw.DCVRP.ReadDCVRScenarioCSV.readScenarioC1;
import static groupw.DCVRP.ReadDCVRScenarioCSV.readStandardTestCase;
import static groupw.Network.NWUtils.DefaultSeedPRNG;
import static groupw.Network.NWUtils.makeSeed;

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
     * This runs the DCVRP adjudication using the DESim API with unopposed movement
     *
     * The SEntity and TEntity classes are the 'bridge' between the DESim API and the DCVRP API.
     * I did not bother to convert units, so they print as seconds from DESim.
     * @throws IOException
     */
    @Test
    public void testUnopposed() throws IOException {
        double runTimeHours = 24 * 60; // 60 days == 1440 hours, scenario C1 finishes around 1300-1306 hours, about 54.3 days)
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readScenarioC1(); // must be synchronized with graph generator test
        int numSerials = sRec.sRecords.size();
        int numRuns = 5;
        double minLogTime = Double.MAX_VALUE;
        double maxLogTime = 0.0;
        double minSimTime = Double.MAX_VALUE;
        double maxSimTime = 0.0;

        // run it several times to look for invariant-violations
        for (int i = 0; i < numRuns; i++) {

            long timeStart = System.currentTimeMillis();
            LogisticalAdjudicator la = new LogisticalAdjudicator(sRec, makeSeed(0)); //DefaultSeedPRNG
            List<LogRecord> records = la.adjudicate(runTimeHours);
            records.sort((a, b) -> Double.compare(a.time, b.time));
            try (PrintWriter pw = new PrintWriter(new FileWriter("logrun.txt"))) {
                for (LogRecord r : records) {
                    pw.println(r);
                }
            }

            LogRecord lastLR = records.get(records.size()-1);
            System.out.printf("Last log record: %s \n", lastLR);
            minLogTime = Math.min(minLogTime, lastLR.time);
            maxLogTime = Math.max(maxLogTime, lastLR.time);
            System.out.printf("Run %d/%d completed %d deliveries\n\n",
                              i+1, numRuns, la.numCompleted);

            // In uncontested scenarios, all serials are eventually delivered
            assert(numSerials == la.numCompleted);

            System.out.flush();
            long timeEnd = System.currentTimeMillis();
            double simTime = (timeEnd - timeStart)/1000.0;
            maxSimTime = Math.max(maxSimTime, simTime);
            minSimTime = Math.min(minSimTime, simTime);
        }



        System.out.printf("Completed %d runs, mean sim time [%.2f, %.2f] seconds, delivery times [%.2f, %.2f] hours\n",
                          numRuns, minSimTime, maxSimTime, minLogTime, maxLogTime);
    }

    /**
     * This runs the DCVRP adjudication using the DESim API with contested movement
     *
     * @throws IOException
     */
    @Test
    public void testContested() throws IOException {
        double runTimeHours = 24 * 90;
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readScenarioC1(); // must be synchronized with graph generator test
        int numSerials = sRec.sRecords.size();
        int numRuns = 50;
        double minLogTime = Double.MAX_VALUE;
        double maxLogTime = 0.0;
        double minSimTime = Double.MAX_VALUE;
        double maxSimTime = 0.0;
        int minDelivered = Integer.MAX_VALUE;
        int maxDelivered = 0;

        // run it several times to look for invariant-violations
        for (int i = 0; i < numRuns; i++) {

            long timeStart = System.currentTimeMillis();
            LogisticalAdjudicator la = new LogisticalAdjudicator(sRec, makeSeed(0)); // makeSeed(0) DefaultSeedPRNG
            var ke = new KEntity(la);
            List<LogRecord> records = la.adjudicate(runTimeHours);
            records.sort((a, b) -> Double.compare(a.time, b.time));
            try (PrintWriter pw = new PrintWriter(new FileWriter("logrun.txt"))) {
                for (LogRecord r : records) {
                    pw.println(r);
                }
            }

            LogRecord lastLR = records.get(records.size()-1);
            System.out.printf("Last log record: %s \n", lastLR);
            minLogTime = Math.min(minLogTime, lastLR.time);
            maxLogTime = Math.max(maxLogTime, lastLR.time);

            System.out.printf("Run %d/%d completed %d/%d deliveries\n\n",
                              i+1, numRuns, la.numCompleted, numSerials);
            minDelivered = Math.min(minDelivered, la.numCompleted);
            maxDelivered = Math.max(maxDelivered, la.numCompleted);

            // in contested scenarios, sometimes everything gets through,
            // but we expect that usually some serials are never delivered

            System.out.flush();
            long timeEnd = System.currentTimeMillis();
            double simTime = (timeEnd - timeStart)/1000.0;
            maxSimTime = Math.max(maxSimTime, simTime);
            minSimTime = Math.min(minSimTime, simTime);
        }



        System.out.printf("Completed %d runs, mean sim time [%.2f, %.2f] seconds, delivery times [%.2f, %.2f] hours, serials delivered [%d, %d]\n",
                          numRuns,
                          minSimTime, maxSimTime,
                          minLogTime, maxLogTime,
                          minDelivered, maxDelivered);
    }
}
// Copyright Group W, SPA. All Rights Reserved.
