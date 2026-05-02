// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import groupw.DCVRP.VRGraph.VRNode;
import groupw.DCVRP.VRGraph.VREdge;
import groupw.Logistics.Manifest;

import java.util.*;

import static groupw.DCVRP.ItineraryBuilder.manifestArea;
import static groupw.DCVRP.ItineraryBuilder.manifestWeight;

/**
 * This class describes what a Transport object is supposed to do with basic operations.
 * Notice that a 2-ton truck might have 'domains' {ImprovedRoad, UnimprovedRoad, CrossCountry}
 * so the itinerary builder must decide which domain to use on each leg.
 * Further, there might be several dirt tracks (CrossCountry) connecting two
 * nodes, so the builder must decide which specific arc to use.
 * It should be "compiled" into an executable format depending on
 * the system in which it is used.
 * For example, in the SimpleIADS, it might be used to build a PFSM.
 * The MAST simulation and SWIFT game engine would "compile" to something else.
 */
public class Itinerary implements CountedItem {

    public Itinerary(VRGraph g) {
        this.graph = g;
        idNum = ItemCounter.makeID();
    }

    /**
     * Make a copy, but not a deep clone, of this itinerary.
     * <p>
     * A literal clone would copy the ID number, which we do not want, as
     * well as duplicating the entire VR graph, which we really do not want.
     *
     * @return
     */
    public Itinerary deepCopy() {
        Itinerary itnry2 = new Itinerary(graph); // with new ID
        itnry2.legs = new ArrayList<>(legs.size());
        for (int i = 0; i < legs.size(); i++) {
            itnry2.legs.add(legs.get(i).deepCopy());
        }
        return itnry2;
    }

    public static class Stop {
        public Stop(VRNode n) {
            this.node = n;
            transfer = new Manifest();
            transferSrtTime = -1.0;
            transferEndTime = -1.0;
        }

        public Stop deepCopy() {
            Stop s2 = new Stop(this.node);
            s2.transfer = Manifest.add(this.transfer, null);
            s2.transferSrtTime = this.transferSrtTime;
            s2.transferEndTime = this.transferEndTime;
            return s2;
        }

        public VRNode node = null;
        public Manifest transfer = null;
        // for the Src Stop of a Leg, time is when the vehicle is present and can start picking up.
        // For the Dst Stop of a Leg, time is when the vehicle arrives and can start dropping off.
        // thus,
        // startTime(0, Src) = whenever the Itinerary starts
        // startTime(i, Src) = endTime(i-1, Dst)
        //
        // endTime(i, Src) = startTime(i, Src) + duration(pickup at Src i)
        //
        // startTime(i, Dst) = endTime(i, Src)  + duration(move Src i to Dst i)
        // endTime(i, Dst) = startTime(i, Dst) + duration(dropoff at Dst i)
        //
        // Even though (4, Dst) is the same place as (5, Src), time(5,Src) comes
        // after time(4, Dst) because of the duration(dropoff at Dst 4)
        //
        public double transferSrtTime = -1.0; //
        public double transferEndTime = -1.0; //
    }

    public static class Leg {
        public Leg(Stop s, Stop d, String dmn, VRGraph g) {
            src = s;
            dst = d;
            edge = g.domainPickEdge(src.node.name, dst.node.name, dmn);
            carry = null;
        }

        public Leg(Stop s, Stop d, VREdge e) {
            src = s;
            dst = d;
            edge = e;
            carry = null;
        }

        public Manifest pickup() {
            return src.transfer;
        }

        public Manifest dropoff() {
            return dst.transfer;
        }

        public Leg deepCopy() {
            Leg l2 = new Leg(src.deepCopy(), dst.deepCopy(), edge);
            l2.carry = Manifest.add(carry, null);
            return l2;
        }

        public Stop src = null;
        public Stop dst = null;
        public VREdge edge = null;
        public Manifest carry = null;
    }

    public List<Leg> legs;
    public VRGraph graph;


