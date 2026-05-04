// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import java.util.*;

import groupw.DCVRP.Backlog.Reservation;
import groupw.Network.NWUtils;
import groupw.Network.NWUtils.Tuple2;
import groupw.DCVRP.VRGraph.VRNode;
import groupw.DCVRP.VRGraph.VREdge;

import static groupw.BaseSim.DSUtils.NAUTICAL_MILE;
import static groupw.BaseSim.DSUtils.greatCircleDistance;
import static groupw.Network.NWUtils.ReportingLevel.Silent;
import static groupw.Network.NWUtils.iMod;
import static groupw.Network.NWUtils.makePRNG;
import static groupw.Network.NWUtils.shuffle;

/**
 * Overall controller of the decentralized vehicle routing (DCVRP) architecture.
 *
 * Because the architecture is decentralized,
 * a VRController exercises only loose control
 * by specifying priorities, due dates, delivery locations, etc.
 * It provides a single copy of common information like all Serials,
 * all Transports, the network and similar.
 *
 * @author BenWise
 */
public class VRController {

    /**
     * Public method to initialize singleton from a ScenarioRecord and PRNG seed
     * @param sr
     * @param seed
     * @return
     */
    public static VRController initialize(ReadDCVRScenarioCSV.ScenarioRecord sr, int seed) {
        clear();
        TheVRC = new VRController(sr, seed);
        TheVRC.resetMaps();
        return TheVRC;
    }

    /**
     * Rarely, it makes sense to clear and reset the VRController singleton
     */
    public static void clear() {
        if (null != TheVRC){
            TheVRC.resetMaps();
        }
        TheVRC = null;
    }

    /**
     * Private constructor from ScenarioRecord to help enforce singleton
     * @param sr
     * @param seed
     */
    private VRController(ReadDCVRScenarioCSV.ScenarioRecord sr, int seed) {
        this.scenRecord = sr;
        final boolean verboseP = true;
        prng = makePRNG(seed, verboseP);
    }

    /**
     * Private constructor of empty VRController to help enforce singleton
     */
    private VRController() {
        this.scenRecord = null;
        final boolean verboseP = true;
        prng = null;
    }

    private void resetMaps() {
        nodeMap = null;
        edgeMap = null;
        unitMap = null;
        vehicleTypeMap = null;
        vehicleDataMap = null;
        portAccessMap = null;
        serialMap = null;
        vehicleMap = null;
        serialRecordMap = null;
        vehicleDomainMap = null; // map from vehicle-type names to Set of domain-names
    }

    public static VRController TheVRC = null;

    /**
     * Level of reporting internal operations, from Silent, Low, ... High, Debugging.
     */
    NWUtils.ReportingLevel rLevel = Silent;

    /**
     * Given Map of vehicle name to Backlog objects, assign those backlogs to vehicle objects
     *
     * @param vehicleBacklogMap
     */
    public void assignBacklogs(Map<String, Backlog> vehicleBacklogMap){
        for (Map.Entry<String, Backlog> e : vehicleBacklogMap.entrySet()) {
            String vName = e.getKey();
            Transport v = getVehicleMap().get(vName);
            v.backlog = e.getValue();
        }
    }

    public Map<String, Backlog> matchSerialsToBacklogs(List<String> serialNames, List<String> transportNames,
                                                       ItineraryBuilder iBuilder, double currTime, boolean useMinTime) {
        int numVehicles = transportNames.size();
        getSerialMap(); // initialize it
        getVehicleMap(); // initialize it
        Map<String, Backlog> vehicleBacklogMap = new HashMap<>(numVehicles);
        List<Transport> transports = new ArrayList<>(numVehicles);
        for (String tn : transportNames) { // these are probably in random order
            vehicleBacklogMap.put(tn, new Backlog(tn));
            transports.add(vehicleMap.get(tn));
        }

        for (String sn : serialNames) { // these are probably in random order
            Serial s = serialMap.get(sn);
            SerialController sc = s.controller;
            if (null == sc) {
                sc = new SerialController(s);
            }
            Tuple2<Transport, Reservation> pr =  sc.selectBacklog(transports, currTime, useMinTime);
            if (null != pr) {
                Transport t = pr.get0();
                //System.out.printf("Serial %22s chose to join backlog of transport %14s \n", s.name, t.name);

                // put the reservation in the backlog
                // and mark the serial as being in a backlog
                t.backlog.appendReservation(pr.get1());
                s.currBacklog = t.backlog;

                vehicleBacklogMap.put(t.name, t.backlog);
                //System.out.printf("  Transport %14s now had %3d reservations for %2d trips\n", t.name, t.backlog.numReservations(), t.backlog.numTrips());
                //System.out.flush();
            }
            else {
                //System.out.printf("Serial %22s did not find any suitable transport \n", s.name);
                //System.out.flush();
            }
        }

        return vehicleBacklogMap;
    }

