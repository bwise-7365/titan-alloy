// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import groupw.DCVRP.ItineraryBuilder;
import groupw.DCVRP.Itinerary;
import groupw.DCVRP.ReadTransportTypeCSV;
import groupw.DCVRP.Serial;
import groupw.DCVRP.Transport;
import groupw.Logistics.Manifest;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.Set;

import static groupw.DCVRP.VRController.TheVRC;

/**
 * Finite state machine for a Transport in the DES.
 *
 * States: Start → Scanning → Loading → Moving → Unloading → (Loading | Scanning)
 *
 * Loading and Unloading each fire N+2 DES events per leg: one initial precondition check,
 * N staggered individual serial pickups/dropoffs, then one final postcondition check.
 * Staggered time for item i: T_start + (i+1)·(T_end−T_start)/(N+2)
 */
public class TEntity extends LEntity {

    public enum State { Start, Scanning, Loading, Moving, Unloading, Stop }

    static final double PLAN_INTERVAL = 4.0;

    public TEntity(Transport tr, LogisticalAdjudicator adj) {
        super(adj.mySim);
        myTrans = tr;
        myAdj = adj;
        state = State.Start;
        adj.transportEntityMap.put(tr, this);
        scheduleAt(0.0);
    }

    /** Also sets myTrans.currNode, which DCVRP planning needs. */
    @Override
    public void initLocation(String nodeName) {
        super.initLocation(nodeName);
        myTrans.currentNodeName = nodeName; // TheVRC.getNodeMap().get(nodeName);
    }

    // ---- Location interface ----

    @Override
    protected void setLoc(String name) {
        myTrans.currentNodeName = name;
    }

    @Override
    public String getCurrLoc() {
        return myTrans.currentNodeName;
    }

    // ---- DES entry point ----

    @Override
    public void process() {
        if (monitoredTransports.contains(myTrans.name)) {
            System.out.println("DEBUG TEntity.process(): " + status() + " at time "+ String.format("%.4f", mySim.getCurrTime()));
            System.out.flush();
        }
        switch (state) {
            case Start:     doStart();     break;
            case Scanning:  doScanning();  break;
            case Loading:   doLoading();   break;
            case Moving:    doMoving();    break;
            case Unloading: doUnloading(); break;
            case Stop: break;
        }

        // only during a specified time interval
        myAdj.reportLocations(mySim.getCurrTime());
    }

    // ---- state handlers ----

    private void doStart() {
        state = State.Scanning;
        doScanning();
    }

    private void doScanning() {
        if (myTrans.backlog == null || myTrans.backlog.numReservations() == 0) {
            scheduleAt(mySim.getCurrTime() + PLAN_INTERVAL);
            return;
        }
        buildAndValidateItinerary();
        if (myItinerary == null || myItinerary.numLegs() == 0) {
            scheduleAt(mySim.getCurrTime() + PLAN_INTERVAL);
            return;
        }
        initLegState(0);
        state = State.Loading;
        scheduleAt(currentLeg().src.transferSrtTime);
    }

    private void doLoading() {
        List<String> sns = getItems(currentLeg().src.transfer);
        int N = sns.size();
        if (pickupCounter < 0) {
            verifyLoadPreconditions(sns);
            pickupCounter = 0;
            scheduleAt(staggeredTime(0, currentLeg().src));
        } else if (pickupCounter < N) {
            executePickup(sns.get(pickupCounter));
            pickupCounter++;
            double nextT = (pickupCounter < N)
                    ? staggeredTime(pickupCounter, currentLeg().src)
                    : currentLeg().src.transferEndTime;
            scheduleAt(nextT);
        } else {
            finaliseLoading(sns);
        }
    }

    private void doMoving() {
        arriveAtDst();
        notifyAllArrival();
        dropoffCounter = -1;
        state = State.Unloading;
        scheduleAt(mySim.getCurrTime());  // fire doUnloading at same sim time (T3)
    }

