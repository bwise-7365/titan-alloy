// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import java.util.*;

import static com.google.common.collect.Lists.reverse;
import static groupw.BaseSim.DSUtils.NAUTICAL_MILE;
import static groupw.BaseSim.DSUtils.greatCircleDistance;
import static groupw.DCVRP.VRController.TheVRC;
import static groupw.Network.NWUtils.*;
import static groupw.Network.NWUtils.ReportingLevel.Silent;
import static java.lang.Math.*;

import groupw.DCVRP.VRGraph.VRNode;
import groupw.DCVRP.VRGraph.VREdge;
import groupw.Logistics.Manifest;

/**
 * Class to hold methods which "decide" how to build or modify Itinerary objects.
 * The singleton, TheIB, holds parameters relevant to planning.
 *
 * Basic operations that do not require decisions (e.g. splice) go in the Itinerary class.
 * For example, deciding which VRNode to splice into which Leg goes in this ItineraryBuilder class.
 * This class builds and caches several useful maps.
 * In each method, the first, one-time reference to abcMap should be getABCMap() to
 * ensure the map is initialized. If the first textual reference is in a loop
 * (used multiple times), just put a getABCMap() call on a single line at top of method.
 * If unsure, use getABCMap().
 *
 * TODO: make this use a backlog of serials
 */
public class ItineraryBuilder {


    /**
     * Public method to initialize singleton
     */
    public static ItineraryBuilder initialize() {
        TheIB = new ItineraryBuilder();
        return TheIB;
    }
    /**
     * Rarely, it makes sense to clear and reset the ItineraryBuilder singleton
     */
    public static void clear() {
        TheIB = null;
    }

    /**
     * Private constructor to help enforce singleton
     */
    private ItineraryBuilder() {
    }


    static public ItineraryBuilder TheIB = null;

    /**
     * Level of reporting internal operations, from Silent, Low, ... High, Debugging.
     */
    ReportingLevel rLevel = Silent;

    /**
     * Potential intermediate locations must be at least 20% closer, from any direction
      */
    double maxRemainDist = 0.80;

    /**
     * Potential Intermediate nodes cannot increase distance more than this factor.
     * For example, in a straight-line route 10 units long, max side deviation of 5
     * makes two 5 by 5 by 7.0711 right triangles with new total distance 14.1422 units, or 41% increase.
     */
    double maxDetourFactor= 1.41422;

    /**
     * Return the best transit time in hours for the serial to get from src to tgt.
     * This considers all transport types which can carry the serial
     * and access the tgt. Thus, if tgt can be reached by jet and boat, and both
     * can carry the Serial, then the jet-time is returned. But if only the boat
     * can carry the Serial, then the boat-time is returned.
     * If none can, return -1.0 to indicate impossible.
     * This assumes cruise speeds are in knots (NM / Hour).
     *
     * @param s
     * @param srcNode
     * @param tgtNode
     * @return
     */
    public double estMinTransitTime(Serial s, final VRNode srcNode, final VRNode tgtNode) {
        // TODO: take into account sea-edges that loop around an island, not across it.
        double distNM = greatCircleDistance(srcNode.latitude, srcNode.longitude,
                tgtNode.latitude, tgtNode.longitude) / NAUTICAL_MILE;
        double maxKnots = -1.0;
        Set<String> transTypes = TheVRC.getPortAccessMap().get(tgtNode.name); // vehicle-types that can access this port
        TheVRC.getVehicleTypeMap();
        for (String tType : transTypes) {
            ReadTransportTypeCSV.DataField vt = TheVRC.vehicleTypeMap.get(tType);
            if ((s.area <= vt.cargoArea) && (s.weight <= vt.cargoWeight)) {
                maxKnots = max(vt.cruiseSpd, maxKnots);
            }
        }
        double minFeasibleHours = (maxKnots < 0.0) ? -1.0 : (distNM / maxKnots);
        return minFeasibleHours;
    }

    /**
     * Estimate how long it would take to transfer the manifest on/off vehicle
     * @param vName
     * @param m
     * @return estimated time
     */
    public double transferTime(String vName, Manifest m) {
        String vType = TheVRC.getVehicleDataMap().get(vName).type;
        ReadTransportTypeCSV.DataField ttRec = TheVRC.getVehicleTypeMap().get(vType);
        double fa = manifestArea(m, TheVRC.getSerialMap()) / ttRec.cargoArea;
        double fw = manifestWeight(m, TheVRC.getSerialMap()) / ttRec.cargoWeight;
        //double loadFraction = (fa > fw) ? fa : fw;
        double loadFraction = sqrt(((fa*fa)+(fw*fw))/2.0); // RMS, so higher has more 'weight'
        return ttRec.transferTime * loadFraction; // hours
    }

    /**
     * Return the average transit time in hours for the serial to get from src to tgt.
     * This considers all transport types which can carry the serial
     * and access the tgt. Thus, if tgt can be reached by jet and boat, and both
     * can carry the Serial, then the average of both times is returned.
     * If none can, return -1.0 to indicate impossible.
     * This assumes cruise speeds are in knots (NM / Hour).
     *
     * @param s
     * @param srcNode
     * @param tgtNode
     * @return
     */
    public double estAverageTransitTime(Serial s, final VRNode srcNode, final VRNode tgtNode) {
        // TODO: take into account sea-edges that loop around an island, not across it.
        double distNM = greatCircleDistance(srcNode.latitude, srcNode.longitude,
                tgtNode.latitude, tgtNode.longitude) / NAUTICAL_MILE;
        double sumHours = 0.0;
        double count = 0.0;
        Set<String> transTypes = TheVRC.getPortAccessMap().get(tgtNode.name); // vehicle-types that can access this port
        for (String tType : transTypes) {
            ReadTransportTypeCSV.DataField vt = TheVRC.getVehicleTypeMap().get(tType);
            if ((s.area <= vt.cargoArea) && (s.weight <= vt.cargoWeight)) {
                double et = distNM / vt.cruiseSpd;
                sumHours = sumHours + et;
                count = count + 1.0;
            }
        }
        double avrgFeasibleHours = (count < 1.0) ? -1.0 : (sumHours / count);
        return avrgFeasibleHours;
    }