    /**
     * Randomly create a Map from vehicle names to their backlogs.
     *
     * Given a node name, find all the transport vehicles whose home base is that node,
     * then find all the serials currently at that node and randomly
     * assign them to the vehicles' backlogs.
     *
     * @param nodeName home base of transport vehicles
     * @return
     */
    public Map<String, Backlog> makeRandomBacklogs(String nodeName) {
        VRNode homeBaseNode = getNodeMap().get(nodeName);
        if (null == homeBaseNode) {
            return null; // not just devoid of vehicles but undefined
        }

        boolean randomTransportOrder = true;
        boolean randomSerialOrder = true;
        // Find all transport vehicles based at this node and start backlogs for each
        Map<String, Backlog> vehicleBacklogMap = new HashMap<>(1);
        List<String> transportNames = transportsAtHomeBase(nodeName, randomTransportOrder);
        for (String tn : transportNames) {
            vehicleBacklogMap.put(tn, new Backlog(tn));
        }
        int numVehicles = transportNames.size();
        getVehicleTypeMap(); // initialize it
        getVehicleDataMap();
        double srcLat = nodeMap.get(nodeName).latitude;
        double srcLon = nodeMap.get(nodeName).longitude;

        List<String> serialNames = serialsAtNode(nodeName, randomSerialOrder);

        List<String> unassignedSerials = new ArrayList<>();
        for (String sn : serialNames) {
            Serial srl = serialMap.get(sn);
            if (nodeName.equals(srl.currentNodeName)) {
                int startNdx = TheVRC.prng.nextInt(numVehicles);
                boolean unassigned = true;
                for (int i = 0; unassigned && (i < numVehicles); i++) {
                    // This is not a uniform selection, but it will do for now.
                    int vNdx = iMod(startNdx + i, numVehicles);
                    String vName = transportNames.get(vNdx);
                    ReadTransportVehicleCSV.DataField vehicle = vehicleDataMap.get(vName);
                    ReadTransportTypeCSV.DataField tType = vehicleTypeMap.get(vehicle.type);
                    if ((srl.area <= tType.cargoArea) && (srl.weight <= tType.cargoWeight)) {

                        // we use great circle distance because we only want relative travel times.
                        // I happen to know that cruise speed is in knots, so we measure
                        // distance in nautical miles so time will be in hours.
                        double tgtLat = nodeMap.get(srl.deliveryNodeName).latitude;
                        double tgtLon = nodeMap.get(srl.deliveryNodeName).longitude;
                        double dist = greatCircleDistance(srcLat, srcLon, tgtLat, tgtLon) / NAUTICAL_MILE;
                        double roundTripTime = (2.0 * dist) / tType.cruiseSpd;
                        Reservation r = new Reservation(
                                vName, srl.name, srl.deliveryNodeName, srl.deliveryNodeName,
                                srl.area, srl.weight, roundTripTime);
                        vehicleBacklogMap.get(vName).appendReservation(r);
                        unassigned = false;
                        //System.out.printf("Serial %s added to backlog of %s at %s\n", srl.name, vName, nodeName);
                    }
                }
                if (unassigned) {
                    //System.out.printf("Serial %s did not fit on any vehicle based at %s\n", srl.name, nodeName);
                    unassignedSerials.add(srl.name);
                }
            }
        }

        /*
        for (Map.Entry<String, Backlog> entry : vehicleBacklogMap.entrySet()) {
            System.out.printf("Vehicle %s has backlog of %d serials \n",
                    entry.getKey(), entry.getValue().numReservations());
        }
        System.out.printf("There were %d unassigned serials", unassignedSerials.size());
        for (String sn : unassignedSerials) {
            System.out.printf(" %s", sn);
        }
        System.out.println("");
        */

        return vehicleBacklogMap;
    }