    /**
     * Append this Leg to the end of the Itinerary.
     * If necessary, initialize the List of legs.
     *
     * @param lg a Leg to be appended to the List of Legs
     */
    public void append(Leg lg) {
        if (null == legs) {
            legs = new ArrayList<>(2); // out and back at least
        }
        legs.add(lg);
        return;
    }

    /**
     * Return True if and only if the transfer times are non-decreasing
     */
    public boolean checkTimes() {
        boolean ok = true;
        if ((null != legs) && (0 < legs.size())) {
            for (int i = 0; ok && (i < legs.size()); i++) {
                boolean p = legs.get(i).src.transferSrtTime <= legs.get(i).src.transferEndTime;
                boolean m = legs.get(i).src.transferEndTime <= legs.get(i).dst.transferSrtTime;
                boolean d = legs.get(i).dst.transferSrtTime <= legs.get(i).dst.transferEndTime;
                ok = ok && (p && m && d);
            }
        }
        return ok;
    }

    /**
     * Return True if and only if every Leg has matched up nodes and arc.
     * Zero-length itineraries do not have mismatch and so are vacuously OK.
     *
     * @return true if matched, false otherwise.
     */
    public boolean checkArcsNodes() {
        int numLegs = legs.size();
        boolean ok = true;
        if (0 < numLegs) {
            for (int i = 0; ok && i < numLegs; i++) {
                VREdge e = legs.get(i).edge;
                if (null == e) {
                    ok = false;
                } else {
                    if (!(e.srcName.equals(legs.get(i).src.node.name))) {
                        ok = false;
                    }
                    if (!(e.tgtName.equals(legs.get(i).dst.node.name))) {
                        ok = false;
                    }
                }
            }
        }
        return ok;
    }

    /**
     * Return True if and only if all the legs connect in order with no gaps.
     * Zero-length routes are not connected; length-1 are (trivially)
     *
     * @return True if connected, False otherwise
     */
    public boolean checkConnected() {
        int numLegs = legs.size();
        boolean ok = (0 < numLegs);
        if (1 < numLegs) {
            for (int i = 0; ok && (i < numLegs - 1); i++) {
                String dstName = legs.get(i).dst.node.name;
                String srcName = legs.get(i + 1).src.node.name;
                if (!dstName.equals(srcName)) {
                    ok = false;
                }
            }
        }
        return ok;
    }

    /**
     * Return True if and only if the start of the first leg is the destination of the last leg.
     * Zero-length routes are not circular; length-1 might be.
     *
     * @return True if circular, False otherwise
     */
    public boolean checkCircular() {
        int numLegs = legs.size();
        boolean ok = (0 < numLegs);
        if (ok) {
            String sName = legs.get(0).src.node.name;
            String dName = legs.get(numLegs - 1).dst.node.name;
            ok = (sName.equals(dName));
        }
        return ok;
    }

    /**
     * Check that for each leg, the transport can not only transit the edge
     * but also access both ports.These are not the same: a heavy jet
     * would be able to transit an Air edge to a short, unimproved dirt strip port
     * at which only small light aircraft land.
     *
     * @param tType name of this transport's type
     * @param tdMap map from transport type names to the set of domains that transport can use
     * @param paMap a map from port names to the set of transport types that can use that port
     * @return
     */
    public boolean checkPortAccess(String tType, final Map<String, Set<String>> tdMap, final Map<String, Set<String>> paMap) {
        int numLegs = legs.size();
        boolean ok = true;
        for (int i = 0; ok && (i < numLegs); i++) {
            Set<String> srcTTypes = paMap.get(legs.get(i).src.node.name);
            Set<String> dstTTypes = paMap.get(legs.get(i).dst.node.name);
            if (!srcTTypes.contains(tType) || !dstTTypes.contains(tType)) {
                ok = false;
            }

            VREdge lde = legs.get(i).edge;
            if (!tdMap.get(tType).contains(lde.domain)) {
                ok = false;
            }

        }
        return ok;
    }