    /**
     * Estimate the Quadratic Weighted Lateness of this Serial,
     * given the specified transport to get from
     * the reference location, at the reference time, to the serial's destination
     *
     * @param s          the serial to be moved
     * @param refTime    reference time, usually current simulation time (hours)
     * @param useMinTime if TRUE, use the minimum feasible transit time, otherwise use average
     * @param refNode    reference node, usually current location of the serial, sometimes hypothetical
     * @return
     */
    public double estQWL(Serial s, double refTime, boolean useMinTime, final VRNode refNode) {
        final double unitDeliveryTimeHours = s.deliveryTime; // often same as parent, but not always
        final double unitDeliveryWindowHours = s.deliveryWindow; // often same as parent, but not always
        final VRNode tgtNode = TheVRC.getNodeMap().get(s.deliveryNodeName);  // often same as parent, but not always

        double estTransitHours = 0.0;
        if (useMinTime) {
            estTransitHours = estMinTransitTime(s, refNode, tgtNode);
        } else {
            estTransitHours = estAverageTransitTime(s, refNode, tgtNode);
        }
        double estArrivalHours = refTime + estTransitHours;
        double lateness = max(0.0, (estArrivalHours - unitDeliveryTimeHours)) / unitDeliveryWindowHours;
        double fractionUsed = 1.0;
        if (unitDeliveryTimeHours > refTime) {
            fractionUsed = (estArrivalHours - refTime)/(unitDeliveryTimeHours - refTime);
        }
        double qwl = fractionUsed + 3.0 * (lateness * lateness);
        //double qwl = 1.0 + (lateness * lateness);
        return qwl;
    }

    /**
     * Find edges from home base to nodes closer to a serial's destination.
     * <p>
     * These are sorted by most desirable (low estimated remaining QWL) first.
     * Those get it into positions where it can likely get to its final destination quickly.
     * <p>
     * Because there might be several edges between the same pair of nodes,
     * we build a list of edges from the transport's home base to nodes
     * rather than a list of nodes.
     * The destination node must handle the transport, and the edge must be
     * one of the transport's domains.
     * For now, this requires that the serial start from the transport's home base.
     * TODO: make sure the intermediate node is not a dead end.
     * It must be the home base of at least one transport that can carry this serial.
     *
     * @param transportName
     * @param serialName
     * @result set of edges to potentially useful intermediate nodes
     */
    public List<VREdge> potentialIntermediateEdges(String transportName, String serialName, double currTime) {

        final ReadTransportVehicleCSV.DataField vRec = TheVRC.getVehicleDataMap().get(transportName);
        // TODO: handle case of NULL vRec (i.e. invalid transportName)
        //assertNotNull(vMap.get(transportName));
        final ReadTransportTypeCSV.DataField vtRec = TheVRC.getVehicleTypeMap().get(vRec.type);
        // domain-names this transport vehicle can use
        final Set<String> vehicleDomains = TheVRC.getVehicleDomainMap().get(vRec.type);

        final Serial serial = TheVRC.getSerialMap().get(serialName);

        final String hBaseName = vRec.homeBase;
        final String dstNodeName = serial.deliveryNodeName;
        //assertTrue(dstNodeName.equals("Dominica"));
        final String puName = serial.parentUnitName;
        final ReadUnitCSV.DataField puRec = TheVRC.getUnitMap().get(puName);
        //assertTrue(puRec.startNodeName.equals(hBaseName));

        final VRNode startNode = TheVRC.getNodeMap().get(hBaseName);
        final VRNode endNode = TheVRC.nodeMap.get(dstNodeName);

        // distances in nautical miles
        //double seDist0 = g.graph.getEdge(startNode, endNode).trueLength;  // if several, takes one (no criteria)
        double seDist = greatCircleDistance(startNode.latitude, startNode.longitude, endNode.latitude, endNode.longitude) / NAUTICAL_MILE;
        //double seDist = max(seDist0, seDist1);

        // find the set of nodes which are not too far out of the way, do get closer,
        // and which this transport can access.
        // Notice that we use GC distance as preliminary check
        Set<VRNode> potentialMiddleNodes = new HashSet<>(1);
        for (VREdge e : TheVRC.scenRecord.vrg.graph.edgeSet()) {
            if (hBaseName.equals(e.srcName)) { // does the edge start at home base?
                VRNode midNode = TheVRC.nodeMap.get(e.tgtName);
                double smDist = greatCircleDistance(startNode.latitude, startNode.longitude, midNode.latitude, midNode.longitude) / NAUTICAL_MILE;
                // Minimal itinerary to midpoint is just there and back
                // Notice that the vehicleTypeMap will definitely have been constructed here
                if ((2.0 * smDist) < TheVRC.vehicleTypeMap.get(vRec.type).oneWayRange) {
                    double meDist = greatCircleDistance(midNode.latitude, midNode.longitude, endNode.latitude, endNode.longitude) / NAUTICAL_MILE;
                    // We do not want a midpoint that is too far "out in left field", even though it might require a different vehicle to complete.
                    // the minimal possible complete itinerary is a triangle that goes to the midpoint then the end point then returns.
                    if ((smDist + meDist <= maxDetourFactor * seDist) && (meDist <= maxRemainDist * seDist)) {
                        if (TheVRC.getPortAccessMap().get(midNode.name).contains(vRec.type)) {

                            if (!TheVRC.isDeadEnd(midNode, serial)) {
                                potentialMiddleNodes.add(midNode);
                            }
                        }
                    }
                }
            }
        }
        Set<VREdge> potentialEdges = new HashSet<>(1);
        for (VRNode midNode : potentialMiddleNodes) {
            potentialEdges.addAll(TheVRC.scenRecord.vrg.multiDomainArcsBetween(startNode, midNode, vehicleDomains));
        }
        int numPotEdges = potentialEdges.size();
        List<Tuple2<VREdge, Double>> orderedPairs = new ArrayList<>(numPotEdges);
        for (VREdge e : potentialEdges) {
            double estTransTime = e.trueLength / vtRec.cruiseSpd;
            // Notice that qwl is based on not only transit time to traverse the edge
            // but also estimated time to continue on to the destination (not unlike A*)
            double qwl = estQWL(serial, currTime + estTransTime, false, TheVRC.nodeMap.get(e.tgtName));
            orderedPairs.add(new Tuple2<>(e, qwl));
        }

        // we want lowest estQWL first, so the least desirable go last.
        // While the edges all have default weight 1.0 (independent of vehicles, serials, time, etc)
        // the QWL of each pair can range from 0.0 for zero-delay after delivery (i.e. it went to destination)
        // to potentially very large (hundreds)
        orderedPairs.sort((Tuple2<VREdge, Double> pr1, Tuple2<VREdge, Double> pr2)
                ->
                (int) (signum(pr1.get1() - pr2.get1())));

/*
        System.out.printf("Sorted %d potential edges for %s on %s\n", orderedPairs.size(), serialName, transportName);
        for (Tuple2<VREdge, Double> pr : orderedPairs) {
            System.out.printf("  Edge (%14s, %14s, %4d) %8.3f \n",
                    pr.get0().srcName, pr.get0().tgtName, pr.get0().getID(), pr.get1());
        }
        System.out.println("-----------------------");
*/

        List<VREdge> orderedEdges = new ArrayList<>(numPotEdges);
        for (int i = 0; i < numPotEdges; i++) {
            orderedEdges.add(orderedPairs.get(i).get0());
        }
        return orderedEdges;
    }

