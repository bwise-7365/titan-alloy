// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import groupw.BaseSim.EntEvent;
import groupw.DCVRP.ItineraryBuilder;
import groupw.DCVRP.Itinerary;

import groupw.DCVRP.Serial;
import groupw.DCVRP.Transport;
import groupw.Logistics.Manifest;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import static groupw.DCVRP.VRController.TheVRC;

/**
 * This encodes a finite state machine to represent what Transports do.
 *
 * Timewise, the main thing they do is follow Itineraries, picking up and dropping off as they go.
 * However, they use the ItineraryBuilder to decide which of the many available Serials
 * to pick up in order to minimize total (estimated) weighted quadratic lateness.
 * Currently, I don't explicitly destroy transports mid-Leg, but MAST does.
 * The planning involves several suboptimizations, such as weighted TSP.
 * See the DCVRP library for more details.
 */
public class TEntity extends LEntity {

    public enum State {Plan, Transfer, Move}

    // if nothing to do, wait this many hours (4 is reasonable)
    public static final double PLAN_INTERVAL = 1.0;

    /**
     * Create a new transport entity, at its home base, ready to plan.
     * @param tr
     * @param adj
     */
    public TEntity(Transport tr, LogisticalAdjudicator adj) {
        super(adj.mySim);
        myTrans = tr;
        myAdj = adj;
        state = State.Plan;
        currentNodeName = TheVRC.getVehicleDataMap().get(tr.name).homeBase;
        myTrans.currNode = TheVRC.getNodeMap().get(currentNodeName);
        adj.transportEntityMap.put(tr, this);
        mySim.addEvent(new EntEvent(this, mySim, 0.0));
    }

    public String status() {
        String hb0 = TheVRC.getVehicleDataMap().get(myTrans.name).homeBase;
        String hb1 = (null == hb0) ? "----" : hb0;
        String cnn = (null == myTrans.currNode) ? "----" : myTrans.currNode.name;
        String cTime = String.format("%08.3f", mySim.getCurrTime());
        String msg = "Transport " +String.format("%14s", myTrans.name)
                + " time " + cTime
                + " from " + hb1
                + " is at " + cnn
                + " in state " + state;
        return msg;
    }

    @Override
    public void process() {
        if (monitoredTransports.contains(myTrans.name)) {
            System.out.println("DEBUG: TEntity.process() - "+ status());
            // already have the transport
            System.out.flush();
        }
        switch (state) {
            case Plan:
                doPlan();
                break;
            case Transfer:
                doTransfer();
                break;
            case Move:
                doMove();
                break;
        }
        myAdj.reportLocations(mySim.getCurrTime(),  mySim.timeStamp());
    }

    public void doPlan() {
        if (myTrans.backlog == null || myTrans.backlog.numReservations() == 0) {
            mySim.addEvent(new EntEvent(this, mySim, mySim.getCurrTime() + PLAN_INTERVAL));
            return;
        }
        itinerary = ItineraryBuilder.TheIB.itineraryFromBacklog(
                myTrans.name, myTrans.backlog, mySim.getCurrTime(), myAdj.useMinTimeP);
        if (itinerary == null || itinerary.numLegs() == 0) {
            mySim.addEvent(new EntEvent(this, mySim, mySim.getCurrTime() + PLAN_INTERVAL));
            return;
        }
        ItineraryBuilder.TheIB.setTimeTable(itinerary, myTrans.name, mySim.getCurrTime());
        legIdx = 0;
        atSrc = true;
        atSrtTime = true;
        onBoard = new Manifest();
        state = State.Transfer;
        mySim.addEvent(new EntEvent(this, mySim, itinerary.legs.get(0).src.transferSrtTime));
    }