    /**
     * Split the n-th Leg into two Legs, with the specified node as a new stop.
     * Thus, splice X into leg #n (A,B) creates Leg #n (A,X) and Leg #n+1 (X, B).
     * Pickup will be at the Stop at start of new Leg #n+1,
     * Dropoff will be at the Stop at end of new Leg #n,
     * and both are at node X.
     * Notice that both new legs must use the same domain (for now)
     *
     * @param n          which Leg will be split
     * @param node       where the new stops will be placed.
     * @param dmn        the domain-name for both new legs
     * @param puManifest items (possibly NULL) to pickup in second new leg
     * @param doManifest items (possibly NULL) to dropoff in first new Leg
     */
    public void spliceOneLeg(int n, final VRNode node, String dmn, Manifest puManifest, Manifest doManifest) {
        final int numLegs = legs.size();
        if ((numLegs <= n) || (n < 0)) {
            throw new RuntimeException("Illegal leg number " + n + " with " + numLegs + " legs defined");
        }

        //System.out.printf("Splicing node %s into leg %d, (%s, %s) \n", node.name, n, legs.get(n).src.node.name, legs.get(n).dst.node.name);

        List<Leg> newLegs = new ArrayList<>(1 + numLegs);
        for (int i = 0; i < n; i++) {
            newLegs.add(legs.get(i));
        }
        Stop s1 = legs.get(n).src;
        Stop s4 = legs.get(n).dst;

        Stop s2 = new Stop(node);
        s2.transfer = doManifest;

        Stop s3 = new Stop(node);
        s3.transfer = puManifest;

        Leg leg12 = new Leg(s1, s2, dmn, graph);
        Leg leg34 = new Leg(s3, s4, dmn, graph);

        legs.set(n, null); // help GC

        newLegs.add(leg12);
        newLegs.add(leg34);

        for (int i = n + 1; i < numLegs; i++) {
            newLegs.add(legs.get(i));
        }

        legs = newLegs;
        return;
    }

    /**
     * Add this manifest into the 'pickup' at the Source of this Leg
     *
     * @param n          which Leg to modify
     * @param puManifest how much to increment the Source's pickup manifest
     */
    public void incrementLegPickup(int n, Manifest puManifest) {
        if (null != puManifest) {
            final int numLegs = legs.size();
            if ((numLegs <= n) || (n < 0)) {
                throw new RuntimeException("Illegal leg number " + n + " with " + numLegs + " legs defined");
            }
            Stop s = legs.get(n).src;
            s.transfer = Manifest.add(puManifest, s.transfer);
        }
    }

    /**
     * Add this manifest into the 'dropoff' at the Destination of this Leg
     *
     * @param n          which Leg to modify
     * @param doManifest how much to increment the Destination's dropoff manifest
     */
    public void incrementLegDropOff(int n, Manifest doManifest) {
        if (null != doManifest) {
            final int numLegs = legs.size();
            if ((numLegs <= n) || (n < 0)) {
                throw new RuntimeException("Illegal leg number " + n + " with " + numLegs + " legs defined");
            }
            Stop s = legs.get(n).dst;
            s.transfer = Manifest.add(doManifest, s.transfer);
        }
    }

    /**
     * Add the specified quantity of the named item into the 'dropoff' at the Destination of this leg
     *
     * @param n        which leg to modify
     * @param itemName which item to add
     * @param quantity how much of the item to add
     */
    public void incrementLegDropOff(int n, String itemName, double quantity) {
        final int numLegs = legs.size();
        if ((numLegs <= n) || (n < 0)) {
            throw new RuntimeException("Illegal leg number " + n + " with " + numLegs + " legs defined");
        }
        Manifest m = legs.get(n).dst.transfer;
        m.addInventory(itemName, quantity);
    }