    /**
     * Sort carriable serials by a combination of priority and quadratic lateness.
     * The ones with highest estQWL are the most urgent to move, so they are at the front the List.
     * The quadratic lateness is estimated from the current node to parent unit's
     * destination node. This is multiplied by the unit priority and the percent
     * of unit capability represented by the serial.
     * <p>
     * Considers only those serials which are alive, are not already at their destination, and are neither scheduled nor in transit.
     * Does not consider current or desired location of either serials or potential transport vehicles.
     * Considers only those serials which fit on the specified transport, if provided.
     * Before sorting, they are shuffled so that ties will be broken randomly:
     * the first suitable in sorted order depends on initial random order.
     * <p>
     * @param vName  name of a particular vehicle
     * @param currTime  current simulation time
     * @param useMinTimeP if True, estimate remaining time using fastest transport, otherwise use average time
     *
     */
    public List<Tuple2<String, Double>> sortHomeBaseSerialsByPrioritizedEstQWL(String vName, double currTime, boolean useMinTimeP) {
        Set<String> serialNames = carriableSerialsAtHomeBase(vName);
        ReadTransportVehicleCSV.DataField vRec = TheVRC.getVehicleDataMap().get(vName);
        String transType = vRec.type;
        List<Tuple2<String, Double>> sortedSerials = sortSerialsByPrioritizedEstQWL(transType, serialNames, currTime, useMinTimeP);
        return sortedSerials;
    }

    /**
     * Set of serials current at home base, needing carriage, and fit on the vehicle.
     *
     * @param vName
     * @return
     */
    public Set<String> OLDcarriableSerialsAtHomeBase(String vName) {
        Set<String> serialNames = new HashSet<>();
        ReadTransportVehicleCSV.DataField vRec = TheVRC.vehicleDataMap.get(vName);
        String transType = vRec.type;
        double maxArea = TheVRC.getVehicleTypeMap().get(transType).cargoArea;
        double maxWeight = TheVRC.getVehicleTypeMap().get(transType).cargoWeight;
        for (String sn : TheVRC.getSerialRecordMap().keySet()) {
            Serial s = TheVRC.getSerialMap().get(sn);
            if (vRec.homeBase.equals(s.currentNodeName)) {
                if (s.aliveP && !(s.inItineraryP || s.inTransitP) && (null == s.currBacklog) && !(s.currentNodeName.equals(s.deliveryNodeName))) {
                    double a = TheVRC.getSerialRecordMap().get(sn).area;
                    double w = TheVRC.getSerialRecordMap().get(sn).weight;
                    if ((a <= maxArea) && (w <= maxWeight)) {
                        serialNames.add(sn);
                    }
                }
            }
        }
        //serialNames = shuffle(serialNames, TheVRC.prng);
        return serialNames;
    }

    /**
     * Set of serials current at this vehicle's home base, needing carriage, and fit on the vehicle.
     *
     * @param vName
     * @return
     */
    public Set<String> carriableSerialsAtHomeBase(String vName) {
        Set<String> serialNames = new HashSet<>();
        ReadTransportVehicleCSV.DataField vRec = TheVRC.vehicleDataMap.get(vName);
        Set<String> carriables = carriableSerialsAtNode(vRec.homeBase);
        String transTypeName = vRec.type;
        double maxArea = TheVRC.getVehicleTypeMap().get(transTypeName).cargoArea;
        double maxWeight = TheVRC.getVehicleTypeMap().get(transTypeName).cargoWeight;
        for (String sn : carriables) {
            double a = TheVRC.getSerialRecordMap().get(sn).area;
            double w = TheVRC.getSerialRecordMap().get(sn).weight;
            if ((a <= maxArea) && (w <= maxWeight)) {
                serialNames.add(sn);
            }
        }
        //serialNames = shuffle(serialNames, TheVRC.prng);
        return serialNames;
    }

    /**
     * Set of serials currently at a node and needing carriage
     *
     * @param nodeName node to be checked
     * @return
     */
    public Set<String> carriableSerialsAtNode(String nodeName) {
        Set<String> serialNames = new HashSet<>();
        for (String sn : TheVRC.getSerialRecordMap().keySet()) {
            Serial s = TheVRC.getSerialMap().get(sn);
            if (nodeName.equals(s.currentNodeName)) {
                if (s.aliveP
                        && !s.inItineraryP
                        && !s.inTransitP
                        && (null == s.currBacklog)
                        && !(s.currentNodeName.equals(s.deliveryNodeName))) {
                    serialNames.add(sn);
                }
            }
        }
        //serialNames = shuffle(serialNames, TheVRC.prng);
        return serialNames;
    }

