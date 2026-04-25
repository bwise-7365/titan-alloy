/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.DCVRP;

import java.util.ArrayList;
import java.util.List;

import static groupw.DCVRP.VRController.TheVRC;
import static java.lang.Math.abs;

import static groupw.BaseSim.DSUtils.NAUTICAL_MILE;
import static groupw.BaseSim.DSUtils.greatCircleDistance;
import static groupw.Network.NWUtils.Tuple2;

import groupw.DCVRP.VRGraph.VRNode;
import groupw.DCVRP.VRGraph.VREdge;
import groupw.DCVRP.Backlog.Reservation;

/**
 * Class to hold methods which "decide" how to manipulate Serial objects.
 *
 * Basic operations that do not require decisions go in the Serial class.
 * For example, deciding which backlog to join goes in this SerialController class.
 * @author BenWise
 */
public class SerialController implements CountedItem {

    public Serial s = null;  // thus visible to MAST, z.b.
    ItineraryBuilder iBuilder = null;

    public SerialController(Serial s, ItineraryBuilder iB) {
        this.s = s;
        s.controller = this;
        this.iBuilder = iB;
        this.idNum = ItemCounter.makeID();
    }

    /**
     * If not backlogged or scheduled, scan transports and decide which transport's backlog to join, but not actually join yet.
     *
     * Counts this serial's roundtrip time as to/from the transport's
     * homebase node to this serial's final estimation node.
     *
     * @param transports List of names of potential transports, not necessarily at the same home base
     * @return which (if any) transport was chosen and the corresponding Reservation
     */
    public Tuple2<Transport, Reservation> selectBacklog(List<Transport> transports, double currTime, boolean useMinTime) {
        //System.out.println("");
        //System.out.printf("Serial %s, %d choosing transport backlog\n", s.name, s.getID());

        // These print messages are necessary because of MAST
        if (null != s.currBacklog) {
            System.out.printf("Serial %s, %d NOT choosing transport backlog: already in a backlog of vehicle %s \n",
                    s.name, s.getID(), s.currBacklog.transportName);
            return null;
        }
        if (s.inItineraryP) {
            System.out.printf("Serial %s, %d NOT choosing transport backlog: already in an itinerary \n",
                    s.name, s.getID());
            return null;
        }

        Tuple2<Transport, Reservation> chosen = null;
        double minTime = Double.MAX_VALUE;
        double deltaT = 0.10; // any time within a 1/10 th of an hour is considered the same time
        List<Tuple2<Transport, Reservation>> phase1 = new ArrayList<>();
        // Make sure these two are initialized before scanning hundreds of vehicles
        TheVRC.getNodeMap();
        TheVRC.getVehicleDataMap();
        for (Transport t : transports) {
            ReadTransportVehicleCSV.DataField vRec = TheVRC.vehicleDataMap.get(t.name);
            if ((s.area <= t.cargoArea) && (s.weight <= t.cargoWeight)) {
                double tSpeed = TheVRC.vehicleTypeMap.get(t.type).cruiseSpd; // knots, or NM/hour
                VRNode tNode = TheVRC.nodeMap.get(vRec.homeBase);
                VRNode dNode = TheVRC.nodeMap.get(s.deliveryNodeName);

                // These have the most promising ones (if any) first
                List<VREdge> pEdges = iBuilder.potentialIntermediateEdges(t.name, s.name, currTime);
                if (0 < pEdges.size()) {
                    VRNode intNode = TheVRC.nodeMap.get(pEdges.get(0).tgtName);
                    double estTransitHours = 0.0;
                    if (useMinTime) {
                        estTransitHours = iBuilder.estMinTransitTime(s, intNode, dNode);
                    } else {
                        estTransitHours = iBuilder.estAverageTransitTime(s, intNode, dNode);
                    }

                    double stDist = greatCircleDistance(tNode.latitude, tNode.longitude, dNode.latitude, dNode.longitude) / NAUTICAL_MILE;
                    double rtt = (2.0 * stDist) / tSpeed;

                    Reservation r = new Reservation( t.name, s.name,
                            s.deliveryNodeName, intNode.name,
                            s.area, s.weight, rtt);

                    double hat = t.backlog.hypoArrivalTime(t.name, s.area, s.weight, rtt)+ estTransitHours;

                    if (hat < minTime - deltaT) { // especially when minTime == MAX_VALUE
                        phase1 = new ArrayList<>();
                        phase1.add( new Tuple2<>(t, r) );
                        minTime = hat;
                        //System.out.printf("Reset phase1 to time %.3f for %s \n", hat, t.name);
                    } else if (abs(minTime - hat) <= deltaT) {
                        phase1.add( new Tuple2<>(t, r) );
                        //System.out.printf("Added phase1 with time %.3f for %s \n", hat, t.name);
                    } else {
                        //System.out.printf("Not added with time %.3f for %s \n", hat, t.name);
                    }
                }
                else {
                    //System.out.printf("Serial %s has no potential edges on %s from %s\n", s.name, t.name, vRec.homeBase);
                    //System.out.flush();
                }
            }
            else {
                //System.out.printf("Serial %s does not fit on %s\n", s.name, t.name);
                //System.out.flush();
            }
        }

        //System.out.printf("Serial %s has %d potential phase1 transports\n", s.name, phase1.size());
        //System.out.flush();
        if (0 == phase1.size()) {
            return null;
        }
        else if (1 == phase1.size()) {
            return phase1.get(0);
        }

        // At this point, there are at least two potential transports with (approx) the same arrival time.
        // Pick the most heavily-backlogged one.
        double loadMax = 0.0;
        double deltaL = 0.001;
        List<Tuple2<Transport, Reservation>> phase2 = new ArrayList<>();
        for (Tuple2<Transport, Reservation> pr : phase1) {
            Transport t = pr.get0();
            int n = t.backlog.numReservations()-1;
            double ta = (s.area + t.backlog.totalArea(n))/ t.cargoArea;
            double tw = (s.weight + t.backlog.totalWeight(n)) / t.cargoWeight;
            double loadFactor = (ta > tw) ? ta : tw;
            if (loadMax + deltaL < loadFactor) { // especially when loadMax == 0
                phase2 = new ArrayList<>();
                phase2.add(pr);
                loadMax = loadFactor;
                //System.out.printf("Reset phase2 to %.3f with %s on %s\n", loadFactor, s.name, t.name);
                //System.out.flush();
            }
            else if (abs(loadMax - loadFactor) <= deltaL) {
                phase2.add(pr);
                //System.out.printf("Added phase2 to %.3f with %s on %s\n", loadFactor, s.name, t.name);
                //System.out.flush();
            }
        }


        //System.out.printf("Serial %s has %d potential phase2 transports\n", s.name, phase2.size());
        //System.out.flush();
        if (1 == phase2.size()) {
            chosen = phase2.get(0);
        }
        else {
            int i = TheVRC.prng.nextInt(phase2.size());
            chosen = phase2.get(i);
        }

        return chosen;
    }

    /**
     * The numerical ID is mostly for identifying almost-anonymous
     * data structures during debugging. See the comments on CountedItem class.
     */
    public long getID() {
        return idNum;
    }

    private final long idNum;


}


// =============================================================================