    /**
     * Get the list of live transports whose home base is the provided node.
     *
     * Optionally, shuffle them.
     *
     * @param nodeName name of homebase node at which to search
     * @param randomOrder if True, randomize order of serials
     * @return List of transport-names
     */
    public List<String> transportsAtHomeBase(String nodeName, boolean randomOrder) {
        VRNode homeBaseNode = getNodeMap().get(nodeName);
        if (null == homeBaseNode) {
            return null; // not just devoid of vehicles but undefined
        }
        List<String> vehicleNames = new ArrayList<>(1);
        getVehicleMap(); // make sure it is initialized before scanning hundreds
        for (Map.Entry<String, ReadTransportVehicleCSV.DataField> entry : getVehicleDataMap().entrySet()) {
            String hbName = entry.getValue().homeBase;
            String vName = entry.getValue().name;
            if (nodeName.equals(hbName)) {
                Transport vehicle = vehicleMap.get(vName);
                if (vehicle.aliveP) {
                    vehicleNames.add(vName);
                }
            }
        }
        if (randomOrder) {
            vehicleNames = shuffle(vehicleNames, prng);
        }
        return vehicleNames;
    }

    /**
     * Get the list of transport names which are at the given node at this moment.
     *
     * @param nodeName name of the node at which to search
     * @return List of transports at the specified node
     */
    public List<Transport> transportsAtNode(String nodeName) {
        List<Transport> transports = new ArrayList<>(1);
        getVehicleMap(); // make sure it is initialized before scanning hundreds
        for (Map.Entry<String, ReadTransportVehicleCSV.DataField> entry : vehicleDataMap.entrySet()) {
            String vName = entry.getValue().name;
            Transport vehicle = vehicleMap.get(vName);
            if (vehicle.aliveP && (vehicle.currentNodeName != null)) { // in-transit, no node assigned
                if (nodeName.equalsIgnoreCase(vehicle.currentNodeName)) {
                    transports.add(vehicle);
                }
            }
        }
        return transports;
    }

    /**
     * A node is a dead end if it is NOT the destination and is NOT the home base of a transport that can carry this serial
     *
     * TODO: cache this data, rather than recomputing constantly
     *
     * @param nodeName
     * @param randomOrder
     * @return
     */
    public boolean isDeadEnd(VRNode node, Serial serial) {
        boolean deadEnd = (!node.name.equals(serial.deliveryNodeName)); // destination is never a dead end.
        if (deadEnd) {
            TheVRC.getVehicleDataMap();
            TheVRC.getVehicleTypeMap();
            for (Map.Entry<String, ReadTransportVehicleCSV.DataField> entry : TheVRC.vehicleDataMap.entrySet()) {
                if (entry.getValue().homeBase.equalsIgnoreCase(node.name)) {
                    String vType = entry.getValue().type;
                    ReadTransportTypeCSV.DataField vData = TheVRC.getVehicleTypeMap().get(vType);
                    if ((serial.area <= vData.cargoArea) && (serial.weight <= vData.cargoWeight)) {
                        deadEnd = false;
                    }
                }
            }
        }
        return deadEnd;
    }


    /**
     * Get the list of serials which are at the given node at this moment.
     *
     * Optionally, shuffle them.
     *
     * @param nodeName name of the node at which to search
     * @param randomOrder if True, randomize order of serials
     * @return List of serial-names
     */
    public List<String> serialsAtNode(String nodeName, boolean randomOrder) {
        VRNode referenceNode = getNodeMap().get(nodeName);
        if (null == referenceNode) {
            return null; // not just devoid of serials but undefined
        }
        List<String> serialNames = new ArrayList<>();
        for (Map.Entry<String, Serial> entry : getSerialMap().entrySet()) {
            Serial srl = entry.getValue();
            if (nodeName.equals(srl.currentNodeName)) {
                serialNames.add(srl.name);
            }
        }
        if (randomOrder) {
            serialNames = shuffle(serialNames, prng);
        }
        return serialNames;
    }

    // access functions to build / retrieve various useful Maps
    public Map<String, VRNode> getNodeMap() {
        if (null == nodeMap) {
            nodeMap = scenRecord.vrg.makeNodeMap();
        }
        return nodeMap;
    }

    public Map<String, VREdge> getEdgeMap() {
        if (null == edgeMap) {
            edgeMap = scenRecord.vrg.makeEdgeMap();
        }
        return edgeMap;
    }