    /**
     * Sort carriable serials by a combination of priority and quadratic lateness.
     * The ones with highest estQWL are the most urgent to move, so they are at the front the List.
     * The quadratic lateness is estimated from the current node to parent unit's
     * destination node. This is multiplied by the unit priority and the percent
     * of unit capability represented by the serial.
     * <p>
     * Considers only those serials which are alive, are not already at their destination, and are neither scheduled nor in transit.
     * Does not consider current or desired location of either serials or potential transport vehicles.
     * Considers only those serials which fit on the specified transport, if provided.
     * Before sorting, they are shuffled so that ties will be broken randomly:
     * the first suitable in sorted order depends on initial random order.
     *
     * @param transType If a defined transport vehicle type, then the serials will be filtered to make sure they fit
     */
    public List<Tuple2<String, Double>> sortSerialsByPrioritizedEstQWL(String transType, Set<String> serialNames, double currTime, boolean useMinTimeP) {
        List<Tuple2<String, Double>> sortedSerials = new ArrayList<>();
        int numLate = 0;
        for (String sn : serialNames) {
            Serial s = TheVRC.getSerialMap().get(sn);
            ReadUnitCSV.DataField uRec = TheVRC.getUnitMap().get(s.parentUnitName);
            String startNodeName = s.currentNodeName;
            double pup = s.deliveryPriority; // often the same as the partent unit, but not always
            ReadSerialCSV.DataField sr = TheVRC.getSerialRecordMap().get(sn);
            double pc = TheVRC.getSerialRecordMap().get(sn).prctCap;
            double qwl = estQWL(s, currTime, useMinTimeP, TheVRC.getNodeMap().get(startNodeName));
            if (1.0 < qwl) { // actually expect lateness
                numLate++;
            }
            double pri = pup * pc * qwl;
            sortedSerials.add(new Tuple2<>(sn, pri));
        }

        sortedSerials.sort((Tuple2<String, Double> s1, Tuple2<String, Double> s2) -> (int) signum(s2.get1() - s1.get1()));
        //System.out.printf("There are %d serials defined, of which %d can be carried, of which %d are expected to be late: \n", getSerialMap().size(), sortedSerials.size(), numLate);
        return sortedSerials;
    }

    /**
     * Sort node-names by great circle distance to the reference node, closest first
     *
     * @param destSet     names of node
     * @param refNodeName name of node from which distance will be measured
     * @return sorted list, closest distances first
     */
    public List<String> gcDistSort(Set<String> destSet, String refNodeName) {
        List<String> destList = new ArrayList<>(destSet.size());
        for (String d : destSet) {
            destList.add(d);
        }
        // This sort is tricky with a lambda-fn
        VRNode refNode = TheVRC.getNodeMap().get(refNodeName);
        for (int i = 0; i < destList.size(); i++) {
            for (int j = i + 1; j < destSet.size(); j++) {
                String nnI = destList.get(i);
                VRNode dnI = TheVRC.getNodeMap().get(nnI);
                double dI = greatCircleDistance(refNode.latitude, refNode.longitude, dnI.latitude, dnI.longitude);

                String nnJ = destList.get(j);
                VRNode dnJ = TheVRC.getNodeMap().get(nnJ);
                double dJ = greatCircleDistance(refNode.latitude, refNode.longitude, dnJ.latitude, dnJ.longitude);
                if (dI > dJ) {
                    destList.set(i, nnJ);
                    destList.set(j, nnI);
                }
            }
        }
        return destList;
    }

    public double setTimeTable(Itinerary itnry, String vName, double startTime) {
        double lastTime = startTime;
        int nLegs = itnry.numLegs();
        if (0 == nLegs) {
            return lastTime;
        }
        String vType = TheVRC.getVehicleDataMap().get(vName).type;
        double speed = TheVRC.getVehicleTypeMap().get(vType).cruiseSpd;

        Manifest runningManifest = new  Manifest();
        for (int i = 0; i < nLegs; i++) {
            if (0 == i) {
                lastTime = startTime;
            } else {
                lastTime = itnry.legs.get(i - 1).dst.transferEndTime;
            }

            itnry.legs.get(i).src.transferSrtTime = lastTime; // time when pickups at src #i should start
            double pickUpInterval = transferTime(vName, itnry.legs.get(i).src.transfer);
            lastTime = lastTime + pickUpInterval;
            itnry.legs.get(i).src.transferEndTime = lastTime; // time when pickups at src #1 should end
            runningManifest = Manifest.add(runningManifest, itnry.legs.get(i).src.transfer);

            double moveInterval = itnry.legs.get(i).edge.trueLength / speed;
            lastTime = lastTime +  moveInterval;

            itnry.legs.get(i).carry = Manifest.add(runningManifest, null); // makes a fresh copy

            itnry.legs.get(i).dst.transferSrtTime = lastTime; // time when dropoffs at dst #i should start
            double dropOffInterval = transferTime(vName, itnry.legs.get(i).dst.transfer);
            lastTime = lastTime + dropOffInterval;
            itnry.legs.get(i).dst.transferEndTime = lastTime; // time when dropoffs at dst #1 should end
            runningManifest = Manifest.sub(runningManifest, itnry.legs.get(i).dst.transfer);

        }
        return lastTime;
    }

    /**
     * Build an itinerary that moves Serials from this vehicle's home base.
     *
     * The goal is to get them closer to their destination so we sort the possible
     * ones by estimated Quadratic Weighted Lateness and put those on first.
     *
     * It already produces reasonable drop-offs (e.g. uses final destination when both
     * final and intermediate are feasible), but the greedy-building process does not
     * always give a reasonable final order.
     *
     * @param vehicleName
     * @param currTime
     * @param useMinTimeP
     */
    public Itinerary buildHomeBaseItinerary(String vehicleName, double currTime, boolean useMinTimeP) {
        Set<String> serialNames = carriableSerialsAtHomeBase(vehicleName);
        Itinerary it = itineraryFromUnsortedSerials(vehicleName, serialNames, currTime, useMinTimeP);
        return it;
    }

