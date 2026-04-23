// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import groupw.BaseSim.Scheduler;
import groupw.DCVRP.ItineraryBuilder;
import groupw.DCVRP.ReadDCVRScenarioCSV;
import groupw.DCVRP.VRController;

import java.util.ArrayList;
import java.util.List;

import static groupw.Network.NWUtils.DefaultSeedPRNG;

public class LogisticalAdjudicator {

    public LogisticalAdjudicator(ReadDCVRScenarioCSV.ScenarioRecord sr) {
        scenRec = sr;
        VRController.initialize(scenRec, DefaultSeedPRNG);
        ItineraryBuilder.initialize();
        mySim = new Scheduler();
    }

    public List<LogRecord> adjudicator() {
        logRecords.clear();
        mySim.run();
        return logRecords;
    }

    void addRecord(LogRecord r) {
        logRecords.add(r);
    }

    public abstract class LogRecord {
        public void recordEvent(LogRecord r) {
            logRecords.add(r);
        }

        @Override
        public abstract String toString();
    }

    public class SerialRecord extends LogRecord {
        public final double time;
        public final String serialName;
        public final String loadUnload;
        public final String transportName;
        public final String nodeName;

        public SerialRecord(double time, String serialName, String loadUnload,
                            String transportName, String nodeName) {
            this.time = time;
            this.serialName = serialName;
            this.loadUnload = loadUnload;
            this.transportName = transportName;
            this.nodeName = nodeName;
        }

        @Override
        public String toString() {
            return String.format("%08.4f hours", time) + ", Serial, " + serialName + ", " + loadUnload + ", "
                    + transportName + ", " + nodeName;
        }
    }

    public class TransportRecord extends LogRecord {
        public final double time;
        public final String transportName;
        public final String startFinish;
        public final String loadUnload;
        public final List<String> serialNames;
        public final String nodeName;

        public TransportRecord(double time, String transportName, String startFinish,
                               String loadUnload, List<String> serialNames, String nodeName) {
            this.time = time;
            this.transportName = transportName;
            this.startFinish = startFinish;
            this.loadUnload = loadUnload;
            this.serialNames = new ArrayList<>(serialNames);
            this.nodeName = nodeName;
        }

        @Override
        public String toString() {
            return String.format("%08.4f hours", time) + ", Transport, " + transportName + ", " + startFinish + ", "
                    + loadUnload + ", " + serialNames + ", " + nodeName;
        }
    }

    // these parameters control how it estimates the remaining delivery time
    boolean useMinTimeP = false; // use the average
    boolean randomTransportOrder = true;
    boolean randomSerialOrder = true;

    public final ReadDCVRScenarioCSV.ScenarioRecord scenRec;
    public final Scheduler mySim;
    private final List<LogRecord> logRecords = new ArrayList<>();
}
// Copyright Group W, SPA. All Rights Reserved.
