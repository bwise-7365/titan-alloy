/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Logistics.LogDistNW.LogDistNode;

import java.util.HashMap;
import java.util.Map;
import java.util.Set;
import org.jgrapht.alg.cycle.CycleDetector;
import org.jgrapht.graph.DefaultEdge;
import org.jgrapht.graph.SimpleDirectedGraph;

import static groupw.Logistics.LogSprtNW.AllocationPolicy.Even; // odd but necessary

/**
 * The support network describes the assigned responsibilities
 * of which unit will support which (similar to fire support assignments).
 * It describes the support roles and which units are currently assigned
 * to which role (which can and will change over time).
 * @author BenWise
 */
public class LogSprtNW {

    public LogSprtNW() {
        // nothing yet
    }

    /**
     *  Policy options for rollup of supply status information from supported to supporting
     *  nodes. Each Support node has its own policy. The 'Even' default method allocates them evenly.
     *  Better versions would weight suppliers based on which suppliers
     *  had direct, primary logistical support responsibility
     *  (vs indirect, secondary), had better physical capacity,
     *  had more stuff on hand, and so on.
     */
    public enum AllocationPolicy {
        Even,  // split resupply responsibility evenly over suppliers
        Primary, // higher weight to assigned primary (not implemented)
        Capacity // higher weight to suppliers with more storage/throughput capacity (not implemented)
    };

    /**
     * The SupportNode is the thinking / planning / guessing
     * part of the logistical system. It is a role in the organization
     * which can be filled by different units at different times,
     * just as the "supporting artillery" role can be assigned to different units at different
     * times, even if the supported unit stays the same.
     * The SN 'owns' vehicles and supplies and decides (simplistic or complex) what
     * to do with them.
     */
    static public class SupportNode {

        public SupportNode() {
            myID = makeID();
        }

        public int getID() {
            return myID;
        }

        // It is pointless and verbose to make a data member private,
        // but then provide trivial getter/setter that allow full access.
        // public String getUnitName() { return unitName; }
        // public void setUnitName(String n) { unitName = n; }
        // public Map<String, Integer> getVehicles() { return vehicles; }
        // public void setVehicles(Map<String, Integer> vehicles) { this.vehicles = vehicles;}

        /**
         * @return Get the current location on the (physical) distribution network
         */
        public LogDistNode getLogDistNode() {
            return logDistNode;
        }

        /**
         * Set the current location on the (physical) distribution network,
         * and update the name of the current location in order to keep them in sync.
         *
         * @param logDistNode the logDistNode to set
         */
        public void setLogDistNode(LogDistNode logDistNode) {
            this.logDistNode = logDistNode;
            setLogDistNodeName(logDistNode.name); // ensure consistency
        }

        /**
         * Get the name of the current location on the (physical) distribution network
         *
         * @return the logDistNodeName
         */
        public String getLogDistNodeName() {
            return logDistNodeName;
        }

        /**
         * Set the name of the current location,
         * completely separate from the current location on the (physical) distribution network.
         * The parser requires that the name must be set before the node is known,
         * so this duplicative parameter cannot be eliminated: you must keep them in sync.
         *
         * @param logDistNodeName the logDistNodeName to set
         */
        public void setLogDistNodeName(String logDistNodeName) {
            this.logDistNodeName = logDistNodeName;
        }

        /**
         * Get the name of the dispatcher controlling this unit.
         *
         * @return the name of the dispatcher
         */
        public String getDispatcherName() {
            return dispatcherName;
        }

        /**
         * Set the name of the dispatcher controlling this unit.
         *
         * @param dispatcherName the name of the dispatcher
         */
        public void setDispatcherName(String dispatcherName) {
            this.dispatcherName = dispatcherName;
        }

        /**
         * Primary use is to identify objects while debugging
         */
        private int myID;
        public String unitName = "no name";
        /**
         * Vehicles is a set of (vehicleName, vehicleQuantity) Tuple2,
         * never Tuple3 or above. Note that it changes with receipt of
         * or attrition to vehicles.
         */
        public Map<String, Integer> vehicles = new HashMap<>(); // no fractional vehicles
        private LogDistNode logDistNode = null; // changes with every move
        private String logDistNodeName = ""; // changes with every move
        private String dispatcherName = "";

        public AllocationPolicy allocationPolicy = Even; // reasonable default

        /**
         * Summed over this Unit and all supported SupportNodes,
         * reorder level, maximum desired level, current level, and consumption rate (per 24 hours)
         */
        public SupplyStatus sStatus = null;


        /**
         * Rollup supply status information from supported to supporting
         * nodes, depending on which allocationPolicy was set for this node.
         *
         * @param lsnw
         */
        public void rollUp(LogSprtNW lsnw, Map<String, Unit> unitMap) {
            switch (allocationPolicy) {
                case Even: // deliberate fall through
                default:
                    evenRollUp(lsnw, unitMap);
                    break;
            }
        }