    /**
     * Build an itinerary from unsorted Set of serials.
     *
     * The goal is to get them closer to their destination so we sort the possible
     * ones by estimated Quadratic Weighted Lateness and put those on first.
     *
     * It already produces reasonable drop-offs (e.g. uses final destination when both
     * final and intermediate are feasible), but the greedy-building process does not
     * always give a reasonable final order.
     *
     * @param vehicleName
     * @param unsortedSerials
     * @param currTime
     * @param useMinTimeP
     */
    public Itinerary itineraryFromUnsortedSerials(String vehicleName, Set<String> unsortedSerials, double currTime, boolean useMinTimeP) {
        final double limitTol = 0.001; // tolerate small round-off errors
        final double one = 1.0; // avoid pointless annotations
        ReadTransportVehicleCSV.DataField vRec = TheVRC.getVehicleDataMap().get(vehicleName);
        String homeBaseName = vRec.homeBase;
        // these are serials that might need to be moved and which fit on this vehicle.
        // They are sorted by priority-weighted estQWL from their current location, with the highest values first.
        // The highest values are the most urgent to move.
        // Not yet checked for range; they might be far away.
        List<Tuple2<String, Double>> sortedPairs = sortSerialsByPrioritizedEstQWL(vRec.type, unsortedSerials, currTime, useMinTimeP);
        //System.out.printf("Found %d potential serials for %s \n", sortedPairs.size(), vehicleName);
        // only continue if there are some candidate serials

        ReadTransportTypeCSV.DataField tType = TheVRC.getVehicleTypeMap().get(vRec.type);
        TheVRC.getSerialMap(); // ensure it is initialized

        List<String> sortedSerials = new ArrayList<>(1);
        for (Tuple2<String, Double> pr : sortedPairs) {
            final Serial s = TheVRC.serialMap.get(pr.get0());
            sortedSerials.add(s.name);
            // System.out.printf("%4d: Adding %24s %8.3f bound for %10s\n", sortedSerials.size(), pr.get0(), pr.get1(), s.deliveryNodeName);
        }
        // sortedSerials are still in order with highest estQWL first

        Itinerary itnry = itineraryFromSortedSerials(vehicleName, sortedSerials, currTime, useMinTimeP);
        return itnry;
    }

    public Itinerary itineraryFromBacklog(String vehicleName, Backlog bLog, double currTime, boolean useMinTimeP) {
        int numBLRes1 = bLog.numReservations();
        //System.out.printf("Building itinerary from Backlog of %d reservations \n", numBLRes1);
        List<String> sortedSerials = new ArrayList<>();
        for (Backlog.Reservation r : bLog.reservations) {
            sortedSerials.add(r.serialName);
        }
        Itinerary itnry = itineraryFromSortedSerials(vehicleName, sortedSerials, currTime, useMinTimeP);

        // We know the only reservations to be picked up are on the initial leg
        Set<String> reservedSerials = itnry.serialNamesInitialPickUp();

        int numItnryRes = reservedSerials.size();
        for (String rs : reservedSerials) {
            bLog.removeReservation(rs);
            Serial s = TheVRC.getSerialMap().get(rs);
            s.inItineraryP = true;
        }
        int numBLRes2 = bLog.numReservations();
        //System.out.printf("From %d in backlog, planned %d, leaving %d in backlog\n", numBLRes1, numItnryRes, numBLRes2);
        return itnry;
    }