    /**
     * Build a String of the node-names in each Leg
     *
     * @return node-names in each Leg
     */
    public String listLegNodes() {
        String desc = "[";
        for (int i = 0; i < legs.size(); i++) {
            String n1 = legs.get(i).src.node.name;
            String n2 = legs.get(i).dst.node.name;
            desc = desc + "(" + n1 + ", " + n2 + ") ";
        }
        desc = desc + "]";
        return desc;
    }

    /**
     * Print out formatted data on which picks up and drops off go where
     */
    public void displayManifests() {

        for (int i = 0; i < legs.size(); i++) {
            Stop srcStop = legs.get(i).src;
            Stop dstStop = legs.get(i).dst;
            System.out.printf("Leg %2d from %s to %s \n",
                    i, srcStop.node.name, dstStop.node.name);
            if ((null != srcStop.transfer) && (0 < srcStop.transfer.uniqueItemCount())) {
                System.out.printf("Leg %2d src pickup  (%.2f to %.2f hours) manifest %d items at %s: %s \n",
                        i, srcStop.transferSrtTime, srcStop.transferEndTime, srcStop.transfer.uniqueItemCount(), srcStop.node.name, srcStop.transfer.toString());
            }

            if ((null != dstStop.transfer) && (0 < dstStop.transfer.uniqueItemCount())) {
                System.out.printf("Leg %2d dst dropoff (%.2f to %.2f hours) manifest %d items at %s: %s \n",
                        i, dstStop.transferSrtTime, dstStop.transferEndTime, dstStop.transfer.uniqueItemCount(), dstStop.node.name, dstStop.transfer.toString());
            }

            System.out.println("");
            System.out.flush();

        }
    }

    /**
     * Create two legs for a round-trip from the start to the end and back.
     *
     * @param startNode   node from which to start
     * @param startDomain domain to use in first (outbound) leg
     * @param endNode     node at far end of trip
     * @param endDomain   domain to use in the second (return) leg
     */
    public void initializeRoundTrip(VRNode startNode, String startDomain, VRNode endNode, String endDomain) {
        legs = new ArrayList<>(2);
        Stop sa1 = new Stop(startNode);
        Stop sa2 = new Stop(endNode);
        Leg legA = new Leg(sa1, sa2, startDomain, graph);
        append(legA);

        Stop sb1 = new Stop(endNode);
        Stop sb2 = new Stop(startNode);
        Leg legB = new Leg(sb1, sb2, endDomain, graph);
        append(legB);
    }

    /**
     * Check that this itinerary was built without any "syntactic errors"
     * such as missing arcs, disconnected legs and so on.
     *
     * @return true if well-formed, false otherwise
     */
    public boolean checkWellFormed() {
        // check the simplest / fastest first
        boolean ok = checkTimes();
        ok = ok && checkArcsNodes();
        ok = ok && checkConnected();
        ok = ok && checkCircular();
        return ok;
    }

    /**
     * Indicate if the Itinerary visits the named node
     *
     * @param nodeName name of the node for which to look
     * @return True if visited in this itinerary, False otherwise
     */
    public boolean visitsNode(String nodeName) {
        boolean found = false;
        for (int i = 0; !found && i < legs.size(); i++) {
            if (nodeName.equals(legs.get(i).src.node.name)) {
                found = true;
            }
            if (nodeName.equals(legs.get(i).dst.node.name)) {
                found = true;
            }
        }
        return found;
    }

    /**
     * Get the list of node-names visited by this Itinerary
     * <p>
     * They are ordered by first visit: nodes visited multiple times once.
     * For example, 'home base' might be visited first and last, but
     * only appears once, first in the list.
     *
     * @return
     */
    public List<String> visitedNodes() {
        List<String> vList = new ArrayList<>(2);
        for (Leg lg : legs) {
            if (!vList.contains(lg.src.node.name)) {
                vList.add(lg.src.node.name);
            }
            if (!vList.contains(lg.dst.node.name)) {
                vList.add(lg.dst.node.name);
            }
        }
        return vList;
    }

