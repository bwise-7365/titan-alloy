// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import org.jgrapht.graph.DefaultWeightedEdge;
import org.jgrapht.graph.DirectedPseudograph;

import java.util.*;

public class VRGraph {

    DirectedPseudograph<VRNode, VREdge> graph = null;

    public VRGraph() {
        // nothing yet
    }

    /**
     * Find the set of arcs between those two nodes which use the specified domain
     *
     * @param src    source node
     * @param tgt    target node
     * @param domain a single domain-name
     * @return arcs between those two nodes which use the specified domain
     */
    public Set<VREdge> domainArcsBetween(VRNode src, VRNode tgt, String domain) {
        Set<VREdge> outEdges = graph.outgoingEdgesOf(src);
        Set<VREdge> inEdges = graph.incomingEdgesOf(tgt);
        Set<VREdge> result = new HashSet<>(1);
        for (VREdge e : outEdges) {
            if (domain.equals(e.domain)) {
                if (inEdges.contains(e)) {
                    result.add(e);
                }
            }
        }
        return result;
    }

    /**
     * Find the set of arcs between those two nodes which use any of the specified domains
     *
     * @param src     source node
     * @param tgt     target node
     * @param domains set of usable domain-names
     * @return arcs between those two nodes which use any of the specified domains
     */
    public Set<VREdge> multiDomainArcsBetween(VRNode src, VRNode tgt, Set<String> domains) {
        Set<VREdge> outEdges = graph.outgoingEdgesOf(src);
        Set<VREdge> inEdges = graph.incomingEdgesOf(tgt);
        Set<VREdge> result = new HashSet<>(1);
        for (VREdge e : outEdges) {
            if (domains.contains(e.domain)) {
                if (inEdges.contains(e)) {
                    result.add(e);
                }
            }
        }
        return result;
    }

    public static VRGraph readCSV(String ifPath, String arcFileName, String nodeFileName) {

        List<ReadVRArcCSV.DataField> arcResults = ReadVRArcCSV.readCSV(ifPath, arcFileName);
        List<ReadVRNodeCSV.DataField> nodeResults = ReadVRNodeCSV.readCSV(ifPath, nodeFileName);
        VRGraph g = new VRGraph();
        DirectedPseudograph<VRNode, VREdge> ldGraph = g.initGraph();
        //Pseudograph<LogDistNode, LogDistArc> ldGraph = new Pseudograph<>(LogDistArc.class);

        Map<String, VRNode> nodeMap = new HashMap<>(2);
        for (ReadVRNodeCSV.DataField nodeRec : nodeResults) {
            VRNode ldn = new VRNode(nodeRec.name, nodeRec.latitude, nodeRec.longitude, null);
            ldGraph.addVertex(ldn);
            nodeMap.put(ldn.name, ldn);
        }


        for (ReadVRArcCSV.DataField arcRec : arcResults) {
            // Notice that the constructor must take no arguments
            // because jGraphT sets the source and target.
            VREdge lda = new VREdge();
            lda.name = arcRec.arcName;
            lda.srcName = arcRec.srcNodeName;
            lda.tgtName = arcRec.tgtNodeName;
            lda.domain = arcRec.domain;
            lda.trueLength = arcRec.trueLength;

            VRNode sNode = nodeMap.get(lda.srcName);
            VRNode tNode = nodeMap.get(lda.tgtName);
            ldGraph.addEdge(sNode, tNode, lda);
        }

        g.setGraph(ldGraph);
        return g;
    }

    public void setGraph(DirectedPseudograph<VRNode, VREdge> g) {
        graph = g;
    }

    public DirectedPseudograph<VRNode, VREdge> initGraph() {
        graph = new DirectedPseudograph<VRNode, VREdge>(VREdge.class);
        return graph;
    }

    /**
     * Return the first (if any) edge from the src to the tgt in the domain
     * <p>
     * TODO: figure out how to either minimize calling this or be more efficient
     *
     * @param srcName name of desired source node
     * @param tgtName name of desired target node
     * @param domain  name of desired domain
     * @return the first such link (if any)
     */
    public VREdge domainPickEdge(String srcName, String tgtName, String domain) {
        VREdge eStar = null;
        Set<VREdge> edges = graph.edgeSet();
        for (VREdge e : edges) {
            if ((null == eStar)
                    && e.srcName.equals(srcName)
                    && e.tgtName.equals(tgtName)
                    && e.domain.equals(domain)) {
                eStar = e;
            }
        }
        return eStar;
    }

    public Map<String, VRNode> makeNodeMap() {
        Map<String, VRNode> nMap = new HashMap<>();
        for (VRNode v : graph.vertexSet()) {
            nMap.put(v.name, v);
        }
        return nMap;
    }

    public Map<String, VREdge> makeEdgeMap() {
        Map<String, VREdge> nMap = new HashMap<>();
        for (VREdge e : graph.edgeSet()) {
            nMap.put(e.name, e);
        }
        return nMap;
    }

    public static class VRNode
            implements CountedItem {
        public VRNode(String nodeName, double latitude, double longitude, Set<String> transAccess) {
            this.name = nodeName;
            this.latitude = latitude;
            this.longitude = longitude;
            this.transAccess = transAccess;
            idNum = ItemCounter.makeID();
        }

        public String name = "";
        public double latitude = 0.0;
        public double longitude = 0.0;
        public Set<String> transAccess = new HashSet<>(1);

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

    public static class VREdge
            extends DefaultWeightedEdge
            implements CountedItem {
        public String name = "";
        public String srcName = "";
        public String tgtName = "";
        public String domain = "";
        public double trueLength = 0.0;

        /**
         * Construct a new arc.
         * Notice that the constructor must take no arguments
         * because jGraphT sets the source and target.
         */
        public VREdge() {
            super();
            idNum = ItemCounter.makeID();
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


}

// =============================================================================

