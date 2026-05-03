// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

/**
 * A Serial is a functional grouping of things that move together.
 * In particular, a Serial cannot be split across several modes of transport.
 * If you want that effect, define several smaller Serials.
 * For example, a notional platoon of 48 Marines might be composed of four Serial,
 * each corresponding to a section of 12 Marines. Another example might
 * be a single large piece of equipment to be moved.
 * We expect that a Manifest of Serials to be picked up or dropped off
 * will refer to them by SerialName and contain 0 or 1 of each Serial.
 */
public class Serial implements  CountedItem {
    public Serial(String sName, String uName) {
        this.name = sName;
        this.parentUnitName = uName;

        // reasonable initial defaults
        currentNodeName = "";
        inItineraryP = false;
        inTransitP = false;
        aliveP = true;
        this.idNum = ItemCounter.makeID();
    }

    public final String name;
    public final String parentUnitName;

    public SerialController controller = null;

    public double weight;
    public double area; // required cargo deck area, including margin (broken stowage), square meters or feet

    // By default, these fields are inherited from the parent unit,
    // but modification is allowed.
    public double deliveryPriority; // priority on delivery, i.e. weight in QWL objective function
    public double deliveryTime;
    public double deliveryWindow;
    public String deliveryNodeName;

    /**
     * Update variables to reflect status of in transit on a vehicle
     *
     * @param vehicleName name of vehicle by which picked up
     */
    public void recordPickup(String vehicleName) {
        currentNodeName = vehicleName;
        inTransitP = true;
        inItineraryP = true; // trivially
        currBacklog = null; // trivially
    }

    /**
     * Update variables to reflect status of no longer being part of any itinerary.
     * It might have been dropped off at the final destination,
     * dropped off at some intermediate location by a vehicle,
     * had its scheduled pickup aborted (e.g. destruction of the transport vehicle)
     */
    public void deschedule() {
        inTransitP = false;
        inItineraryP = false;
        currBacklog = null;
    }

    /**
     * The Serial object has to be marked as dead if the associated
     * entity (e.g. in MAST or SWIFT) is destroyed.
     */
    public void die() {
        aliveP = false;
        inTransitP = false;
        inItineraryP = false;
        currBacklog = null;
        currentNodeName = "";
    }

    /**
     * Update variables to reflect status of being at a node
     *
     * @param nodeName       name of node at which dropped off
     * @param moreScheduledP true if still on an existing itinerary
     */
    public void recordDropoff(String nodeName, boolean moreScheduledP) {
        currentNodeName = nodeName;
        inTransitP = false;
        inItineraryP = moreScheduledP;
        currBacklog = null;
    }

    public String currentNodeName = ""; // non-empty if stationary at this node at current simulation-time
    boolean inItineraryP = false; // included in an Itinerary in progress by a live vehicle (waiting or in transit)
    public Backlog currBacklog = null; // null if not in a backlog
    boolean inTransitP = false;
    boolean aliveP = true; // a reasonable initial default

    public boolean isAliveP() {
        return aliveP;
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