    /**
     * Build an itinerary from sorted List of serials.
     *
     * The goal is to get them closer to their destination so we sort the possible
     * ones by estimated Quadratic Weighted Lateness and put those on first.
     *
     * It already produces reasonable drop-offs (e.g. uses final destination when both
     * final and intermediate are feasible), but the greedy-building process does not
     * always give a reasonable final order.
     *
     * @param vehicleName
     * @param sortedSerials names of serials, sorted by highest estQWL first
     * @param currTime
     * @param useMinTimeP
     */
    public Itinerary itineraryFromSortedSerials(String vehicleName, List<String> sortedSerials, double currTime, boolean useMinTimeP) {
        final double limitTol = 0.001; // tolerate small round-off errors
        final double one = 1.0; // avoid pointless annotations
        ReadTransportVehicleCSV.DataField vRec = TheVRC.getVehicleDataMap().get(vehicleName);
        String homeBaseName = vRec.homeBase;

        ReadTransportTypeCSV.DataField tType = TheVRC.getVehicleTypeMap().get(vRec.type);
        TheVRC.getSerialMap(); // ensure it is initialized

        double areaLimit = TheVRC.vehicleTypeMap.get(vRec.type).cargoArea;
        double weightLimit = TheVRC.vehicleTypeMap.get(vRec.type).cargoWeight;

        Set<String> destSet = new HashSet<>(1);
        int numDest = 0;
        for (String sName : sortedSerials) {
            destSet.add(TheVRC.serialMap.get(sName).deliveryNodeName);
            if (numDest != destSet.size()) {
                //System.out.printf("  New final destination %s for serial %s \n", TheVRC.serialMap.get(sName).deliveryNodeName, sName);
                numDest = destSet.size();
            }
        }
        //System.out.println("Delivery location unordered set: " + destSet);
        List<String> destList = reverse(gcDistSort(destSet, homeBaseName)); // furthest first
        //System.out.println("Final destination sorted (furthest first) list: " + destList);

        // Build several important data structures:
        // (*) Set of potentially useful final or intermediate nodes
        // (*) Map from each serial-name to the list of edges that get it closer to its destination.
        //     The ones likely to facilitate faster delivery are at the front of the list.
        // (*) List of all potential intermediate nodes in order from furthest to closest.
        Map<String, List<VREdge>> serialEdgeMap = new HashMap<>(sortedSerials.size());
        int minEdges = Integer.MAX_VALUE;
        int maxEdges = 0;
        double meanEdges = 0.0;
        Set<String> reachableNodeSet = new HashSet<>(1);
        for (String s : sortedSerials) {
            final List<VREdge> candidateEdges = potentialIntermediateEdges(vehicleName, s, currTime);
            minEdges = min(candidateEdges.size(), minEdges);
            maxEdges = max(candidateEdges.size(), maxEdges);
            meanEdges = meanEdges + candidateEdges.size();
            serialEdgeMap.put(s, candidateEdges);
            for (VREdge e : candidateEdges) {
                reachableNodeSet.add(e.tgtName);
            }
        }
        meanEdges = meanEdges / sortedSerials.size();
        //System.out.printf("Edge set size %d with range: %d, %.2f, %d \n", sortedSerials.size(), minEdges, meanEdges, maxEdges);
        //System.out.println("Set of reachable final or intermediate nodes: " + reachableNodeSet);

        List<String> reachableNodeList = reverse(gcDistSort(reachableNodeSet, homeBaseName)); // furthest first
        //System.out.println("Sorted (furthest first) list of reachable final or intermediate nodes: " + reachableNodeList);

        Manifest iPickup = new Manifest();
        double iArea = 0.0;
        double iWeight = 0.0;
        Itinerary itnry = new Itinerary(TheVRC.scenRecord.vrg);

        double t = setTimeTable( itnry, vehicleName, currTime);
        //System.out.printf("Itinerary (zero items) ends at time %.2f\n", t);

        int numAdded = 0;
        // These still have highest estQWL first
        for (int sNdx = 0; sNdx < sortedSerials.size(); sNdx++) { // allow me to start far down the list
            String serialName = sortedSerials.get(sNdx);
            boolean added = false;
            Serial s = TheVRC.serialMap.get(serialName);
            //System.out.printf("Processing serial %4d/%4d, %s bound for %10s\n", sNdx, serials.size(), serialName, s.deliveryNodeName);
            if (!s.aliveP || s.inTransitP || s.inItineraryP) {
                continue; // skip to the next one
            }
            if ((iArea + s.area > areaLimit + limitTol) || (iWeight + s.weight > weightLimit + limitTol)) {
                continue; // skip to the next one
            }

            /*
            System.out.printf("Serial %24s %6.2f, %6.2f fits, area = %6.2f/%6.2f, weight = %6.2f/%6.2f \n",
                    serialName, s.area, s.weight,
                    iArea, areaLimit,
                    iWeight, weightLimit);
            */

            /*
            if (0 < itnry.numLegs()) {
                // check if itinerary already includes a potential drop off and try to drop off there
                List<VREdge> potentialEdges = serialEdgeMap.get(serialName); // scan edges in best-first order
                for (int i = 0; !added && (i < potentialEdges.size()); i++) {
                    VREdge e = potentialEdges.get(i);
                    // scan the itinerary's dst nodes to see if any match
                    for (int j = 0; !added && (j < itnry.numLegs()); j++) {
                        if (e.tgtName.equals(itnry.legs.get(j).dst.node.name)) {
                            itnry.incrementLegDropOff(j, serialName, one);
                            added = true;
                            iPickup.addInventory(serialName, one);
                            itnry.legs.get(0).src.transfer = iPickup;
                            numAdded++;
                            //System.out.printf("Serial %s will be dropped off at %s\n", serialName, itnry.legs.get(j).dst.node.name);
                        }
                    }
                }
            }
            */


            // If the itinerary is empty, just take the most advantageous edge for this Serial, i.e. the first
            if (0 == itnry.numLegs()) {
                VREdge e0 = serialEdgeMap.get(serialName).get(0);
                itnry.initializeRoundTrip(TheVRC.nodeMap.get(homeBaseName), e0.domain,
                        TheVRC.nodeMap.get(e0.tgtName), e0.domain);

                // add to dropoff at end of first leg
                /*
                System.out.printf("Created round-trip of %d legs and length %.2f from %s to %s and back.\n",
                        itnry.numLegs(), itnry.getLength(),
                        itnry.legs.get(0).src.node.name, itnry.legs.get(1).src.node.name);
                */
                itnry.incrementLegDropOff(0, serialName, one);
                added = true;
                iPickup.addInventory(serialName, one);
                itnry.legs.get(0).src.transfer = iPickup;
                numAdded++;
                //System.out.printf("Serial %s will be dropped off at %s\n", serialName, itnry.legs.get(0).dst.node.name);
                //System.out.flush();
            }

            // if we arrive at this line without adding anything, we know the following conditions hold.
            // (*) There is a round-trip itinerary in place with at least one serial dropped off at the far end.
            // On the first pass, with zero legs, such an itinerary would have been created, and something
            // added. But we're considering the case where nothing was added in this pass through the loop.
            // (*) The current serial does not have any suggested intermediate nodes on the current itinerary.
            // If it did, it would have been added to the appropriate manifest already.
            // Thus, we have to consider adding a new stop to the Itinerary.
            // The challenge is to check each leg to insert it so that the extended route is still
            // range-feasible and cargo-feasible, and pick the one with the least ton-miles.
            // This requires a double-loop over potentially useful edges and over legs which might be split.
            if (!added) {

                // big splice goes here
                Itinerary newIT = bestSplice(itnry,s, tType, serialEdgeMap);
                if (null != newIT) {
                    itnry = newIT;
                    added = true;
                    numAdded++;
                }
            }

            if (added) {
                s.inItineraryP = true;
                iPickup = itnry.legs.get(0).src.transfer;
                iArea = manifestArea(iPickup, TheVRC.serialMap);
                iWeight =  manifestWeight(iPickup, TheVRC.serialMap);
                //System.out.printf("Current number added %2d on legs: %s \n", numAdded, itnry.listLegNodes());
                //System.out.flush(); // place for breakpoint

                /*
                System.out.printf("Serial %24s will be loaded at %s, area = %6.2f/%6.2f, weight = %6.2f/%6.2f \n",
                        serialName, homeBaseName,
                        iArea, areaLimit,
                        iWeight, weightLimit);
                System.out.println("");
                System.out.flush();
                */
            }

            boolean wf = itnry.checkWellFormed();
            boolean fsbl = itnry.checkFeasible(tType, TheVRC.serialMap, TheVRC.vehicleDomainMap, TheVRC.portAccessMap);
            if (!wf || !fsbl){
                //System.out.flush(); // place for a breakpoint
            }
            //System.out.flush(); // place for a breakpoint

        }

        // initially pickup everything we intend to dropoff later
        //System.out.printf("Added %2d unique items \n", numAdded);
        return itnry;
    }