    /**
     * Check that this route carries onlyknown cargo and satisfies the range, area and weight limits
     * of this type of transport - assuming it is well-formed.
     *
     * @param tType data on the type of transport to use this itinerary
     * @param sMap  map of serial-name to sMap, including area and weight data for each serial which might be moved
     * @param tdMap map from transport name to the set of domains the transport can traverse
     * @param paMap map from port name to the set of transport names which can use that port
     * @return false if at least one leg is infeasible, true otherwise.
     */
    public boolean checkFeasible(final ReadTransportTypeCSV.DataField tType,
                                 final Map<String, Serial> sMap,
                                 final Map<String, Set<String>> tdMap,
                                 final Map<String, Set<String>> paMap) {
        // simplest / fastest checks are done first
        boolean ok = (getLength() <= tType.oneWayRange);
        ok = ok && (undefinedCargo(sMap).isEmpty()); //TODO: make that value available for inspection

        ok = ok && (excessDropoff()); // but might not drop off everything

        ok = ok && checkPortAccess(tType.type, tdMap, paMap);

        ok = ok && checkAreaWeightLimits(tType.cargoArea, tType.cargoWeight, sMap);
        return ok;
    }

    public boolean validP(final ReadTransportTypeCSV.DataField tType,
                          final Map<String, Serial> sMap,
                          final Map<String, Set<String>> tdMap,
                          final Map<String, Set<String>> paMap) {
        boolean ok = checkWellFormed();
        ok = ok && checkFeasible(tType, sMap, tdMap, paMap);
        return ok;
    }

    /**
     * Check if any legs of the itinerary exceed the maximum area or weight.
     * Zero-length routes are thus OK because it is impossible for a leg to
     * be infeasible when there are no legs.
     *
     * @param maxArea   absolute maximum total area which can be carried
     * @param maxWeight absolute maximum total weight which can be carried
     * @param sMap      Map of sMap, by String
     * @return False if any leg is infeasible, True otherwise
     */
    public boolean checkAreaWeightLimits(double maxArea, double maxWeight, final Map<String, Serial> sMap) {
        final boolean displayP = false;
        boolean ok = true;
        int numLegs = legs.size();
        if (0 < numLegs) {
            for (int i = 0; ok && (i < numLegs); i++) {
                if (displayP) {
                    System.out.printf("Leg %02d src: %s, dst: %s\n",
                            i, legs.get(i).src.node.name, legs.get(i).dst.node.name);
                }
                // pickup at src of this leg
                final Manifest srcPickUp = legs.get(i).src.transfer;
                if (null != srcPickUp) {
                    if (displayP) {
                        System.out.printf("  Src pickup: %s\n", srcPickUp.toString());
                    }
                }
                if (null == legs.get(i).carry) {
                    ok = false;
                }
                if (displayP){
                    System.out.printf("   Carry: %s\n", legs.get(i).carry);
                }
                double as = manifestArea(legs.get(i).carry, sMap);
                double ws = manifestWeight(legs.get(i).carry, sMap);
                if ((maxArea < as) || (maxWeight < ws)) {
                    ok = false;
                }
                if (displayP) {
                    System.out.printf("Leg %02d carried from src to dst: as = %.2f , ws = %.2f ,",
                            i, as, ws);
                    System.out.printf("  %s\n", legs.get(i).carry.toString());
                }

                // dropoff at dst of this leg
                final Manifest dstDropOff = legs.get(i).dst.transfer;
                if (null != dstDropOff) {
                    if (displayP) {
                        System.out.printf("  Dst dropoff: %s\n", dstDropOff.toString());
                    }
                }
                /* Dropoff can only decrease cargo, so no need to check here
                double ad = manifestArea(carried, sMap);
                double wd = manifestWeight(carried, sMap);
                if ((maxArea < ad) || (maxWeight < wd)) {
                    ok = false;
                }
                */

                if (displayP) {
                    System.out.printf("Leg %02d finished with Manifest:", i);
                    System.out.printf("  %s\n", legs.get(i).carry.toString());
                    System.out.println("====");
                }
            }
        }
        return ok;
    }


