/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Network.NWUtils.Tuple;
import static java.lang.Math.sqrt;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

import org.apache.commons.math4.legacy.linear.RealVector;
import org.jgrapht.graph.DefaultEdge;
import org.jgrapht.graph.Pseudograph;

/**
 * This class represents the arcs and vertices of an actual physical network
 * around which vehicles and supplies can move. It is NOT intended to be used
 * solely by LogSprtNW, so it CANNOT make any reference to or rely in any way upon
 * LogSprtNW, SupportNode or SupportArc. In particular, the LogDistNW is intended
 * to also support a completely decentralized architecture for opportunistic routing
 * of vehicles and "packages", without any centralized or even semi-centralized control.
 * @author BenWise
 */
public class LogDistNW {

    public LogDistNW() {
        // nothing yet
    }

    /**
     * The domains of a node are all those of its
     * incoming or outgoing edges
     *
     * @param ldn the LogDistNode to have its domains set
     */
    public void setDomains(LogDistNode ldn) {
        Set<LogDistArc> inArcs = ldGraph.incomingEdgesOf(ldn);

        Set<String> ds = new HashSet<>(1);
        for (LogDistArc a : inArcs) {
            ds.add(a.domain);
        }

        Set<LogDistArc> outArcs = ldGraph.outgoingEdgesOf(ldn);
        for (LogDistArc a : outArcs) {
            ds.add(a.domain);
        }
        ldn.domains = ds;
    }

    /**
     * An actual, physical node in the distribution network:
     * road intersection, railroad yard, port, etc.
     */
    static public class LogDistNode {

        public LogDistNode() {
            name = "unnamed";
            coords = null;
            domains = null;//Does not have domain
            myID = makeID();
        }
        public LogDistNode(String name){
           this.name = name; 
           myID = makeID();
        }
        public LogDistNode(RealVector cs) {
            name = "unnamed";
            coords = cs;
            domains = null;//Does not have domain
            myID = makeID();
        }

        public double eucDist(LogDistNode g2) {
            return eucDist(g2.coords);
        }

        /**
         * Return straight-line Euclidean distance, regardless of global curvature
         * @param cs real vector to which we desire the distance
         * @return distance from this real vector to 'cs'
         */
        public double eucDist(RealVector cs) {
            double d0 = coords.getEntry(0) - cs.getEntry(0);
            double d1 = coords.getEntry(1) - cs.getEntry(1);
            double d2 = coords.getEntry(2) - cs.getEntry(2);
            return sqrt((d0 * d0) + (d1 * d1) + (d2 * d2));
        }

        public boolean open = true; // a reasonable default
        public String name;
        public RealVector coords;
    
        public Set<String> domains = null;

        /** maximum on-site / organic storage capability by supply type
         * null Manifest means no limits
         */
        public Manifest storeMax = null; // maximum storage capacity, tons if non-NULL

        /** maximum daily throughput, which reflects material-handling capability
         * null Manifest means no limits
         */
        public Manifest throughDaily = null; // maximum daily input/output, tons/day if non-NULL

        public int getID() {
            return myID;
        }

        /**
         * Primary use is to identify objects while debugging
         */
        private int myID;

    // end of class LogDistNode
    }

    /**
     * An actual physical link in the distribution network:
     * road, railroad, air corridor, sea lane, etc.
     */
    static public class LogDistArc extends DefaultEdge {

        // Just in case some SWIFT code does serialize this
        private static final long serialVersionUID = 1000;

        /**
         * Construct a new log-distribution arc.
         * Notice that the constructor must take no arguments
         * because jGraphT sets the source and target.
         */
        public LogDistArc() {
            super();
            myID = makeID();
        }
        
        /**
         * Get the 'source', even though it is an undirected edge
         *
         * @return one of the arcs endpoints
         */
        public LogDistNode getSrc() {
            return (LogDistNode) getSource();
        }

        /**
         * Get the 'target', even though it is an undirected edge
         *
         * @return one of the arcs endpoints
         */
        public LogDistNode getTgt() {
            return (LogDistNode) getTarget();
        }

        public String name;
        public String srcNodeName = "";
        public String tgtNodNamee = "";
        public boolean open = true; // a reasonable default

        public String domain;  // domain of the arc as defined in the domains csv file.
        public double maxSpeed = 0.0; // primarily for roads, kilometers / hour, sometimes Double.MAX_VALUE
        public double trueLength = 0.0;   // kilometers
        public double throughDaily = 0.0; // primarily for roads, throughput limit in tons/day, sometimes Double.MAX_VALUE
        public double loadMax = 0.0; // primarily for roads and bridges, load size limit in tons, sometimes Double.MAX_VALUE
        public List<Tuple> intermediateCoordinates = null;
        public int getID() {
            return myID;
        }

        /**
         * Primary use is to identify objects while debugging
         */
        private int myID;

        /**
         * List of intermediate points, if any, between src and tgt (in that order).
         * Used only for drawing the path.
         */
        public List<RealVector> intPoints;

        // end of class LogDistArc
    }

    /**
     * The underlying JGraphT representation of an undirected, unweighted graph.
     * Multiple edges between a vertex-pair and self-loop are allowed because
     * there might easily be several roads between the same two locations
     */
    public Pseudograph<LogDistNode, LogDistArc> ldGraph = null;
    public void setGraph(Pseudograph<LogDistNode, LogDistArc> g) {
        ldGraph = g;
    }
    
    public Set<LogDistNode> getNodes() {
        return ldGraph.vertexSet();
    }


    /**
     * For repeated testing, it is often desirable to reset the static
     * id counter back to some standard value.
     * This avoids enormous ID numbers.
     *
     * @param id new ID number
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
