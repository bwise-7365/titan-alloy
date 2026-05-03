// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import groupw.DCVRP.Backlog;
import groupw.DCVRP.Serial;
import groupw.DCVRP.Transport;
import groupw.DCVRP.TravelLog;
import groupw.Network.NWUtils;

import java.util.ArrayList;
import java.util.List;

import static groupw.DCVRP.VRController.TheVRC;

/**
 * Finite state machine for a Serial in the DES.
 *
 * States: Start → Scanning ⇄ Waiting → Loading → Moving → Unloading → (Scanning | Stop)
 *
 * Loading, Moving, and Unloading are passive: process() is a no-op for them.
 * All transitions through those states are driven by TEntity notification calls.
 */
public class SEntity extends LEntity {

    public enum State {Start, Scanning, Waiting, Loading, Moving, Unloading, Stop}

    static final double SCAN_INTERVAL = 2.0;

    public SEntity(Serial sr, LogisticalAdjudicator adj) {
        super(adj.mySim);
        mySerial = sr;
        myAdj = adj;
        myTL = new TravelLog();
        state = State.Start;
        adj.serialEntityMap.put(sr, this);
        scheduleAt(0.0);
    }

    // ---- Location interface ----

    @Override
    protected void setLoc(String name) {
        mySerial.currentNodeName = name;
    }

    @Override
    public String getCurrLoc() {
        return mySerial.currentNodeName;
    }



    // ---- DES entry point ----

    @Override
    public void process() {
        switch (state) {
            case Start:
                doStart();
                break;
            case Scanning:
                doScanning();
                break;
            case Waiting:
                doWaiting();
                break;
            default:
                break;  // Loading, Moving, Unloading, Stop — driven by TEntity
        }
    }

    // ---- active state handlers ----

    private void doStart() {
        if (mySerial.currentNodeName.equals(mySerial.deliveryNodeName)) {
            state = State.Stop;
        } else {
            state = State.Scanning;
            doScanning();
        }
    }

    private void doScanning() {if (monitoredSerials.contains(mySerial.name)) {
        System.out.printf("Serial %s @ %s scanning at time %.4f\n",
                          mySerial.name, mySerial.currentNodeName,
                          mySim.getCurrTime());
        System.out.flush();
    }
        List<String> transportNames = TheVRC.transportsAtHomeBase(mySerial.currentNodeName, myAdj.randomTransportOrder);
        List<Transport> transports = new ArrayList<>();
        for (String tn : transportNames){
            transports.add(TheVRC.getVehicleMap().get(tn));
        }
        NWUtils.Tuple2<Transport, Backlog.Reservation> result =
                mySerial.controller.selectBacklog(transports, mySim.getCurrTime(), myAdj.useMinTimeP);
        if (result != null) {
            result.get0().backlog.appendReservation(result.get1());
            mySerial.currBacklog = result.get0().backlog;
            state = State.Waiting;
        } else {
            scheduleAt(mySim.getCurrTime() + SCAN_INTERVAL);
        }
    }

    private void doWaiting() {
        // passive placeholder — TEntity drives the next transition
    }

    // ---- TEntity notifications ----

    public void receivePickUp(TEntity t) {

        if (monitoredSerials.contains(mySerial.name) || monitoredTransports.contains(t.myTrans.name)) {
            System.out.printf("Serial %s @ %s received pickup by %s @ %s at time %.4f\n",
                              mySerial.name, mySerial.currentNodeName,
                              t.myTrans.name, t.getCurrLoc(),
                              mySim.getCurrTime());
            System.out.flush();
        }

        verifyPrePickup(t);
        setLoc(t.myTrans.name);
        logPickUp(t.getCurrLoc(), t.myTrans.name);
        mySerial.recordPickup(t.myTrans.name);
        state = State.Loading;
        verifyPostPickup(t);
    }

    public void notifyDeparture(TEntity t) {
        state = State.Moving;
    }

    public void notifyArrival(TEntity t) {
        state = State.Unloading;
    }

    public void receiveDO(TEntity t) {

        if (monitoredSerials.contains(mySerial.name) || monitoredTransports.contains(t.myTrans.name)) {
            System.out.printf("Serial %s @ %s received drop off by %s @ %s at time %.4f\n",
                              mySerial.name, mySerial.currentNodeName,
                              t.myTrans.name, t.getCurrLoc(),
                              mySim.getCurrTime());
            System.out.flush();
        }

        verifyPreDropoff(t);
        setLoc(t.getCurrLoc());
        mySerial.recordDropoff(t.getCurrLoc(), false);
        logDropOff(t.getCurrLoc(), t.myTrans.name);
        verifyPostDropoff(t);
        if (mySerial.currentNodeName.equals(mySerial.deliveryNodeName)) {
            state = State.Stop;
            System.out.printf("Serial %s @ %s delivered to final node by %s @ %s at time %.4f\n",
                              mySerial.name, mySerial.currentNodeName,
                              t.myTrans.name, t.getCurrLoc(),
                              mySim.getCurrTime());
            myAdj.numCompleted++;
        } else {
            state = State.Scanning;
            doScanning();
        }
    }

    // ---- TravelLog management ----

    private void logPickUp(String nodeName, String transName) {
        pendingPickupStop = new TravelLog.Stop(nodeName, mySim.getCurrTime());
    }

    private void logDropOff(String dropNode, String transName) {
        TravelLog.Stop dstStop = new TravelLog.Stop(dropNode, mySim.getCurrTime());
        myTL.legs.add(new TravelLog.Leg(pendingPickupStop, transName, dstStop));
        pendingPickupStop = null;
    }

    // ---- verification helpers ----

    private void verifyPrePickup(TEntity t) {
        boolean ok = myTL.validP();
        if (!ok) {
            assert ok : "TravelLog invalid before pickup: " + mySerial.name;
        }
        verifyNode(t.getCurrLoc());
        assertLocsEqual(mySerial.currentNodeName, t.getCurrLoc());
    }

    private void verifyPostPickup(TEntity t) {
        verifyNode(t.getCurrLoc());
        assertLocsEqual(mySerial.currentNodeName, t.myTrans.name);
    }

    private void verifyPreDropoff(TEntity t) {
        verifyNode(t.getCurrLoc());
        assertLocsEqual(mySerial.currentNodeName, t.myTrans.name);
    }

    private void verifyPostDropoff(TEntity t) {
        verifyNode(t.getCurrLoc());
        assertLocsEqual(mySerial.currentNodeName, t.getCurrLoc());
        boolean ok = myTL.validP();
        if (!ok) {
            assert ok : "TravelLog invalid after dropoff: " + mySerial.name;
        }
    }

    @Override
    String status() {
        return String.format("Serial %s @ %s [%s]", mySerial.name, mySerial.currentNodeName, state);
    }

    // ---- fields ----

    public State state;
    final Serial mySerial;
    final LogisticalAdjudicator myAdj;
    final TravelLog myTL;
    private TravelLog.Stop pendingPickupStop = null;
}

// Copyright Group W, SPA. All Rights Reserved.
