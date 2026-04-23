/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import com.jsevy.jsvg.SVGDocument;
import groupw.Network.NWUtils;
import groupw.Network.NWUtils.Tuple2;

import static groupw.BaseSim.DSUtils.makeRV3;
import static groupw.BaseSim.DSUtils.segmentIntersect;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import org.apache.commons.math4.legacy.linear.RealVector;
import org.jgrapht.alg.connectivity.ConnectivityInspector;
import org.jgrapht.alg.shortestpath.FloydWarshallShortestPaths;
import org.jgrapht.graph.DefaultWeightedEdge;
import org.jgrapht.graph.WeightedPseudograph;

/**
 *
 * @author BenWise
 */
public class GeoGraph {

    public GeoGraph() {
        wpg = null;
    }


    /**
     * This extends the PointCoords object (testing only) so that we
     * can use things like TSP restricted to a GeoGraph.
     * See GeoGraphTest for examples.
     */
    public static class GGCoords
            extends NWUtils.PointCoords {

        public GGCoords(int nPoints, double[] xs, double[] ys) {
            super(nPoints, xs, ys);
        }

        public GGCoords(NWUtils.PointCoords pcs) {
            super(pcs.nPoints, pcs.xs, pcs.ys);
        }

        @Override
        public double cost(int i, int j) {
            GeoNode gn1 = geoNodes.get(i);
            GeoNode gn2 = geoNodes.get(j);
            return fw.getPathWeight(gn1, gn2);
        }

        public FloydWarshallShortestPaths<GeoNode, GeoEdge> fw;
        public List<GeoNode> geoNodes;
    }


    public void drawSVG(String filePath) {
        // output SVG picture of the GeoGraph
        double inchWidth = 11.0;
        double inchHeight = 8.5;
        double inchMargin = 0.6;
        DrawGeoGraph dgg1 = new DrawGeoGraph(this);
        SVGDocument svgDoc1 = dgg1.drawSVG(inchWidth, inchHeight, inchMargin);
        dgg1.outputSVGtoFile(svgDoc1, filePath);
        dgg1 = null;
    }

    public void setGraph(WeightedPseudograph<GeoNode, GeoEdge> g) {
        wpg = g;
    }

    /**
     * Do the segments defined by (nda0, nda1) and (ndb0, ndb1) intersect?
     * This makes the most sense in two dimensions.
     *
     * @param nda0 an end of the first segment
     * @param nda1 other end of the first segment
     * @param ndb0 an end of the second segment
     * @param ndb1 other end of the second segment
     * @return true iff they intersect
     */
    public boolean intersectingEdges(GeoNode nda0, GeoNode nda1,
                                     GeoNode ndb0, GeoNode ndb1) {
        double thresh = 1.0E-10;
        boolean result = segmentIntersect(nda0.coords, nda1.coords,
                ndb0.coords, ndb1.coords,
                thresh);
        return result;
    }

    /**
     * Does this (probably hypothetical) edge intersect any existing edge?
     *
     * @param nda0
     * @param nda1
     * @return
     */
    public boolean edgeIntersectsGraph(GeoNode nda0, GeoNode nda1) {
        Set<GeoEdge> edges = wpg.edgeSet();
        boolean intersects = false;
        for (GeoEdge edge : edges) {
            if (!intersects) {
                GeoNode src = edge.getSrc();
                GeoNode dst = edge.getDst();
                intersects = intersectingEdges(nda0, nda1, src, dst);
            }
        }
        return intersects;
    }

