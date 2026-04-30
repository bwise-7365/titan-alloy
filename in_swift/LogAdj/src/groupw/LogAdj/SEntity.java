// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import groupw.BaseSim.EntEvent;
import groupw.BaseSim.Entity;
import groupw.DCVRP.Backlog;
import groupw.DCVRP.Serial;
import groupw.DCVRP.Transport;
import groupw.Network.NWUtils;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static groupw.DCVRP.VRController.TheVRC;

/**
 * This encodes a finite state machine to represent what Serials do.
 *
 * The most important thing they do is estimate how long it would take to get to
 * the destination of their parent unit, then join the backlog the corresponding transport.
 * This involves a lot of A*-like estimation of delivery times, even when they cannot
 * go directly to the destination.
 * See the DCVRP library for more details.
 *
 */
public class SEntity extends Entity {
    // trigger breakpoints for debugging
    static final String  monitoredSerial = "9-INF-BDE-MTBR-010";
    static final List<String>  monitoredTransports = new ArrayList<>(Arrays.asList("LSM-03-SD" , "LSM-02-SJ", "LSM-01-IDM"));

    // if nothing to do, wait this many hours (4 is reasonable)
    public static final double SCAN_INTERVAL = 1.0;


    public enum State { Select, Wait, Move, Delivered }

    public SEntity(Serial sr, LogisticalAdjudicator adj) {
        super(adj.mySim);
        mySerial = sr;
        myAdj = adj;
        state = State.Select;
        adj.serialEntityMap.put(sr, this);
        mySim.addEvent(new EntEvent(this, mySim,0.0));
    }

    @Override
    public void process() {
        switch (state) {
            case Select:    doSelect();  break;
            case Wait:      doWait();    break;
            case Move:      doMove();    break;
            case Delivered: finish();    break;
        }
    }

    public void doSelect() {


        //String tStamp = this.mySim.timeStamp();
        if (monitoredSerial.equalsIgnoreCase(mySerial.name)) {
            System.out.println("DEBUG: SEntity.doSelect() - doSelect by monitored serial: " + monitoredSerial);
            System.out.flush();
        }

        List<Transport> transports = TheVRC.transportsAtNode(this.mySerial.currentNodeName);
        if (myAdj.randomTransportOrder) {
            transports = NWUtils.shuffle(transports, mySim.prng);
        }
        NWUtils.Tuple2<Transport, Backlog.Reservation> result =
                mySerial.controller.selectBacklog(transports, mySim.getCurrTime(), myAdj.useMinTimeP);
        if (result != null) {
            Transport t = result.get0();
            t.backlog.appendReservation(result.get1());
            mySerial.currBacklog = t.backlog;
            state = State.Wait;
        } else {
            mySim.addEvent(new EntEvent(this, mySim,mySim.getCurrTime() + SCAN_INTERVAL));
        }
    }

    public void doWait() {
        // passive — TEntity drives the transition to Move
    }

    public void doMove() {
        // passive — TEntity drives the transition to Delivered or Select
    }

    public void finish() {
        // terminal state
    }

    public State state;
    final Serial mySerial;
    final LogisticalAdjudicator myAdj;
}

// Copyright Group W, SPA. All Rights Reserved.
