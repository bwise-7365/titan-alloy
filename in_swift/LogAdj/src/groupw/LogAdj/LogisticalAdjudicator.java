// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import groupw.BaseSim.Scheduler;
import groupw.DCVRP.ItineraryBuilder;
import groupw.DCVRP.ReadDCVRScenarioCSV;
import groupw.DCVRP.ReadSerialCSV;
import groupw.DCVRP.ReadUnitCSV;
import groupw.DCVRP.Serial;
import groupw.DCVRP.SerialController;
import groupw.DCVRP.Transport;
import groupw.DCVRP.VRController;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static groupw.Network.NWUtils.DefaultSeedPRNG;

public class LogisticalAdjudicator {

    public LogisticalAdjudicator(ReadDCVRScenarioCSV.ScenarioRecord sr, int rngSeed) {
        scenRec = sr;
        VRController.initialize(scenRec, rngSeed);
        ItineraryBuilder.initialize();
        mySim = new Scheduler();
        int offSet = 1173;
        mySim.setPRNG(offSet + rngSeed);
    }

    public List<LogRecord> adjudicator(double simDuration) {
        logRecords.clear();
        initializeDESim();
        mySim.run(simDuration);
        return logRecords;
    }

    public void initializeDESim() {
        for (Transport t : VRController.TheVRC.getVehicleMap().values()) {
            new TEntity(t, this);
        }
        Map<String, ReadSerialCSV.DataField> sRecMap = VRController.TheVRC.getSerialRecordMap();
        Map<String, ReadUnitCSV.DataField>   unitMap  = VRController.TheVRC.getUnitMap();
        for (Serial s : VRController.TheVRC.getSerialMap().values()) {
            String unitName = sRecMap.get(s.name).unitName;
            s.currentNodeName = unitMap.get(unitName).startNodeName;
            if (s.controller == null) {
                new SerialController(s, ItineraryBuilder.TheIB);
            }
            new SEntity(s, this);
        }
    }

    public enum LoadUnload  { load, unload }
    public enum StartFinish { start, finish }

    public abstract class LogRecord {
        public final double time;

        protected LogRecord(double time) {
            this.time = time;
        }

        public void recordEvent(LogRecord r) {
            logRecords.add(r);
        }

        @Override
        public abstract String toString();
    }

    public class SerialRecord extends LogRecord {
        public final String serialName;
        public final StartFinish startFinish;
        public final LoadUnload loadUnload;
        public final String transportName;
        public final String nodeName;

        public SerialRecord(double time, String serialName, StartFinish startFinish,
                            LoadUnload loadUnload, String transportName, String nodeName) {
            super(time);
            this.serialName = serialName;
            this.startFinish = startFinish;
            this.loadUnload = loadUnload;
            this.transportName = transportName;
            this.nodeName = nodeName;
        }

        @Override
        public String toString() {
            return String.format("%08.4f hours, Serial, %s, %s, %s, %s, %s",
                    time, serialName, startFinish, loadUnload, transportName, nodeName);
        }
    }

    public class TransportRecord extends LogRecord {
        public final String transportName;
        public final StartFinish startFinish;
        public final LoadUnload loadUnload;
        public final List<String> serialNames;
        public final String nodeName;

        public TransportRecord(double time, String transportName, StartFinish startFinish,
                               LoadUnload loadUnload, List<String> serialNames, String nodeName) {
            super(time);
            this.transportName = transportName;
            this.startFinish = startFinish;
            this.loadUnload = loadUnload;
            this.serialNames = new ArrayList<>(serialNames);
            this.nodeName = nodeName;
        }

        @Override
        public String toString() {
            return String.format("%08.4f hours, Transport, %s, %s, %s, %s, %s",
                    time, transportName, startFinish, loadUnload, serialNames, nodeName);
        }
    }

    public int rngSeed;
    boolean useMinTimeP = false;
    boolean randomTransportOrder = true;
    boolean randomSerialOrder = true;

    public final ReadDCVRScenarioCSV.ScenarioRecord scenRec;
    public final Scheduler mySim;
    public final Map<Serial, SEntity> serialEntityMap = new HashMap<>();
    public final Map<Transport, TEntity> transportEntityMap = new HashMap<>();
    private final List<LogRecord> logRecords = new ArrayList<>();
}
// Copyright Group W, SPA. All Rights Reserved.
