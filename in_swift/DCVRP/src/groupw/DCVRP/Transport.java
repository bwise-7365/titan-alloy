// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import groupw.DCVRP.VRGraph.VRNode;
import groupw.Logistics.Manifest;

import static groupw.DCVRP.VRController.TheVRC;

/**
 * The transport object actually moves Serials from Node to Node.
 *
 * This needs to be connected to a simulated agent so that it gets
 * necessary simulation attributes such as position, alive or dead status, etc.
 *
 * @author BenWise
 */
public class Transport  implements  CountedItem {


    public Transport(String name, String type) {
        this.name = name;
        this.type = type;
        this.backlog = new Backlog(name);
        this.aliveP = true;
        this.idNum = ItemCounter.makeID();
    }

    /**
     * The Transport object has to be marked as dead if the associated
     * entity (e.g. in MAST or SWIFT) is destroyed.
     */
    public void die() {
        aliveP = false;

        // clearing the backlog marks all serials in it as no longer backlogged
        if (null != backlog) {
            backlog.clear();
            backlog = null;
        }

        // we clear the itinerary, marking every serial in it as no longer
        // in an itinerary. In addition, if the serial is aboard this vehicle, then it dies.
        if (null != itinerary) {
            for (Itinerary.Leg lg : itinerary.legs) {
                Manifest pum = lg.pickup();
                for (String sName : pum.getItemNames()) {
                    Serial s = TheVRC.getSerialMap().get(sName);
                    s.inTransitP = false;
                    s.inItineraryP = false;
                    s.currBacklog = null;
                    if (name.equals(s.currentNodeName)) {
                        s.die();
                    }
                }
            }
            itinerary = null;
        }
        currentNodeName = null;
    }

    /**
     * Build, but not necessarily use, an itinerary from current backlog
     *
     */
    public Itinerary buildItineraryFromBacklog(double currTime, boolean useMinTimeP) {
        Itinerary it = null;
        if ((null == backlog) || (null == iBuilder)) {
            return it;
        }
        it = iBuilder.itineraryFromBacklog(name, backlog, currTime, useMinTimeP);
        return it;
    }

    public String name = "";
    public String type = "";
    boolean aliveP = true; // a reasonable initial default
    public String currentNodeName = "";
    //public VRNode currNode = null;
    double cargoArea = 0.0; // square units (feet, meters) for cargo
    double cargoWeight = 0.0; // typical max cargo on realistic mission (not manufacturer's "max that can takeoff")
    public Backlog backlog = null;
    Itinerary itinerary = null;
    ItineraryBuilder iBuilder = null;



    public double getCargoArea() {
        return cargoArea;
    }

    public double getCargoWeight() {
        return cargoWeight;
    }

    /**
     * The numerical ID is mostly for identifying almost-anonymous
     * data structures during debugging. See the comments on CountedItem class.
     */
    @Override
    public long getID() {
        return idNum;
    }

    private final long idNum;
}

// =============================================================================