    public Map<String, ReadUnitCSV.DataField> getUnitMap() {
        if (null == unitMap) {
            unitMap = ReadUnitCSV.makeUnitMap(scenRecord.unitRecords);
        }
        return unitMap;
    }

    public Map<String, ReadTransportTypeCSV.DataField> getVehicleTypeMap() {
        if (null == vehicleTypeMap) {
            vehicleTypeMap = ReadTransportTypeCSV.makeVTypeMap(scenRecord.vtRecords);
        }
        return vehicleTypeMap;
    }

    public Map<String, ReadTransportVehicleCSV.DataField> getVehicleDataMap() {
        if (null == vehicleDataMap) {
            vehicleDataMap = ReadTransportVehicleCSV.makeVehicleDataMap(scenRecord.vRecords);
        }
        return vehicleDataMap;
    }

    public Map<String, Set<String>> getPortAccessMap() {
        if (null == portAccessMap) {
            portAccessMap = ReadPortAccessCSV.makePortAccessMap(scenRecord.paRecords);
        }
        return portAccessMap;
    }

    public Map<String, Serial> getSerialMap() {
        if (null == serialMap) {
            serialMap = ReadSerialCSV.makeSerialMap(scenRecord.sRecords, getUnitMap());
        }
        return serialMap;
    }

    public Map<String, Transport> getVehicleMap() {
        if (null == vehicleMap) {
            vehicleMap = ReadTransportVehicleCSV.makeVehicleMap(
                    getVehicleDataMap(),
                    getVehicleTypeMap(),
                    this);
        }
        return vehicleMap;
    }

    public Map<String, ReadSerialCSV.DataField> getSerialRecordMap() {
        if (null == serialRecordMap) {
            serialRecordMap = new HashMap<String, ReadSerialCSV.DataField>(scenRecord.sRecords.size());
            for (ReadSerialCSV.DataField sr : scenRecord.sRecords) {
                serialRecordMap.put(sr.name, sr);
            }
        }
        return serialRecordMap;
    }

    public Map<String, Set<String>> getVehicleDomainMap() {
        if (null == vehicleDomainMap) {
            vehicleDomainMap = ReadTransportDomainCSV.makeTransportDomainMap(scenRecord.tdRecords);
        }
        return vehicleDomainMap;
    }

    /**
     * Change the home base of a vehicle.
     *
     * A vehicle might make one long move from its pre-positioning port,
     * then start cycling from and to a second port.
     *
     * @param vehicleName name of vehicle to be reassigned
     * @param nodeName name of node to which it will be assigned
     */
    public void resetHomeBase(String vehicleName, String nodeName) {
        scenRecord.resetHomeBase(vehicleName, nodeName);
        vehicleDataMap = null; // wipe out old one
        getVehicleDataMap();
    }

    /**
     * Change which kinds of vehicles can use a port because of destruction or repair.
     *
     * Notice that we change the whole set of types, not just add or subtract one type.
     *
     * @param portName name of port to be modified
     * @param vehicleTypes Set of vehicles to be handled
     */
    public void resetPortAccess(String portName, Set<String> vehicleTypes) {
        scenRecord.resetPortAccess(portName, vehicleTypes);
        portAccessMap = null;
        getPortAccessMap();
    }

    // Various useful Maps to speed data access
    // All are package-private.
    ReadDCVRScenarioCSV.ScenarioRecord scenRecord = null;
    Map<String, VRNode> nodeMap = null;
    Map<String, VREdge> edgeMap = null;
    Map<String, ReadUnitCSV.DataField> unitMap = null; // map from unit-names to unit data records
    Map<String, ReadTransportTypeCSV.DataField> vehicleTypeMap = null; // map from vehicle-type names to vehicle-type data records
    Map<String, ReadTransportVehicleCSV.DataField> vehicleDataMap = null; // map from vehicle names to vehicle data records
    Map<String, Set<String>> portAccessMap = null; // map from port-name to set of vehicle-type names
    Map<String, Serial> serialMap = null; // map from serial-names to Serial objects
    Map<String, Transport> vehicleMap = null; // map from transport-names to Transport objects
    Map<String, ReadSerialCSV.DataField> serialRecordMap = null; // map from serial-names to serial data records
    Map<String, Set<String>> vehicleDomainMap = null; // map from vehicle-type names to Set of domain-names

    // Only the VRC should access its internal PRNG.
    // In production, this should be from MAST, AFSIM, or whatever.
    public Random prng;

}

// =============================================================================