    /**
     * Use simple heuristics to decide a good place to splice in a drop-off for this serial.
     * The potential edges are in order of minimum estQWL first (quickest to deliver).
     * It looks for the first such edge that either is already on the itinerary or
     * can be feasibly spliced into the itinerary.
     * @param itnry the itinerary to be inserted
     * @param s the serial to be inserted
     * @param tType the type of transport used
     * @param serialEdgeMap Map from serial-name to sorted list of potential edges
     * @return improved Itinerary, if possible; NULL otherwise.
     */
    public Itinerary bestSplice(Itinerary itnry, final Serial s, final ReadTransportTypeCSV.DataField tType, final Map<String, List<VREdge>> serialEdgeMap) {
        final double one = 1.0;
        Itinerary splicedIT = null;
        String serialName = s.name;

        /*
        System.out.printf("Need route-splice from %d candidate edges for %s to reach %s: ", serialEdgeMap.get(serialName).size(), s.name, s.deliveryNodeName);
        for (VREdge e17 : serialEdgeMap.get(serialName)) {
            System.out.printf(" %s ", e17.tgtName);
        }
        System.out.println("");
        */

        double currentLength = itnry.getLength();
        double currentWD = itnry.totalWeightDistance(TheVRC.getSerialMap());
        //System.out.printf("Current legs: " + itnry.listLegNodes() + " with total length %.2f and total lb-NM %.5e\n", currentLength, currentWD);
        System.out.flush();
        List<VREdge> potentialEdges = serialEdgeMap.get(serialName);
        double minWD = Double.MAX_VALUE;
        for (int edgeNdx = 0; ((null == splicedIT) && (edgeNdx < potentialEdges.size())); edgeNdx++) {
            VREdge pe = potentialEdges.get(edgeNdx);
            //System.out.printf("Potential edge: %s->%s \n", pe.srcName, pe.tgtName);

            // check if this target is already on some leg
            for (int legNdx = 0; ((null == splicedIT) && (legNdx < itnry.numLegs())); legNdx++) {
                String dstName = itnry.legs.get(legNdx).dst.node.name;
                if (dstName.equals(pe.tgtName)) { // already on itinerary
                    Itinerary it2 = itnry.deepCopy();

                    //System.out.printf("Edge target %s is already destination of leg %d \n", pe.tgtName, legNdx);
                    //System.out.flush();

                    it2.legs.get(0).src.transfer.addInventory(serialName, one);
                    Manifest doManifest = new Manifest();
                    doManifest.addInventory(serialName, one);
                    doManifest = Manifest.add(doManifest, it2.legs.get(legNdx).dst.transfer);
                    it2.legs.get(legNdx).dst.transfer = doManifest;
                    boolean fs2 = it2.checkFeasible(tType, TheVRC.serialMap, TheVRC.getVehicleDomainMap(), TheVRC.getPortAccessMap());
                    if (fs2) {
                        splicedIT = it2;
                    }
                }
            }
            if (null == splicedIT) {
                for (int legNdx = 0; legNdx < itnry.numLegs(); legNdx++) {
                    String srcName = itnry.legs.get(legNdx).src.node.name;
                    String dstName = itnry.legs.get(legNdx).dst.node.name;
                    VREdge e12 = itnry.legs.get(legNdx).edge;
                    String dmn = e12.domain;// TODO allow domain-switching
                    //System.out.printf("Splicing %s into leg %d between %s and %s \n", pe.tgtName, legNdx, srcName, dstName);
                    VREdge e1x = itnry.graph.domainPickEdge(srcName, pe.tgtName, dmn);
                    VREdge ex2 = itnry.graph.domainPickEdge(pe.tgtName, dstName, dmn);
                    if ((null != e1x) && (null != ex2)) {
                        //System.out.printf("Edges %.2f %.2f vs %.2f, ", e1x.trueLength, ex2.trueLength,  e12.trueLength);
                        double delta = e1x.trueLength + ex2.trueLength - e12.trueLength;
                        double newLen = currentLength + delta;
                        //System.out.printf("increase length by %.2f to %.2f\n", delta, newLen);
                        if (newLen <= tType.oneWayRange) {
                            //System.out.printf(" Feasible range %.3f / %.3f\n", newLen, tType.oneWayRange);
                            Itinerary it2 = itnry.deepCopy();


                        double nWD = it2.totalWeightDistance(TheVRC.serialMap);
                        if (0.001 < abs(currentWD - nWD)) {
                            System.out.printf("bad it2\n");
                        }

                            it2.legs.get(0).src.transfer.addInventory(serialName, one);
                            Manifest doManifest = new Manifest();
                            doManifest.addInventory(serialName, one);
                            it2.spliceOneLeg(legNdx, TheVRC.getNodeMap().get(pe.tgtName), dmn, null,
                                    doManifest);
                            double nl2 = it2.getLength();
                            double nWD2 = it2.totalWeightDistance(TheVRC.getSerialMap());
                            boolean wf2 = it2.checkWellFormed();
                            boolean fs2 = it2.checkFeasible(tType, TheVRC.serialMap, TheVRC.getVehicleDomainMap(), TheVRC.getPortAccessMap());
                            // because distances are not symmetric, (nWD2 < currentWD) is actually true sometimes,
                            // especially when nodes are almost on a line
                            if (!wf2 || !fs2 || (0.001 < abs(newLen - nl2)) ) {
                                System.out.printf("bad it2\n");
                            }
                            if (nWD2 < minWD) {
                                splicedIT = it2;
                                minWD = nWD2;
                                //System.out.printf("Reduced WD to %.5e \n", minWD);
                                //System.out.flush();;
                            }
                        } else {
                            //System.out.printf(" Infeasible range %.3f / %.3f\n", newLen, tType.oneWayRange);
                        }
                    } else {
                        //System.out.println("Missing one or both edges");
                    }
                }
            }
            //System.out.flush();
        }
        return splicedIT;
    }