    public boolean checkAreaWeightLimits_From_Scratch(double maxArea, double maxWeight, final Map<String, Serial> sMap) {
        final boolean displayP = false;
        boolean ok = true;
        int numLegs = legs.size();
        if (0 < numLegs) {
            Manifest carried = new Manifest(); // empty hence zero area and weight
            for (int i = 0; ok && (i < numLegs); i++) {
                if (displayP) {
                    System.out.printf("Leg %02d src: %s, dst: %s\n",
                                      i, legs.get(i).src.node.name, legs.get(i).dst.node.name);
                    System.out.printf("Leg %02d started with Manifest:", i);
                    System.out.printf("  %s\n", carried.toString());
                }
                // pickup at src of this leg
                final Manifest srcPickUp = legs.get(i).src.transfer;
                if (null != srcPickUp) {
                    if (displayP) {
                        System.out.printf("  Src pickup: %s\n", srcPickUp.toString());
                    }
                    carried = Manifest.add(carried, srcPickUp);
                }
                double as = manifestArea(carried, sMap);
                double ws = manifestWeight(carried, sMap);
                if ((maxArea < as) || (maxWeight < ws)) {
                    ok = false;
                }
                if (displayP) {
                    System.out.printf("Leg %02d carried from src to dst: as = %.2f , ws = %.2f ,",
                                      i, as, ws);
                    System.out.printf("  %s\n", carried.toString());
                }

                // dropoff at dst of this leg
                final Manifest dstDropOff = legs.get(i).dst.transfer;
                if (null != dstDropOff) {
                    if (displayP) {
                        System.out.printf("  Dst dropoff: %s\n", dstDropOff.toString());
                    }
                    carried = Manifest.sub(carried, dstDropOff);
                }
                /* Dropoff can only decrease cargo, so no need to check here
                double ad = manifestArea(carried, sMap);
                double wd = manifestWeight(carried, sMap);
                if ((maxArea < ad) || (maxWeight < wd)) {
                    ok = false;
                }
                */

                if (displayP) {
                    System.out.printf("Leg %02d finished with Manifest:", i);
                    System.out.printf("  %s\n", carried.toString());
                    System.out.println("====");
                }
            }
        }
        return ok;
    }



    /**
     * Sum of weight times distance over all legs
     *
     * @param sMap Map from serial names to Serial objects
     * @return False if any leg is infeasible, True otherwise
     */
    public double totalWeightDistance(final Map<String, Serial> sMap) {

        //System.out.printf("Itinerary legs: %s\n", listLegNodes());
        //displayManifests();
        //System.out.flush();

        double totalWD = 0.0;
        int numLegs = legs.size();
        if (0 < numLegs) {
            Manifest carried = new Manifest(); // empty hence zero area and weight
            for (int i = 0; i < numLegs; i++) {

                final Manifest srcPickUp = legs.get(i).src.transfer;
                carried = Manifest.add(carried, srcPickUp);

                double w = manifestWeight(carried, sMap);
                double d = legs.get(i).edge.trueLength;
                double wd = w * d;
                totalWD = totalWD + wd;

                final Manifest dstDropOff = legs.get(i).dst.transfer;
                carried = Manifest.sub(carried, dstDropOff);

            }
        }
        return totalWD;
    }

    /**
     * Check if any dropoff exceed what is available at that point
     *
     * @return
     */
    public boolean excessDropoff() {
        boolean ok = true;
        Manifest carried = new Manifest();
        for (int i = 0; ok && (i < legs.size()); i++) {

            Stop src = legs.get(i).src;
            carried = Manifest.add(carried, src.transfer); // handles NULL appropriately

            Stop dst = legs.get(i).dst;
            if (null != dst.transfer) {
                for (String itemName : dst.transfer.getItemNames()) {
                    double avail = carried.getAvailable(itemName);
                    double rqrd = dst.transfer.getAvailable(itemName);
                    if (rqrd > avail) {
                        ok = false;
                    }
                }
            }
        }
        return ok;
    }

