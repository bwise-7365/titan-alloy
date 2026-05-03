// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import groupw.BaseSim.EntEvent;
import groupw.BaseSim.Entity;
import groupw.BaseSim.Scheduler;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import static groupw.DCVRP.VRController.TheVRC;

public abstract class LEntity extends Entity {

    static final List<String> monitoredSerials =
            new ArrayList<>(Arrays.asList(
            //"413-INF-BN-SQD-0003"
            ));
    static final List<String> monitoredTransports =
            new ArrayList<>(Arrays.asList(
            //"MV22-18-USV",
            //"MV22-01-DMN"
            ));

    // ---- location — single source of truth ----

    //protected String currLoc;

    /** Called only by LogisticalAdjudicator during setup; never by subclasses directly. */
    public void initLocation(String nodeName) {
        verifyNode(nodeName);
        setLoc(nodeName);
    }

    /** Only mutation point for currLoc. */
    abstract protected void setLoc(String name);
    //{ currLoc = name; }

    abstract public String getCurrLoc(); //{ return currLoc; }


    // ---- verification helpers ----

    protected void verifyNode(String nname) {
        boolean ok = TheVRC.getNodeMap().containsKey(nname);
        if (!ok) {
            assert ok : "Unknown node: " + nname;
        }
    }

    protected void verifyTrans(String tname) {
        boolean ok = TheVRC.getVehicleMap().containsKey(tname);
        if (!ok) {
            assert ok : "Unknown transport: " + tname;
        }
    }

    protected void assertLocsEqual(String actual, String expected) {
        boolean ok = actual.equals(expected);
        if (!ok) {
            assert ok  : "Location mismatch: expected '" + expected + "' but was '" + actual + "'";
        }
    }
    // ---- scheduling convenience ----

    protected void scheduleAt(double t) {
        mySim.addEvent(new EntEvent(this, mySim, t));
    }

    // ---- subclass contract ----

    abstract String status();

    public LEntity(Scheduler s) {
        super(s);
    }
}

// Copyright Group W, SPA. All Rights Reserved.
