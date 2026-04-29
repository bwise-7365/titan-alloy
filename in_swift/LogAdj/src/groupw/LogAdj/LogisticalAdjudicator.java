// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import groupw.BaseSim.Scheduler;
import groupw.DCVRP.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

import static groupw.DCVRP.VRController.TheVRC;

/**
 * This is the bridge between the DCVRP and DESim.
 *
 * It takes in a ScenarioRecord, configures the DESim with SEntity and TEntity objects,
 * then runs it to build the List of LogRecords. The key method is adjudicate().
 */
public class LogisticalAdjudicator {

    public LogisticalAdjudicator(ReadDCVRScenarioCSV.ScenarioRecord sr, int rngSeed) {
        scenRec = sr;
        VRController.initialize(scenRec, rngSeed);
        ItineraryBuilder.initialize();
        mySim = new Scheduler();
        int offSet = 1173;
        mySim.setPRNG(offSet + rngSeed);
    }

    public List<LogRecord> adjudicate(double simDuration) {
        logRecords.clear();
        initializeDESim();
        mySim.run(simDuration);
        return logRecords;
    }

    public void initializeDESim() {
        Map<String, ReadTransportVehicleCSV.DataField> vehicleDataMap = TheVRC.getVehicleDataMap();
        Map<String, Transport> vehicleMap = TheVRC.getVehicleMap();
        Map<String, Serial> serialMap = TheVRC.getSerialMap();
        Map<String, ReadSerialCSV.DataField> sRecMap = TheVRC.getSerialRecordMap();
        Map<String, ReadUnitCSV.DataField> unitMap = TheVRC.getUnitMap();

        for (String vName : vehicleDataMap.keySet()) {
            ReadTransportVehicleCSV.DataField vData = vehicleDataMap.get(vName);
            Transport t = vehicleMap.get(vName);
            new TEntity(t, this);
            System.out.flush();
        }
        for (Serial s : serialMap.values()) {
            String unitName =  sRecMap.get(s.name).unitName;
            s.currentNodeName = unitMap.get(unitName).startNodeName;
            if (s.controller == null) {
                new SerialController(s, ItineraryBuilder.TheIB);
            }
            new SEntity(s, this);
        }
    }

    public enum LoadUnload {load, unload}

    public enum StartFinish {start, finish}

    /**
     * Base class for the two kinds of records.
     */
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

    /**
     * TransportRecord shows what was transferred where and when.
     * <p>
     * It includes the percentage of weight and space used, so we can
     * more easily see inefficient planning.
     */
    public class TransportRecord extends LogRecord {
        public final String transportName;
        public final StartFinish startFinish;
        public final LoadUnload loadUnload;
        public final String nodeName;
        public final int pctArea;
        public final int pctWeight;
        public final List<String> serialNames;

        public TransportRecord(double time, String transportName, StartFinish startFinish,
                               LoadUnload loadUnload, String nodeName, int pctArea, int pctWeight,
                               List<String> serialNames) {
            super(time);
            this.transportName = transportName;
            this.startFinish = startFinish;
            this.loadUnload = loadUnload;
            this.nodeName = nodeName;
            this.pctArea = pctArea;
            this.pctWeight = pctWeight;
            this.serialNames = new ArrayList<>(serialNames);
        }

        @Override
        public String toString() {
            return String.format("%08.4f hours, Transport, %s, %s, %s, %s, %d%%, %d%%, %s",
                    time, transportName, startFinish, loadUnload, nodeName, pctArea, pctWeight, serialNames);
        }
    }

    boolean useMinTimeP = false;
    boolean randomTransportOrder = true;

    public final ReadDCVRScenarioCSV.ScenarioRecord scenRec;
    public final Scheduler mySim;
    public final Map<Serial, SEntity> serialEntityMap = new HashMap<>();
    public final Map<Transport, TEntity> transportEntityMap = new HashMap<>();
    private final List<LogRecord> logRecords = new ArrayList<>();
}

// Copyright Group W, SPA. All Rights Reserved.