    /**
     * This steps through the serials, unloading them at even intervals.
     * Notice that each becomes eligible for pickup the instant it is dropped off,
     * so we cannot do any "final check" of them all, as the early ones
     * might have been picked up before we dropped off the last.
     */
    private void doUnloading() {
        List<String> sns = getItems(currentLeg().dst.transfer);
        int N = sns.size();
        if (dropoffCounter < 0) {
            verifyUnloadPreconditions(sns);
            dropoffCounter = 0;
            scheduleAt(staggeredTime(0, currentLeg().dst));
        } else if (dropoffCounter < N) {
            executeDropoff(sns.get(dropoffCounter));
            dropoffCounter++;
            double nextT = (dropoffCounter < N)
                    ? staggeredTime(dropoffCounter, currentLeg().dst)
                    : currentLeg().dst.transferEndTime;
            scheduleAt(nextT);
        } else {
          //  finaliseUnloading(sns);
        }
    }

    // ---- leg lifecycle ----

    private void buildAndValidateItinerary() {
        myItinerary = ItineraryBuilder.TheIB.itineraryFromBacklog(
                myTrans.name, myTrans.backlog, mySim.getCurrTime(), myAdj.useMinTimeP);
        if (myItinerary != null && myItinerary.numLegs() > 0) {
            ItineraryBuilder.TheIB.setTimeTable(myItinerary, myTrans.name, mySim.getCurrTime());
            validateItinerary();
        }
    }

    private boolean validateItinerary() {
        ReadTransportTypeCSV.DataField tType = TheVRC.getVehicleTypeMap().get(myTrans.type);
        Map<String, Serial> sMap = TheVRC.getSerialMap();
        Map<String, Set<String>> tdMap = TheVRC.getVehicleDomainMap();
        Map<String, Set<String>> paMap = TheVRC.getPortAccessMap();
        boolean ok = myItinerary.validP(tType, sMap, tdMap, paMap);
        if (!ok){
            System.out.flush();
        }
        return ok;
    }

    private void initLegState(int idx) {
        legIdx = idx;
        pickupCounter = -1;
        dropoffCounter = -1;
        if (idx == 0) onBoard = new Manifest();
    }

    private Itinerary.Leg currentLeg() {
        return myItinerary.legs.get(legIdx);
    }

    private void arriveAtDst() {
        String dstName = currentLeg().dst.node.name;
        setLoc(dstName);
        myTrans.currentNodeName = currentLeg().dst.node.name;
    }

    private void advanceToNextLegOrScan() {
        if (legIdx + 1 < myItinerary.numLegs()) {
            initLegState(legIdx + 1);
            state = State.Loading;
            scheduleAt(currentLeg().src.transferSrtTime);
        } else {
            state = State.Scanning;
            scheduleAt(mySim.getCurrTime() + PLAN_INTERVAL);
        }
    }

    // ---- loading phase ----

    private void verifyLoadPreconditions(List<String> sns) {
        boolean ok = validateItinerary();
        if (!ok) {
            assert ok : "Itinerary invalid at start of leg " + legIdx + " for: " + myTrans.name;
        }
        String nodeName = currentLeg().src.node.name;
        assertSerialsAtNode(nodeName, sns);
        int[] pct = pctAreaWeight(onBoard);
        recordTransport(LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.load, nodeName, pct[0], pct[1], sns);
        recordSerials(LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.load, nodeName, sns);
    }

    private void executePickup(String sn) {
        Double qty = currentLeg().src.transfer.getAvailable(sn);
        onBoard.addInventory(sn, qty != null ? qty : 1.0);
        notifyPickUp(TheVRC.getSerialMap().get(sn));
    }

    private void finaliseLoading(List<String> sns) {
        String nodeName = currentLeg().src.node.name;
        assertSerialsOnTransport(myTrans.name, sns);
        int[] pct = pctAreaWeight(onBoard);
        recordSerials(LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.load, nodeName, sns);
        recordTransport(LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.load, nodeName, pct[0], pct[1], sns);
        notifyAllDeparture();
        state = State.Moving;
        scheduleAt(currentLeg().dst.transferSrtTime);
    }

    // ---- unloading phase ----

    private void verifyUnloadPreconditions(List<String> sns) {
        assertSerialsOnTransport(myTrans.name, sns);
        String nodeName = currentLeg().dst.node.name;
        int[] pct = pctAreaWeight(onBoard);
        recordTransport(LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.unload, nodeName, pct[0], pct[1], sns);
        recordSerials(LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.unload, nodeName, sns);
    }