    public GeoNode closestNode(RealVector cs) {
        double bestDist = Double.MAX_VALUE;
        GeoNode bestNode = null;
        Set<GeoNode> gns = wpg.vertexSet();
        for (GeoNode n : gns) {
            double d = n.eucDist(cs);
            if (d < bestDist) {
                bestNode = n;
                bestDist = d;
            }
        }
        return bestNode;
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

    /**
     * If necessary, successively connect closest components until the whole graph is connected.
     */
    public void connectAllComponents() {
        int numCC = connectClosestComponent();
        while (1 < numCC) {
            numCC = connectClosestComponent();
        }
    }

    /**
     * If there are two or more connected components, connect just the Euclidean-closest
     * pair by one edge and return the number of components AFTER connection.
     * If just one component initially, do nothing and return 1.
     *
     * @return Number of connect components after one or zero connection added.
     */
    public int connectClosestComponent() {
        ConnectivityInspector<GeoNode, GeoEdge> inspector = new ConnectivityInspector<>(wpg);
        List<Set<GeoNode>> ccs = inspector.connectedSets();
        int numCC = ccs.size();
        if (1 < numCC) {
            Tuple2<GeoNode, GeoNode> closestPair = null;
            double closestDist = Double.POSITIVE_INFINITY;
            for (int i = 0; i < numCC; i++) {
                Set<GeoNode> ci = ccs.get(i);
                for (int j = i + 1; j < numCC; j++) {
                    Set<GeoNode> cj = ccs.get(j);
                    for (GeoNode gni : ci) {
                        for (GeoNode gnj : cj) {
                            double dij = gni.eucDist(gnj);
                            assert (0.0 < dij); // they are in different components
                            if (dij < closestDist) {
                                closestDist = dij;
                                closestPair = new Tuple2<>(gni, gnj);
                            }
                        }
                    }
                }
            }
            assert (closestPair != null);
            GeoEdge ge = wpg.addEdge(closestPair.get0(), closestPair.get1());
            wpg.setEdgeWeight(ge, closestDist);
            numCC = numCC - 1;
        }
        return numCC;
    }

    /**
     * Build a graph whose structure is known and testable. There are two
     * connected components. The first has six nodes, some pairs of which are
     * connected by multiple arcs. It has eleven arcs, one of which is a
     * loop-to-self. The second has five nodes, again with loops and multiple
     * connections. It has eight arcs.
     */
    public static GeoGraph makeTestGraphA() {
        GeoGraph gGraph = new GeoGraph();
        WeightedPseudograph<GeoNode, GeoEdge> wpg = new WeightedPseudograph<>(GeoEdge.class);

        List<GeoNode> nodes = new ArrayList<>(11);
        // First component
        GeoNode gn00 = new GeoNode(makeRV3(1.0, 5.0, +4.2));
        GeoNode gn01 = new GeoNode(makeRV3(7.0, 4.0, -4.6));
        GeoNode gn02 = new GeoNode(makeRV3(-1.0, 1.0, +1.8));
        GeoNode gn03 = new GeoNode(makeRV3(6.0, 0.0, +3.2));
        GeoNode gn04 = new GeoNode(makeRV3(-1.0, -1.0, +1.6));
        GeoNode gn05 = new GeoNode(makeRV3(5.0, -1.0, -3.4));
        wpg.addVertex(gn00);
        wpg.addVertex(gn01);
        wpg.addVertex(gn02);
        wpg.addVertex(gn03);
        wpg.addVertex(gn04);
        wpg.addVertex(gn05);

        // Second component
        GeoNode gn06 = new GeoNode(makeRV3(11.0, 7.0, 0.0));
        GeoNode gn07 = new GeoNode(makeRV3(15.0, 6.0, 0.0));
        GeoNode gn08 = new GeoNode(makeRV3(11.0, 3.0, 0.0));
        GeoNode gn09 = new GeoNode(makeRV3(13.0, 3.0, 0.0));
        GeoNode gn10 = new GeoNode(makeRV3(10.0, 4.0, 0.0));
        wpg.addVertex(gn06);
        wpg.addVertex(gn07);
        wpg.addVertex(gn08);
        wpg.addVertex(gn09);
        wpg.addVertex(gn10);

        wpg.addEdge(gn00, gn02);  // edge  0
        wpg.addEdge(gn03, gn05);  // edge  1
        wpg.addEdge(gn03, gn01);  // edge  2
        wpg.addEdge(gn04, gn02);  // edge  3
        wpg.addEdge(gn00, gn01);  // edge  4
        wpg.addEdge(gn01, gn03);  // edge  5
        wpg.addEdge(gn02, gn00);  // edge  6
        wpg.addEdge(gn05, gn02);  // edge  7
        wpg.addEdge(gn05, gn04);  // edge  8
        wpg.addEdge(gn03, gn01);  // edge  9
        wpg.addEdge(gn04, gn04);  // edge 10

        wpg.addEdge(gn09, gn08);  // edge  11
        wpg.addEdge(gn06, gn06);  // edge  12
        wpg.addEdge(gn10, gn08);  // edge  13
        wpg.addEdge(gn09, gn07);  // edge  14
        wpg.addEdge(gn08, gn09);  // edge  15
        wpg.addEdge(gn06, gn07);  // edge  16
        wpg.addEdge(gn08, gn07);  // edge  17
        wpg.addEdge(gn10, gn06);  // edge  18
        wpg.addEdge(gn06, gn06);  // edge  19

        gGraph.setGraph(wpg);
        return gGraph;
    }

    static public class GeoNode {

        public GeoNode(RealVector cs) {
            name = "unnamed";
            coords = cs;
            dimensions = cs.getDimension();
            myID = highestID++;
        }

        public int getID() {
            return myID;
        }

        public double eucDist(GeoNode g2) {
            double d = (coords.subtract(g2.coords)).getNorm();
            return d;
        }

        public double eucDist(RealVector g2) {
            double d = (coords.subtract(g2)).getNorm();
            return d;
        }

        public String name = "unnamed";
        protected int myID = 0;

        /**
         * Lat/Lon/Alt, Geocentric, whatever
         */
        public int dimensions = 2; // reasonable default
        public RealVector coords;

    }

    /**
     * Construct a new GeoEdge arc.
     * Notice that the constructor must take no arguments
     * because jGraphT sets the source and target.
     * <p>
     * Extend DefaultWeightedEdge just enough to get the source and destination GeoNodes
     */
    static public class GeoEdge extends DefaultWeightedEdge {

        // Just in case some SWIFT code does serialize this
        private static final long serialVersionUID = 1000;

        public GeoNode getSrc() {
            return (GeoNode) getSource();
        }

        public GeoNode getDst() {
            return (GeoNode) getTarget();
        }
    }

    /**
     * The underlying JGraphT representation of an undirected, weighted graph.
     * Multiple edges between a vertex-pair and self-loop are allowed.
     */
    protected WeightedPseudograph<GeoNode, GeoEdge> wpg = null;
    static private int highestID = 0;

    public int numVertices() {
        int num = 0;
        if (null != wpg) {
            num = wpg.vertexSet().size();
        }
        return num;
    }

    public int numEdges() {
        int num = 0;
        if (null != wpg) {
            num = wpg.edgeSet().size();
        }
        return num;
    }
}

// =============================================================================