    /**
     * If possible, reorder an itinerary to reduce total ton-miles while keeping the length feasible.
     * This is similar to the furthest-insertion heuristic for TSP.
     * At each step, it tries to insert the furthest node in the most efficient place.
     *
     * For now, we can only process single-pickup itineraries.
     *
     * @return Improved itinerary if possible; NULL otherwise
     */
    public Itinerary reorderCircularItinerary(Itinerary itnry, ReadTransportTypeCSV.DataField vtRec) {
        //System.out.printf("Starting to reorder itinerary %d\n", itnry.getID());
        Itinerary reorderedIT = null;
        int numLegs = itnry.numLegs(); // only 3 or more can be reordered
        boolean ok = (3 <= numLegs);
        ok = ok && itnry.noSelfIntersection();
        ok = ok && itnry.singlePickup();
        ok = ok && itnry.checkCircular();
        if (!ok) {
            return reorderedIT;
        }
        double totalWD = itnry.totalWeightDistance(TheVRC.serialMap);
        VRNode refNode = itnry.legs.get(0).src.node;
        Set<String> nodeNameSet = new HashSet<>();
        for (int i = 1; i < numLegs; i++) { // skip the first 'src' node, which we assume is fixed reference
            nodeNameSet.add(itnry.legs.get(i).src.node.name);
        }
        List<String> nodeNameList = reverse(gcDistSort(nodeNameSet, refNode.name)); // most distant first

        // on trips of 4 or 5 stops, shuffling seems to have no effect.
        // The insertion heuristic seems to provide all the benefit.
        //nodeNameList = shuffle(nodeNameList, TheVRC.prng);

        //System.out.printf("Most distant node first: %s \n", nodeNameList.toString());
        int numNodes = nodeNameList.size();

        // map from node-name to the corresponding drop-off Manifest
        Map<String, Manifest> nodeManifestMap = new HashMap<>();
        for (int i = 0; i < numLegs; i++) {
            nodeManifestMap.put(itnry.legs.get(i).dst.node.name, itnry.legs.get(i).dst.transfer);
        }
        // use the domain of the first leg
        String dmn = itnry.legs.get(0).edge.domain;

        VRNode furthestNode = TheVRC.nodeMap.get(nodeNameList.get(0));
        // initialize with refNode and furthest node
        reorderedIT = new Itinerary(itnry.graph);
        reorderedIT.initializeRoundTrip(refNode, dmn, furthestNode, dmn);
        reorderedIT.legs.get(0).src.transfer = Manifest.add(itnry.legs.get(0).src.transfer, null);
        reorderedIT.legs.get(0).dst.transfer = Manifest.add(nodeManifestMap.get(furthestNode.name), null);
        /*
        System.out.printf("Initialized itinerary %d  %s <--> %s with %d legs and WD %.4E \n",
                reorderedIT.getID(), reorderedIT.legs.get(0).src.node.name, reorderedIT.legs.get(0).dst.node.name,
                reorderedIT.numLegs(), reorderedIT.totalWeightDistance(TheVRC.serialMap));
        System.out.flush();
        */


        for (int n = 1; n < numNodes; n++) {
            String nodeName = nodeNameList.get(n);
            VRNode node = TheVRC.nodeMap.get(nodeName);
            double minWD = Double.MAX_VALUE;
            Itinerary bestItnry = null;
            for (int i = 0; i < reorderedIT.numLegs(); i++) {
                double currentLength = reorderedIT.getLength();

                String srcName = reorderedIT.legs.get(i).src.node.name;
                String dstName = reorderedIT.legs.get(i).dst.node.name;
                VREdge e12 = reorderedIT.legs.get(i).edge;
                //System.out.printf("Considering splice %s into leg %d/%d between %s and %s \n", nodeName, i, reorderedIT.numLegs(), srcName, dstName);
                VREdge e1x = itnry.graph.domainPickEdge(srcName, nodeName, dmn);
                VREdge ex2 = itnry.graph.domainPickEdge(nodeName, dstName, dmn);
                if ((null != e1x) && (null != ex2)) {
                    //System.out.printf("Edges %.2f %.2f vs %.2f, ", e1x.trueLength, ex2.trueLength, e12.trueLength);
                    double delta = e1x.trueLength + ex2.trueLength - e12.trueLength;
                    double newLen = currentLength + delta;
                    //System.out.printf("increase length by %.2f to %.2f\n", delta, newLen);
                    //System.out.flush();
                    if (newLen <= vtRec.oneWayRange) {
                        Itinerary it2 = reorderedIT.deepCopy();
                        Manifest puManifest = null;
                        Manifest doManifest = Manifest.add(nodeManifestMap.get(nodeName), null);
                        it2.spliceOneLeg(i, node, dmn, puManifest, doManifest);
                        double newWD = it2.totalWeightDistance(TheVRC.serialMap);
                        //System.out.printf("Feasible range %.2f, new WD %.4E vs %.4E for %s\n", newLen, newWD, minWD, it2.listLegNodes());
                        //System.out.flush();
                        if (newWD < minWD) {
                            minWD = newWD;
                            bestItnry = it2;
                        }
                    }
                }
            }
            if (null != bestItnry) {
                reorderedIT = bestItnry;
                double newWD = reorderedIT.totalWeightDistance(TheVRC.serialMap);
                //System.out.printf("New legs: %s\n", reorderedIT.listLegNodes());
                //System.out.flush();
            }
        }


        return reorderedIT;
    }

    /**
     * Calculate the total area required for everything in the Manifest.
     * <p>
     * Similar to Manifest.resourceWeight, but for Serial's areas
     *
     * @param m    the manifest
     * @param sMap map from serial-names to the complete serial record, including area and weight
     * @return total area required
     */
    static public double manifestArea(final Manifest m, final Map<String, Serial> sMap) {
        double totalArea = 0.0;
        if ((null != m) && !m.isEmpty()) {
            for (String itemName : m.getItemNames()) {
                double a = sMap.get(itemName).area;
                double c = m.getAvailable(itemName);
                totalArea = totalArea + (a * c);
            }
        }
        return totalArea;
    }

    /**
     * Calculate the total weight of everything in the Manifest
     * <p>
     * Similar to Manifest.resourceWeight, but for Serial's weights
     *
     * @param m    the manifest
     * @param sMap map from serial-names to the complete serial record, including area and weight
     * @return total weight
     */
    static public double manifestWeight(final Manifest m, final Map<String, Serial> sMap) {
        double totalWeight = 0.0;
        if ((null != m) && !m.isEmpty()) {
            for (String itemName : m.getItemNames()) {
                double w = sMap.get(itemName).weight;
                double c = m.getAvailable(itemName);
                totalWeight = totalWeight + (w * c);
            }
        }
        return totalWeight;
    }

}

// =============================================================================