    /**
     * Check that no serials are picked up after the first leg.
     * Notice that itineraries with zero or one legs trivially pass.
     *
     * @return
     */
    public boolean singlePickup() {
        boolean ok = true;
        int num = numLegs();
        if (1 < num) {
            for (int i = 1; ok && (i < num); i++) {
                Manifest m = legs.get(i).pickup();
                if ((null != m) && (0 < m.uniqueItemCount())) {
                    ok = false;
                }
            }
        }
        return ok;
    }

    /**
     * Check if any node other than the initial one appears twice.
     *
     * @return true if no self-intersection
     */
    public boolean noSelfIntersection() {
        boolean ok = true;
        int num = numLegs();
        if (2 < num) {
            int ic = num - 1;
            List<VRNode> nodeList = new ArrayList<>(ic);
            for (int i = 0; i < num; i++) {
                if (nodeList.contains(legs.get(i).src.node)) {
                    ok = false;
                } else {
                    nodeList.add(legs.get(i).src.node);
                }
            }
        }
        return ok;
    }

    /**
     * Get the set (possibly empty) of undefined items to be picked up or dropped off
     */
    public Set<String> undefinedCargo(final Map<String, Serial> sMap) {
        Set<String> undefined = new HashSet<>(0);
        int numLegs = legs.size();
        if (0 < numLegs) {
            for (int i = 0; i < numLegs; i++) {
                final Manifest srcPickUp = legs.get(i).src.transfer;
                undefined.addAll(undefinedItems(srcPickUp, sMap));

                final Manifest dstPickUp = legs.get(i).dst.transfer;
                undefined.addAll(undefinedItems(dstPickUp, sMap));
            }
        }
        return undefined;
    }

    /**
     * Get the set (possibly empty) of items in the Manifest which are not defined
     *
     * @param m    the Manifest to be checked
     * @param sMap Map from serial-names to Serial objects
     * @return
     */
    public static Set<String> undefinedItems(final Manifest m, final Map<String, Serial> sMap) {
        Set<String> undefined = new HashSet<>(0);
        if (null != m) {
            for (String itemName : m.getItemNames()) {
                Serial s = sMap.get(itemName);
                if (null == s) {
                    undefined.add(itemName);
                }
            }
        }
        return undefined;
    }

    /**
     * Assuming only serials are picked up, return the names of everything picked up at first leg.
     *
     * @return Set of names
     */
    public Set<String> serialNamesInitialPickUp() {
        Set<String> rsn = new HashSet<>();
        if (0 < numLegs()) {
            rsn = legs.get(0).pickup().getItemNames();
        }
        return rsn;
    }

    /**
     * Total length of this itinerary using only edges of the specified legs (if any)
     *
     * @return total itinerary length
     */
    public final double getLength() {
        double totalLength = 0.0;
        if (null != legs) {
            int numLegs = legs.size();
            if (0 < numLegs) {
                for (int i = 0; i < numLegs; i++) {
                    VREdge lde = legs.get(i).edge;
                    double legLength = lde.trueLength;
                    totalLength = totalLength + legLength;
                }
            }
        }
        return totalLength;
    }

    /**
     * Number of legs in this Itinerary (if any)
     *
     * @return number of legs
     */
    public final int numLegs() {
        int numLegs = (null == legs) ? 0 : legs.size();
        return numLegs;
    }

    /**
     * Return the time at which the last dropoff ends; -1 if no such.
     * @return
     */
    public final double finalDropOffTime() {
        double ft = -1.0;
        if (0 < numLegs()){
            Leg l = legs.get(legs.size() - 1);
            ft = l.dst.transferEndTime;
        }
        return ft;
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
