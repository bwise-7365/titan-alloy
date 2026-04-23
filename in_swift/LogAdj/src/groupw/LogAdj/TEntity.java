// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import groupw.BaseSim.EntEvent;
import groupw.BaseSim.Entity;
import groupw.DCVRP.ItineraryBuilder;
import groupw.DCVRP.Itinerary;
import groupw.DCVRP.ReadTransportVehicleCSV;
import groupw.DCVRP.Serial;
import groupw.DCVRP.Transport;
import groupw.DCVRP.VRController;
import groupw.Logistics.Manifest;

import java.util.ArrayList;
import java.util.List;

public class TEntity extends Entity {

    public enum State { Plan, Transfer, Move }

    public static final double PLAN_INTERVAL = 1.0;

    public TEntity(Transport tr, LogisticalAdjudicator adj) {
        super(adj.mySim);
        myTrans = tr;
        myAdj = adj;
        state = State.Plan;
        currentNodeName = VRController.TheVRC.getVehicleDataMap().get(tr.name).homeBase;
        adj.transportEntityMap.put(tr, this);
        mySim.addEvent(new EntEvent(this, mySim,0.0));
    }

    @Override
    public void process() {
        switch (state) {
            case Plan:     doPlan();     break;
            case Transfer: doTransfer(); break;
            case Move:     doMove();     break;
        }
    }

    public void doPlan() {
        if (myTrans.backlog == null || myTrans.backlog.numReservations() == 0) {
            mySim.addEvent(new EntEvent(this, mySim,mySim.getCurrTime() + PLAN_INTERVAL));
            return;
        }
        itinerary = ItineraryBuilder.TheIB.itineraryFromBacklog(
                myTrans.name, myTrans.backlog, mySim.getCurrTime(), myAdj.useMinTimeP);
        if (itinerary == null || itinerary.numLegs() == 0) {
            mySim.addEvent(new EntEvent(this, mySim,mySim.getCurrTime() + PLAN_INTERVAL));
            return;
        }
        ItineraryBuilder.TheIB.setTimeTable(itinerary, myTrans.name, mySim.getCurrTime());
        legIdx = 0;
        atSrc = true;
        atSrtTime = true;
        state = State.Transfer;
        mySim.addEvent(new EntEvent(this, mySim,itinerary.legs.get(0).src.transferSrtTime));
    }

    public void doTransfer() {
        Itinerary.Leg leg = itinerary.legs.get(legIdx);
        double t = mySim.getCurrTime();
        String vName = myTrans.name;

        if (atSrc && atSrtTime) {
            // T1: loading starts at src
            String nodeName = leg.src.node.name;
            List<String> sns = itemNames(leg.src.transfer);
            recordTransport(t, vName, LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.load, sns, nodeName);
            for (String sn : sns) {
                recordSerial(t, sn, LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.load, vName, nodeName);
            }
            atSrtTime = false;
            mySim.addEvent(new EntEvent(this, mySim,leg.src.transferEndTime));

        } else if (atSrc) {
            // T2: loading ends; depart
            String nodeName = leg.src.node.name;
            List<String> sns = itemNames(leg.src.transfer);
            for (String sn : sns) {
                recordSerial(t, sn, LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.load, vName, nodeName);
                Serial s = VRController.TheVRC.getSerialMap().get(sn);
                s.recordPickup(vName);
                SEntity se = myAdj.serialEntityMap.get(s);
                if (se != null) {
                    se.state = SEntity.State.Move;
                    mySim.addEvent(new EntEvent(se, mySim, t));
                }
            }
            state = State.Move;
            mySim.addEvent(new EntEvent(this, mySim,leg.dst.transferSrtTime));

        } else {
            // T4: unloading ends
            String nodeName = leg.dst.node.name;
            List<String> sns = itemNames(leg.dst.transfer);
            recordTransport(t, vName, LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.unload, sns, nodeName);
            for (String sn : sns) {
                recordSerial(t, sn, LogisticalAdjudicator.StartFinish.finish, LogisticalAdjudicator.LoadUnload.unload, vName, nodeName);
                Serial s = VRController.TheVRC.getSerialMap().get(sn);
                boolean atDest = nodeName.equals(s.deliveryNodeName);
                s.recordDropoff(nodeName, false);
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
                mySim.addEvent(new EntEvent(this, mySim,itinerary.legs.get(legIdx).src.transferSrtTime));
            } else {
                state = State.Plan;
                mySim.addEvent(new EntEvent(this, mySim,t + PLAN_INTERVAL));
            }
        }
    }

    public void doMove() {
        Itinerary.Leg leg = itinerary.legs.get(legIdx);
        double t = mySim.getCurrTime();
        String vName = myTrans.name;
        String nodeName = leg.dst.node.name;
        List<String> sns = itemNames(leg.dst.transfer);
        recordTransport(t, vName, LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.unload, sns, nodeName);
        for (String sn : sns) {
            recordSerial(t, sn, LogisticalAdjudicator.StartFinish.start, LogisticalAdjudicator.LoadUnload.unload, vName, nodeName);
        }
        state = State.Transfer;
        atSrc = false;
        mySim.addEvent(new EntEvent(this, mySim,leg.dst.transferEndTime));
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
                                 LogisticalAdjudicator.LoadUnload lu, List<String> serialNames, String nodeName) {
        LogisticalAdjudicator.LogRecord r = myAdj.new TransportRecord(time, transportName, sf, lu, serialNames, nodeName);
        r.recordEvent(r);
    }

    public State state;
    public String currentNodeName;
    final Transport myTrans;
    final LogisticalAdjudicator myAdj;
    Itinerary itinerary = null;
    int legIdx = 0;
    boolean atSrc = true;
    boolean atSrtTime = true;
}
// Copyright Group W, SPA. All Rights Reserved.