    public void doTransfer() {
        Itinerary.Leg leg = itinerary.legs.get(legIdx);
        double t = mySim.getCurrTime();
        String vName = myTrans.name;

        if (atSrc) {
            myTrans.currNode = leg.src.node;
        }

        if (atSrc && atSrtTime) {
            // T1: loading starts at src
            String nodeName = leg.src.node.name;
            List<String> sns = itemNames(leg.src.transfer);
            int[] pct = pctAreaWeight(onBoard);
            recordTransport(t, vName, LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.load, nodeName, pct[0], pct[1], sns);
            for (String sn : sns) {
                recordSerial(t, sn, LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.load, vName, nodeName);

                if (monitoredSerials.contains(sn)) {
                    System.out.println("DEBUG: EntTransfer.doTransfer(): " + status());
                    System.out.println("DEBUG: EntTransfer.doTransfer() - start loading monitored serial: " + sn);
                    System.out.println();
                    System.out.flush();
                }

            }
            atSrtTime = false;
            mySim.addEvent(new EntEvent(this, mySim, leg.src.transferEndTime));

        }
        else if (atSrc) {
            // T2: loading ends; depart
            String nodeName = leg.src.node.name;
            List<String> sns = itemNames(leg.src.transfer);
            onBoard = Manifest.add(onBoard, leg.src.transfer);
            int[] pct = pctAreaWeight(onBoard);
            recordTransport(t, vName, LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.load, nodeName, pct[0], pct[1], sns);
            for (String sn : sns) {
                recordSerial(t, sn, LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.load, vName, nodeName);
                Serial s = TheVRC.getSerialMap().get(sn);
                s.recordPickup(vName);

                if (monitoredSerials.contains(sn)) {
                    System.out.println("DEBUG: EntTransfer.doTransfer(): " + status());
                    System.out.println("DEBUG: EntTransfer.doTransfer() - finish loading monitored serial: " + sn);
                    System.out.println();
                    System.out.flush();
                }

                SEntity se = myAdj.serialEntityMap.get(s);
                if (se != null) {
                    se.state = SEntity.State.Move;
                    mySim.addEvent(new EntEvent(se, mySim, t));
                }
            }
            state = State.Move;
            myTrans.currNode = null; // moving
            mySim.addEvent(new EntEvent(this, mySim, leg.dst.transferSrtTime));

        }
        else {
            // T4: unloading ends
            String nodeName = leg.dst.node.name;
            myTrans.currNode = leg.dst.node; // still there
            List<String> sns = itemNames(leg.dst.transfer);
            onBoard = Manifest.sub(onBoard, leg.dst.transfer);
            int[] pct = pctAreaWeight(onBoard);
            recordTransport(t, vName, LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.unload, nodeName, pct[0], pct[1], sns);
            for (String sn : sns) {
                recordSerial(t, sn, LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.unload, vName, nodeName);
                Serial s = TheVRC.getSerialMap().get(sn);
                boolean atDest = nodeName.equals(s.deliveryNodeName);
                s.recordDropoff(nodeName, false);

                if (monitoredSerials.contains(sn)) {
                    System.out.println("DEBUG: EntTransfer.doTransfer(): " + status());
                    System.out.println("DEBUG: EntTransfer.doTransfer() - finish unloading monitored serial: " + sn);
                    System.out.println();
                    System.out.flush();
                }


                SEntity se = myAdj.serialEntityMap.get(s);
                if (se != null) {
                    se.state = atDest ? SEntity.State.Delivered : SEntity.State.Select;
                    mySim.addEvent(new EntEvent(se, mySim, t));
                }
            }
            if (legIdx + 1 < itinerary.numLegs()) {
                legIdx++;
                atSrc = true;
                atSrtTime = true;
                mySim.addEvent(new EntEvent(this, mySim, itinerary.legs.get(legIdx).src.transferSrtTime));
            } else {
                state = State.Plan;
                mySim.addEvent(new EntEvent(this, mySim, t + PLAN_INTERVAL));
            }
        }
    }

    public void doMove() {
        Itinerary.Leg leg = itinerary.legs.get(legIdx);
        double t = mySim.getCurrTime();
        String vName = myTrans.name;
        String nodeName = leg.dst.node.name;
        myTrans.currNode = leg.dst.node;
        List<String> sns = itemNames(leg.dst.transfer);
        int[] pct = pctAreaWeight(onBoard);
        recordTransport(t, vName, LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.unload, nodeName, pct[0], pct[1], sns);
        for (String sn : sns) {
            recordSerial(t, sn, LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.unload, vName, nodeName);

            if (monitoredSerials.contains(sn)) {
                System.out.println("DEBUG: EntTransfer.doTransfer(): " + status());
                System.out.println("DEBUG: EntTransfer.doTransfer() - start unloading monitored serial: " + sn);
                System.out.println();
                System.out.flush();
            }

        }
        state = State.Transfer;
        atSrc = false;
        mySim.addEvent(new EntEvent(this, mySim, leg.dst.transferEndTime));
    }

    private static List<String> itemNames(Manifest m) {
        return (m != null) ? new ArrayList<>(m.getItemNames()) : new ArrayList<>();
    }

    private void recordSerial(double time, String serialName, LogisticalAdjudicator.StartFinish sf,
                              LogisticalAdjudicator.LoadUnload lu, String transportName, String nodeName) {
        LogisticalAdjudicator.LogRecord r = myAdj.new SerialRecord(time, serialName, sf, lu, transportName, nodeName);
        r.recordEvent(r);
    }

    private void recordTransport(double time, String transportName, LogisticalAdjudicator.StartFinish sf,
                                 LogisticalAdjudicator.LoadUnload lu, String nodeName,
                                 int pctArea, int pctWeight, List<String> serialNames) {
        LogisticalAdjudicator.LogRecord r = myAdj.new TransportRecord(time, transportName, sf, lu, nodeName, pctArea, pctWeight, serialNames);
        r.recordEvent(r);
    }

    private int[] pctAreaWeight(Manifest m) {
        Map<String, Serial> sMap = TheVRC.getSerialMap();
        int pa = (int) Math.round(100.0 * ItineraryBuilder.manifestArea(m, sMap) / myTrans.getCargoArea());
        int pw = (int) Math.round(100.0 * ItineraryBuilder.manifestWeight(m, sMap) / myTrans.getCargoWeight());
        return new int[]{pa, pw};
    }

    public State state;
    public String currentNodeName;
    final Transport myTrans;
    final LogisticalAdjudicator myAdj;
    Itinerary itinerary = null;
    Manifest onBoard = new Manifest();
    int legIdx = 0;
    boolean atSrc = true;
    boolean atSrtTime = true;
}

// Copyright Group W, SPA. All Rights Reserved.