        /**
         * Rollup supply status information from supported to this supporting node.
         * The default method is to allocate them evenly.
         *
         * @param lsnw
         */
        public void evenRollUp(LogSprtNW lsnw, Map<String, Unit> unitMap) {
            System.out.printf("\nStarting even rollup for SupportNode %s \n", unitName);
            if (null == sStatus) {
                Unit u = unitMap.get(unitName);
                System.out.printf("Creating new SupplyStatus for SupportNode %s from Unit %s\n",
                        unitName, u.getName());
                sStatus = SupplyStatus.copy(u.sStatus);
                System.out.printf("SupportNode for Unit %s initially has current levels: \n", unitName);
                System.out.printf("  %s \n", sStatus.currentLevel.toString());
            }
            Set<SupportArc> supportArcs = lsnw.lsGraph.outgoingEdgesOf(this);
            for (SupportArc sa : supportArcs) {
                SupportNode sn2 = lsnw.lsGraph.getEdgeTarget(sa);
                if (null == sn2.sStatus) {
                    sn2.sStatus = new SupplyStatus(); // all zeros
                    System.out.printf("Created new SupplyStatus for subordinate SupportNode %s \n",
                            sn2.unitName);
                }
                int numIn = lsnw.lsGraph.inDegreeOf(sn2);
                //Manifest m = sn2.sStatus.currentLevel.makeScaled(1.0 / numIn);
                //sStatus.currentLevel = Manifest.add(m, sStatus.currentLevel);
                System.out.printf("Subordinate SupportNode %s has %d parents (including SupportNode for %s)\n",
                        sn2.unitName, numIn, unitName);
                SupplyStatus s2 = sn2.sStatus.makeScaled(1.0 / numIn);
                sStatus = sStatus.add(s2);
            } // end of loop over arcs to subordinate SupportNodes
            System.out.printf("SupportNode for Unit %s now has current levels: \n", unitName);
            System.out.printf("  %s \n", sStatus.currentLevel.toString());
            System.out.flush();
        }
    } // end of class SupportNode

    /**
     * The SupportArc describes who supports whom. It is an organizational
     * relationship: who is currently assigned to provide logistical
     * support to whom. The units at each end can change over time, depending
     * on who is assigned to support whom when.
     * It points from supplier to supplied.
     */
    static public class SupportArc extends DefaultEdge {

        public String name = "";
        public String srcLogDistNode = "";
        public String tgtLogDistNode = "";

        // Just in case some SWIFT code does serialize this
        private static final long serialVersionUID = 1000;

        /**
         * Construct a new "supports" arc.
         * Notice that the constructor must take no arguments
         * because jGraphT sets the source and target.
         */
        public SupportArc() {
            super();
            myID = makeID();
        }

        /**
         * Get the source end of this link, i.e. which supporting node sends out supplies
         *
         * @return
         */
        public SupportNode getSrc() {
            return (SupportNode) getSource();
        }

        /**
         * Get the target end of this link, i.e. which supported node receives supplies
         *
         * @return
         */
        public SupportNode getTgt() {
            return (SupportNode) getTarget();
        }

        public int getID() {
            return myID;
        }

        /**
         * Primary use is to identify objects while debugging
         */
        private int myID;
        // end of class SupportArc
    }

    /**
     * Check if this Log-Support Network has cycles (it should not).
     *
     * @return True if any cycle is present
     */
    public boolean cycleCheck() {
        CycleDetector<SupportNode, SupportArc> cd = new CycleDetector<>(lsGraph);
        return cd.detectCycles();
    }

    /**
     * The underlying JGraphT representation of a directed, unweighted graph.
     * Cycles (but not self-loops) are allowed by jGraphT;
     * this code will have to enforce DAG-ness.
     * Notice that this is cannot be a multigraph / pseudograph because
     * multiple "supports / supported-by" links between the same two
     * nodes would not make sense.
     */
    public SimpleDirectedGraph<SupportNode, SupportArc> lsGraph = null;

    public void setGraph(SimpleDirectedGraph<SupportNode, SupportArc> g) {
        lsGraph = g;
    }

    public Set<SupportNode> getNodes() {
        return lsGraph.vertexSet();
    }

    /**
     * For repeated testing, it is often desirable to reset the static
     * id counter back to some standard value.
     * This avoids enormous ID numbers.
     *
     * @param id
     */
    static public void resetHighestID(int id) {
        highestID = id;
    }

    static public int makeID() {
        return highestID++;
    }

    static private int highestID = 1000;
}


// =============================================================================