    private void executeDropoff(String sn) {
        Double qty = onBoard.getAvailable(sn);
        if (qty != null) onBoard.subtractInventory(sn, qty);
        notifyDropOff(TheVRC.getSerialMap().get(sn));
    }

    private void finaliseUnloading(List<String> sns) {
        String nodeName = currentLeg().dst.node.name;
        assertSerialsAtNode(nodeName, sns);
        int[] pct = pctAreaWeight(onBoard);
        recordSerials(LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.unload, nodeName, sns);
        recordTransport(LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.unload, nodeName, pct[0], pct[1], sns);
        advanceToNextLegOrScan();
    }

    // ---- serial notifications ----

    private void notifyPickUp(Serial s) {
        SEntity se = myAdj.serialEntityMap.get(s);
        if (se != null) se.receivePickUp(this);
    }

    private void notifyDropOff(Serial s) {
        SEntity se = myAdj.serialEntityMap.get(s);
        if (se != null) se.receiveDO(this);
    }

    private void notifyAllDeparture() {
        for (String sn : getItems(onBoard)) {
            SEntity se = myAdj.serialEntityMap.get(TheVRC.getSerialMap().get(sn));
            if (se != null) se.notifyDeparture(this);
        }
    }

    private void notifyAllArrival() {
        for (String sn : getItems(onBoard)) {
            SEntity se = myAdj.serialEntityMap.get(TheVRC.getSerialMap().get(sn));
            if (se != null) se.notifyArrival(this);
        }
    }

    // ---- assertion helpers ----

    private void assertSerialsAtNode(String nodeName, List<String> sns) {
        for (String sn : sns) {
            SEntity se = myAdj.serialEntityMap.get(TheVRC.getSerialMap().get(sn));
            if (se != null) assertLocsEqual(se.getCurrLoc(), nodeName);
        }
    }

    private void assertSerialsOnTransport(String tname, List<String> sns) {
        for (String sn : sns) {
            SEntity se = myAdj.serialEntityMap.get(TheVRC.getSerialMap().get(sn));
            if (se != null) assertLocsEqual(se.getCurrLoc(), tname);
        }
    }

    // ---- timing ----

    private double staggeredTime(int i, Itinerary.Stop stop) {
        int N = getItems(stop.transfer).size();
        return stop.transferSrtTime + (i + 1.0) * (stop.transferEndTime - stop.transferSrtTime) / (N + 2);
    }

    // ---- record helpers ----

    private void recordTransport(LogisticalAdjudicator.StartFinish sf, LogisticalAdjudicator.LoadUnload lu,
                                 String nodeName, int pctArea, int pctWeight, List<String> sns) {
        LogisticalAdjudicator.LogRecord r = myAdj.new TransportRecord(
                mySim.getCurrTime(), myTrans.name, sf, lu, nodeName, pctArea, pctWeight, sns);
        r.recordEvent(r);
    }

    private void recordSerials(LogisticalAdjudicator.StartFinish sf, LogisticalAdjudicator.LoadUnload lu,
                               String nodeName, List<String> sns) {
        double t = mySim.getCurrTime();
        for (String sn : sns) {
            LogisticalAdjudicator.LogRecord r = myAdj.new SerialRecord(t, sn, sf, lu, myTrans.name, nodeName);
            r.recordEvent(r);
        }
    }

    private int[] pctAreaWeight(Manifest m) {
        Map<String, Serial> sMap = TheVRC.getSerialMap();
        int pa = (int) Math.round(100.0 * ItineraryBuilder.manifestArea(m, sMap) / myTrans.getCargoArea());
        int pw = (int) Math.round(100.0 * ItineraryBuilder.manifestWeight(m, sMap) / myTrans.getCargoWeight());
        return new int[]{pa, pw};
    }

    private static List<String> getItems(Manifest m) {
        return (m != null) ? new ArrayList<>(m.getItemNames()) : new ArrayList<>();
    }

    @Override
    String status() {
        return String.format("Transport %s @ %s [%s]", myTrans.name, myTrans.currentNodeName, state);
    }

    // ---- fields ----

    public State state;
    final Transport myTrans;
    final LogisticalAdjudicator myAdj;
    Itinerary myItinerary = null;
    Manifest onBoard = new Manifest();
    int legIdx = 0;
    int pickupCounter = -1;
    int dropoffCounter = -1;
}

// Copyright Group W, SPA. All Rights Reserved.
