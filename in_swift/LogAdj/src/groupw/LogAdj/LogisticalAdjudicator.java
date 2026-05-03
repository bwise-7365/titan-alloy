// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import groupw.BaseSim.Scheduler;
import groupw.DCVRP.*;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

import static groupw.DCVRP.VRController.TheVRC;

/**
 * This is the bridge between the DCVRP and DESim.
 *
 * It takes in a ScenarioRecord, configures the DESim with SEntity and TEntity objects,
 * then runs it to build the List of LogRecords. The key method is adjudicate().
 */
public class LogisticalAdjudicator {

    public int numCompleted;

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
        verifyDistinctNames();
        Map<String, ReadTransportVehicleCSV.DataField> vehicleDataMap = TheVRC.getVehicleDataMap();
        Map<String, Transport> vehicleMap = TheVRC.getVehicleMap();
        Map<String, Serial> serialMap = TheVRC.getSerialMap();
        Map<String, ReadSerialCSV.DataField> sRecMap = TheVRC.getSerialRecordMap();
        Map<String, ReadUnitCSV.DataField> unitMap = TheVRC.getUnitMap();

        for (String vName : vehicleDataMap.keySet()) {
            Transport t = vehicleMap.get(vName);
            TEntity te = new TEntity(t, this);
            te.initLocation(vehicleDataMap.get(vName).homeBase);
        }
        for (Serial s : serialMap.values()) {
            String unitName = sRecMap.get(s.name).unitName;
            String startNode = unitMap.get(unitName).startNodeName;
            s.currentNodeName = startNode;  // DCVRP library still needs this
            if (s.controller == null) {
                new SerialController(s);
            }
            SEntity se = new SEntity(s, this);
            se.initLocation(startNode);
        }
    }

    private void verifyDistinctNames() {
        Set<String> allNames = new HashSet<>();
        for (String n : TheVRC.getNodeMap().keySet()) {
            assert allNames.add(n) : "Duplicate name among nodes: " + n;
        }
        for (String n : TheVRC.getVehicleMap().keySet()) {
            assert allNames.add(n) : "Name collision (transport vs node): " + n;
        }
        for (String n : TheVRC.getSerialMap().keySet()) {
            assert allNames.add(n) : "Name collision (serial vs node/transport): " + n;
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

    public Map<String, String> entityLocations() {
        Map<String, String> result = new HashMap<>();
        for (Map.Entry<Serial, SEntity> e : serialEntityMap.entrySet()) {
            result.put(e.getKey().name, e.getValue().getCurrLoc());
        }
        for (Map.Entry<Transport, TEntity> e : transportEntityMap.entrySet()) {
            result.put(e.getKey().name, e.getValue().getCurrLoc());
        }
        return result;
    }


    public void reportLocations(double cTime) {
        final double minTime = 72.45;
        final double maxTime = 73.00;
        if ((minTime < cTime) && (cTime < maxTime)) {
            System.out.printf("\n\n Entity locations at t=%8.3f\n", cTime);
            int nSrl = 0, nStk = 0, nTrn = 0;
            for (Map.Entry<Serial, SEntity> e : serialEntityMap.entrySet()) {
                nSrl++;
                SEntity se = e.getValue();
                if (se.state != SEntity.State.Stop) {
                    nStk++;
                    System.out.printf("  Serial %s @ %s [%s]%n",
                            e.getKey().name, se.getCurrLoc(), se.state);
                }
            }
            for (Map.Entry<Transport, TEntity> e : transportEntityMap.entrySet()) {
                TEntity te = e.getValue();
                if (!te.myTrans.name.startsWith("MV22-")) {// Skip the many MV22's for now
                    nTrn++;
                    System.out.printf("  Transport %s @ %s [%s]%n",
                            te.myTrans.name, te.getCurrLoc(), te.state);
                }
            }
            System.out.printf("%8.3f: %d serials (%d stuck), %d transports%n",
                              cTime, nSrl, nStk, nTrn);
            System.out.flush();
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
