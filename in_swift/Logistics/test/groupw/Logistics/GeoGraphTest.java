/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Logistics.GeoGraph.GeoEdge;
import groupw.Logistics.GeoGraph.GeoNode;
import groupw.Network.ApproxTSP;
import groupw.Network.ApproxTSP23;
import groupw.SimpleIADS.Route;
import org.apache.commons.math4.legacy.linear.ArrayRealVector;
import org.apache.commons.math4.legacy.linear.RealVector;
import org.jgrapht.GraphPath;
import org.jgrapht.alg.shortestpath.FloydWarshallShortestPaths;
import org.jgrapht.alg.spanning.PrimMinimumSpanningTree;
import org.jgrapht.graph.WeightedPseudograph;
import org.junit.Test;

import java.util.*;

import static groupw.BaseSim.DSUtils.makeRV3;
import static groupw.Network.NWUtils.*;
import static java.lang.Math.*;
import static org.junit.Assert.*;

/**
 * @author BenWise
 */
public class GeoGraphTest {
    
    public GeoGraphTest() {
    }

    @Test
    public void ringTests() {
        System.out.println("\nStarting ringTests");
        int sd = DefaultSeedPRNG;
        Random prng = makePRNG(sd, true);
        int lineLength = 80;
        int numRT = 5; // 5 * lineLength;
        boolean verbose = (1 == numRT);
        System.out.printf("Num inner ring tests: %d\n", numRT);

        boolean connected = true;
        for (int i = 1; connected && (i <= numRT); i++) {
            GeoGraph.resetHighestID(0);
            GeoGraph gGraph = innerRingTest(verbose, prng);

            System.out.print(".");
            if (0 == (i % lineLength)) {
                System.out.print(" |\n"); // just EOL marker
                System.out.flush();
            }
        }
        System.out.println();
    }

    private GeoGraph innerRingTest(boolean verbose, Random prng) {
        double ln10 = log(10.0); // natural logarithm
        double ln50 = log(50.0); // natural logarithm
        double lnInc = ln10 + ((ln50 - ln10) * prng.nextDouble());
        int increment = (int) (0.5 + exp(lnInc)); // natural power

        int numPoints = 10 + increment; // eight cardinal plus randoms

        double rMin = 200.0 + (100.0 * prng.nextDouble());
        double rFactor = 1.1 + prng.nextDouble();
        double rMax = rMin * rFactor; // this must be larger than rMin/threshold

        // With minArc = 1, about 50% of the graphs are disconnected.
        // With minArc = 2, about 2% are disconnected.
        // With minArc = 3, less than 1 in 2000.
        int minArcs = 3; //(int) (2.0 + 2.0 * prng.nextDouble());

        int sd2 = prng.nextInt();
        if (sd2 < 0) {
            sd2 = -sd2;
        }

        if (verbose) {
            System.out.printf("Using prng seed %10d \n", sd2);
            System.out.printf("Using %3d points\n", numPoints);
            System.out.printf("Ring min/max: %5.1f / %5.1f \n", rMin, rMax);
            System.out.printf("Min arcs: %1d \n", minArcs);
        }
        // add in the dispatcher node
        GeoGraph gGraph = makeRingGraph(numPoints - 1, rMin, rMax, minArcs, verbose, sd2);
        gGraph.drawSVG("./tmp_file_A.svg");

        // basic test of JGraphT's Floyd-Warshall algorithm.
        // NOTE WELL, 1: there seems to be some kind of small round-off error somewhere.
        // The 'tolerance' should not be necessary but errors 1e-13 to 5e-13 (vs ~1000.0 edge costs) are common
        // NOTE WELL, 2: JGraphT works on a weighted psuedo-graph, so a LAWST-like thing would seem to need
        // to make multiple variants of a graph, depending on open/closed link status, which set of Domains
        // was desired, and so on. Hence, I wrote groupw.Network.FloydWarshall that works on an arbitrary
        // cost matrix, not a Graph.
        double fwTol = 1e-12;
        FloydWarshallShortestPaths<GeoNode, GeoEdge> fw = new FloydWarshallShortestPaths<>(gGraph.wpg);
        Set<GeoNode> nodes = gGraph.wpg.vertexSet();
        for (GeoNode gn1 : nodes) {
            for (GeoNode gn2 : nodes) {
                double d12 = fw.getPathWeight(gn1, gn2);
                if (gn1 == gn2) {
                    assertEquals(0.0, d12, 0.0);
                } else {
                    assertTrue(0.0 < d12);
                }
                assertTrue(d12 < Double.MAX_VALUE); // disconnected components fail this test
                for (GeoNode gn3 : nodes) {
                    if ((gn1 != gn2) && (gn2 != gn3) && (gn3 != gn1)) {
                        double d13 = fw.getPathWeight(gn1, gn3);
                        double d23 = fw.getPathWeight(gn2, gn3);
                        double costDetour = d12 + d23 - d13;
                        if (costDetour < 0.0) {
                            //System.out.printf("%8.2f + %8.2f >? %8.2f (%+9.2e)\n", d12, d23, d13, costDetour);
                            //System.out.flush();
                        }
                        assertTrue(0.0 < fwTol + costDetour);
                    }
                }
            }
        }

        // Figure out what the TSP route would be.
        // NOTE WELL: these are cache-to-cache legs, but we still would need to use the
        // underlying graph to find F-W routes between them, then use Route to
        // build fractal movement Routes
        List<GeoNode> caches = new ArrayList<>(6);
        // I know that the dispatch location is in the NE,
        double dispCoord = (2.0 * rMax) - rMin; // = rMax + (rMax - rMin)
        GeoNode dNd = gGraph.closestNode(makeRV3(dispCoord, dispCoord, 0.0));
        int dispID = dNd.getID();
        caches.add(dNd);

        // Get eight semi-evenly spaced points near inner edge as caches.
        // Of the 16 evenly spaced locations, we take the odd ones that
        // fall between the eight cardinal directions.
        // There be some duplicate indices, so it might be less than 8.
        for (int j = 0; j < 8; j++) {
            int k = 1 + (2 * j);
            GeoNode nk = findCache(k, rMin, gGraph);
            if (!caches.contains(nk)) {
                caches.add(nk);
            }
        }

        // somewhat shuffle them
        int numTSP = caches.size();
        System.out.printf("Number of caches (+ dispatcher): %d\n", numTSP);
        caches = shuffle(caches, prng);
        double[][] cMat =  new double[numTSP][numTSP];
        double[] xs = new double[numTSP];
        double[] ys = new double[numTSP];
        for  (int i = 0; i < numTSP; i++) {
            xs[i] = caches.get(i).coords.getEntry(0);
            ys[i] = caches.get(i).coords.getEntry(1);
        }
        GeoGraph.GGCoords ncs = new GeoGraph.GGCoords(numTSP, xs, ys);
        ncs.fw = fw;
        ncs.geoNodes = caches;
        for  (int i = 0; i < numTSP; i++) {
            GeoNode gn1 = caches.get(i);
            for (int j = i; j < numTSP; j++) {
                GeoNode gn2 = caches.get(j);
                double d12 = fw.getPathWeight(gn1, gn2);
                cMat[i][j] = d12;
                cMat[j][i] = d12;
            }
        }
        int maxIter = 3; // fix2, fix3, fix2, then stop
        ApproxTSP bt0 = new ApproxTSP(numTSP);
        ApproxTSP23 et0 = new ApproxTSP23(bt0.extendTour());
        double c0 = et0.costETour(ncs);
        ApproxTSP23.ImprvTour it = ApproxTSP23.improve23(et0, ncs, false, maxIter);
        ApproxTSP et1 = it.newTour;
        while(dispID != caches.get(et1.getPoint(0)).getID()) {
            et1 = ApproxTSP.rotateETourLeft(et1);
        }
        double c1 = it.newCost;
        double s = c0 - c1;
        assertTrue(0.0 <= s); // sometimes, we just get lucky
        System.out.printf("Improved %8.2f to %8.2f and saved %8.2f \n", c0, c1, s);
        System.out.printf("TSP cache-IDs in order: ");
        for (int i = 0; i < numTSP+1; i++) { // get dispatcher at start and at end
            int index = et1.getPoint(i);
            System.out.printf(" %3d", caches.get(index).getID());
        }
        System.out.printf("\n");
        double cumulativeCost = 0.0;
        double costTol = 1e-11; // compared to typical cost of 1000-5000, 1.82e-12 is largest seen so far

        double segmentFrac = 0.10;
        double segmentMin = 100.0;
        for (int i=1; i < numTSP+1; i++) {
            int ndx0 = et1.getPoint(i-1);
            int ndx1 = et1.getPoint(i);
            GeoNode gn0 = caches.get(ndx0);
            GeoNode gn1 = caches.get(ndx1);

            GraphPath<GeoNode, GeoEdge> p = fw.getPath(gn0, gn1);
            double pw = p.getWeight();

            System.out.printf("Cache-leg %3d -> %3d, cost %7.2f:", gn0.getID(), gn1.getID(), pw);

            cumulativeCost = cumulativeCost + pw;
            List<GeoNode> vl = p.getVertexList();
            assertTrue(vl.size() == 1 + p.getLength()); // 2-node path has 1 edge
            GeoNode v0 = vl.get(0);
            double[] dv0 = new double[3];
            double[] dv2 = new double[3];
            for (GeoNode gn2 : vl) {
                if (v0 != gn2) {
                    dv0[0] = v0.coords.getEntry(0);
                    dv0[1] = v0.coords.getEntry(1);
                    dv0[2] = v0.coords.getEntry(2);
                    RealVector rv0 = new ArrayRealVector(dv0);
                    dv2[0] = gn2.coords.getEntry(0);
                    dv2[1] = gn2.coords.getEntry(1);
                    dv2[2] = gn2.coords.getEntry(2);
                    RealVector rv2 = new ArrayRealVector(dv2);
                    Route routeAB = Route.fractalSegment(segmentFrac, segmentMin, rv0, rv2, prng);
                    System.out.printf(" (%2d)", routeAB.numPoints()-1); // 3-point segment has 2 edges
                }
                System.out.printf(" %3d", gn2.getID());
                v0 = gn2;
            }
            System.out.printf("\n");
        }
        //System.out.printf("%.4e vs %.4e differ %.4e \n", c1, cumulativeCost, cumulativeCost-c1);
        assertTrue(abs(c1 - cumulativeCost) < costTol);
        return gGraph;
    } // end of innerRingTest

    @Test
    public void globalLogNWTests() {
        System.out.println("\nStarting GlobalLogNWTest");
        int sd = DefaultSeedPRNG;
        Random prng = makePRNG(sd, true);
        int lineLength = 80;
        int numRT = 2; // 5 * lineLength;
        boolean verbose = (1 == numRT);
        System.out.printf("Num inner ring tests: %d\n", numRT);

        boolean connected = true;
        for (int i = 1; connected && (i <= numRT); i++) {
            GeoGraph.resetHighestID(0);
            GeoGraph gGraph = innerGlobalLogNWTest(verbose, prng);

            System.out.print(".");
            if (0 == (i % lineLength)) {
                System.out.print(" |\n"); // just EOL marker
                System.out.flush();
            }
        }
        System.out.println();
    }
    private GeoGraph innerGlobalLogNWTest(boolean verbose, Random prng) {
        GeoGraph gGraph = new GeoGraph();
        WeightedPseudograph<GeoNode, GeoEdge> wpgBase = new WeightedPseudograph<>(GeoEdge.class);
        WeightedPseudograph<GeoNode, GeoEdge> wpgAllMST = new WeightedPseudograph<>(GeoEdge.class);
        WeightedPseudograph<GeoNode, GeoEdge> wpgMerged = new WeightedPseudograph<>(GeoEdge.class);

        // TODO: 50, 5, 20 would be more reasonable to debug whole system
        // TODO: 15, 3, 8 would be reasonable to debug GAMS
        int numPoints = 50;
        int numSources = 5;
        int numSinks = 20;
        double RStar = 1000000.0;
        int desiredDegree = 5;
        double mapHeight = 850.0;
        double mapWidth = 1100.0;
        double minSep = sqrt((mapHeight*mapHeight + mapWidth*mapWidth)/numPoints);
        minSep = minSep/3.0;

        PointCoords pcs = makeUniformRect(numPoints, prng);
        //double[][] dm0 = pcs.dMat;

        GeoNode[] nodeList =  new GeoNode[numPoints];
        for (int i = 0; i < numPoints; i++) {
            GeoNode gn = new GeoNode(makeRV3(pcs.xs[i], pcs.ys[i], 0.0));
            gn.myID = i;
            nodeList[i] = gn;
        }
        
        for (int i = 0; i < numPoints; i++) {
            GeoNode gn = nodeList[i];
            wpgBase.addVertex(gn);
            wpgAllMST.addVertex(gn);
            wpgMerged.addVertex(gn);
        }

        // we will find the MST of the dense graph, record it, then delete all edges.
        for (int i = 0; i < numPoints; i++) {
            for (int j = i+1; j < numPoints; j++) {
                GeoEdge ge = wpgBase.addEdge(nodeList[i], nodeList[j]);
                wpgBase.setEdgeWeight(ge, pcs.cost(i,j));
            }
        }

        // TODO: the mergedEdges does somehow get duplicates, which it should not.
        Set<Tuple3<Integer, Integer, Double>> mergedEdges = new HashSet<>(numPoints);

        PrimMinimumSpanningTree<GeoNode, GeoEdge> primAllMST = new PrimMinimumSpanningTree<>(wpgBase);
        System.out.printf("Total weight of All MST: %.2f \n", primAllMST.getSpanningTree().getWeight());
        System.out.println("Edges in MST: \n");
        for (GeoEdge ge : primAllMST.getSpanningTree().getEdges()) {
            double w = wpgAllMST.getEdgeWeight(ge);
            GeoNode gns = wpgBase.getEdgeSource(ge);
            GeoNode gnt = wpgBase.getEdgeTarget(ge);
            System.out.printf("%3d - %3d \n", gns.getID(), gnt.getID());
            wpgAllMST.addEdge(gns, gnt);
            if (gns.getID() < gnt.getID()) {
                mergedEdges.add(new Tuple3<>(gns.getID(), gnt.getID(), w));
            }
            else {
                mergedEdges.add(new Tuple3<>(gnt.getID(), gns.getID(), w));
            }
        }

        // because they are randomly scattered, I just take the
        // first ones as sources and sinks.
        int[] sources = new int[numSources];
        for (int i = 0; i < numSources; i++) {
            sources[i] = i;
        }
        int[] sinks = new int[numSinks];
        for (int i = 0; i < numSinks; i++) {
            int j = i+numSources;
            sinks[i] = j;
        }

        int maxIter = 3; // 2-edge, 3-edge, then 2-edge
        ApproxTSP23 et0  = new ApproxTSP23(new ApproxTSP(numPoints).extendTour());
        double c0 = et0.costETour(pcs);
        ApproxTSP23.ImprvTour it = ApproxTSP23.improve23(et0, pcs, verbose, maxIter);
        ApproxTSP23 et1 = (ApproxTSP23) it.newTour;
        double c1 = it.newCost;
        double s = c0 - c1;
        double f = c0/c1;
        //System.out.printf("Improved by %.2f , divided by %.2f \n", s, f);
        int numEdges = et1.points.size(); // extended-tour, so first and last points equal
        for (int i=1; i < numEdges; i++) {
            int j = et1.points.get(i-1);
            GeoNode gnJ = nodeList[j];
            int k = et1.points.get(i);
            GeoNode gnK = nodeList[k];
            GeoEdge ge = wpgBase.addEdge(gnJ, gnK);
            double w =  pcs.cost(j,k);
            wpgBase.setEdgeWeight(ge, w);
            if (gnJ.getID() < gnK.getID()) {
                mergedEdges.add(new Tuple3<>(gnJ.getID(), gnK.getID(), w));
            }
            else {
                mergedEdges.add(new Tuple3<>(gnK.getID(), gnJ.getID(), w));
            }
        }
        for (Tuple3<Integer, Integer, Double> t : mergedEdges) {
            GeoNode gnS = nodeList[t.get0()];
            GeoNode gnT = nodeList[t.get1()];
            double w = t.get2();
            System.out.printf("Adding %2d - %2d to merged graph\n", gnS.getID(), gnT.getID());
            assertTrue(gnS.getID() < gnT.getID());
            GeoEdge ge = wpgMerged.addEdge(gnS, gnT);
            wpgMerged.setEdgeWeight(ge, w);
        }
        gGraph.setGraph(wpgMerged);
        if (0 < gGraph.numVertices()) {
            gGraph.drawSVG("./tmp_file_GlobalNW.svg");
        }
        return gGraph;
    }

    /**
     * Make a ring-shaped GeoGraph, with additional dispatcher vertex outside the ring in NE
     *
     * @param numRingPoints desired number of vertices in the ring, excluding dispatcher
     * @param rMin minimum distance from center to a vertex
     * @param rMax maximum distance from center to a vertex (excluding dispatcher)
     * @param minArcs minimum number of arcs a vertex should have
     * @param verbose print out lots of data, or not
     * @param sd seed for PRNG, possibly zero for irreproducible
     * @return a GeoGraph with the desired properties
     */
    private GeoGraph makeRingGraph(int numRingPoints,
            double rMin, double rMax,
            int minArcs,
            boolean verbose, int sd) {

        Random prng = makePRNG(sd, false);

        // The ring vertices always include the 8 cardinal points at the front of pcs
        PointCoords pcs = makeRing(numRingPoints, rMin, rMax, prng);
        assertEquals(pcs.xs.length , numRingPoints);
        assertEquals(pcs.ys.length , numRingPoints);

        // Undirected graph in which edges have weights, multiple edges between a vertex-pair are allowed, self-loops are allowed
        WeightedPseudograph<GeoNode, GeoEdge> wpg = new WeightedPseudograph<>(GeoEdge.class);

        List<GeoNode> cardinals = new ArrayList<>(8);
        for (int i = 0; i < numRingPoints; i++) {
            double xi = pcs.xs[i];
            double yi = pcs.ys[i];
            if (verbose) {
                if (i < 8) { // don't need to check those any more
                   // System.out.printf("Cardinal point , %3d , %7.2f , %7.2f \n", i, xi, yi);
                }
            }
            GeoNode gNode = new GeoNode(makeRV3(xi, yi, 0.0));
            cardinals.add(gNode);
            wpg.addVertex(gNode);
        }

        Set<GeoNode> nodes = wpg.vertexSet();
        assertEquals(numRingPoints, nodes.size());

        // Add all the carefully chosen edges inside the ring
        // First, we connect each vertex to the closest few
        int nShortEdges = 0;
        for (GeoNode gni : nodes) {
            List<Tuple2<GeoNode, Double>> distList = new ArrayList<>(numRingPoints);
            for (GeoNode gnj : nodes) {
                if (gni != gnj) {
                    Tuple2<GeoNode, Double> pr = new Tuple2<>(gnj, gni.eucDist(gnj));
                    distList.add(pr);
                }
            }
            // sort the shortest to front
            distList.sort((pr1, pr2) -> Double.compare(pr1.get1(), pr2.get1()));
            assertEquals(numRingPoints - 1, distList.size());

            for (int j = 0; ((j < numRingPoints - 1) && (wpg.degreeOf(gni) < minArcs)); j++) {
                Tuple2<GeoNode, Double> pr = distList.get(j);
                GeoNode gnj = pr.get0();
                assertNotSame(gni, gnj);
                boolean unconnected = wpg.getAllEdges(gni, gnj).isEmpty();
                if (unconnected) {
                    GeoEdge e = wpg.addEdge(gni, gnj);
                    nShortEdges++;
                    wpg.setEdgeWeight(e, pr.get1());
                }
            }
        }

        // Second, we create the outer ring.
        // Because we know the order in which GraphUtils.makeRing created them,
        // we know the following is a clockwise loop through the eight cardinal points
        int nOuterEdges = 0;
        List<Tuple2<Integer, Integer>> outerCircle = new ArrayList<>(8);
        outerCircle.add(new Tuple2<>(4, 7));
        outerCircle.add(new Tuple2<>(7, 6));
        outerCircle.add(new Tuple2<>(6, 5));
        outerCircle.add(new Tuple2<>(5, 3));
        outerCircle.add(new Tuple2<>(3, 0));
        outerCircle.add(new Tuple2<>(0, 1));
        outerCircle.add(new Tuple2<>(1, 2));
        outerCircle.add(new Tuple2<>(2, 4));
        for (Tuple2<Integer, Integer> pr : outerCircle) {
            GeoNode gns = cardinals.get(pr.get0());
            GeoNode gnd = cardinals.get(pr.get1());
            boolean unconnected = wpg.getAllEdges(gns, gnd).isEmpty();
            if (unconnected) {
                double d = gnd.eucDist(gns);
                GeoEdge e = wpg.addEdge(gns, gnd);
                nOuterEdges++;
                wpg.setEdgeWeight(e, d);
            }
        }

        // Finally, we add a point in the NE for the dispatcher.
        double dispCoord = (2.0 * rMax) - rMin; // = rMax + (rMax - rMin)
        GeoNode dNode = new GeoNode(makeRV3(dispCoord, dispCoord, 0.0));
        wpg.addVertex(dNode);

        int numEdges = wpg.edgeSet().size();
        assertEquals(nShortEdges + nOuterEdges, numEdges);

        GeoGraph gGraph = new GeoGraph();
        gGraph.setGraph(wpg);
        // I happen to know that the NE, N and E nodeSet are 7, 4 and 6
        // in the 'pcs' list. They should link to dNode.
        // They are shifted up/down ten meters to make sure no one counts on
        // everything being at zero altitude
        GeoNode gnNE = gGraph.closestNode(makeRV3(pcs.xs[7], pcs.ys[7], 10.0));
        GeoNode gnN = gGraph.closestNode(makeRV3(pcs.xs[4], pcs.ys[4], 0.0));
        GeoNode gnE = gGraph.closestNode(makeRV3(pcs.xs[6], pcs.ys[6], -10.0));

        // connect the dispatcher to the ring
        GeoEdge eDN = wpg.addEdge(dNode, gnN);
        wpg.setEdgeWeight(eDN, dNode.eucDist(gnN));

        GeoEdge eDNE = wpg.addEdge(dNode, gnNE);
        wpg.setEdgeWeight(eDNE, dNode.eucDist(gnNE));

        GeoEdge eDE = wpg.addEdge(dNode, gnE);
        wpg.setEdgeWeight(eDE, dNode.eucDist(gnE));

        assertEquals(3 + numEdges, wpg.edgeSet().size());

        // There can still be little diamond-shaped subgraphs: merge them.
        gGraph.connectAllComponents();

        return gGraph;
    }

    /**
     * Given an angular increment around the ring-graph, find the closest
     * point so that it can be used as a cache.
     *
     * @param i zero is zero degrees, 4 is North, ... 16 is zero degrees again
     * @param rMin inner radius of the ring-graph
     * @param gGraph the ring-graph
     * @return GeoNode of the selected cache (not necessarily unique)
     */
    private GeoNode findCache(int i, double rMin, GeoGraph gGraph) {
        double dTheta = (2.0 * 3.1416) / 16.0; // cos and sin use radians, of course
        GeoNode gn = gGraph.closestNode(makeRV3(rMin * cos(i * dTheta), rMin * sin(i * dTheta), 0.0));
        return gn;
    }

    @Test
    public void gridTests() {
        System.out.println("\nStarting gridTests");
        int sd = DefaultSeedPRNG;
        Random prng = makePRNG(sd, true);
        int lineLength = 80;
        int numRT = 5; // 5 * lineLength;
        boolean verbose = (1 == numRT);
        System.out.printf("Num inner grid tests: %d\n", numRT);

        boolean connected = true;
        for (int i = 1; connected && (i <= numRT); i++) {
            GeoGraph.resetHighestID(0);
            GeoGraph gGraph = innerGridTest(verbose, prng);

            System.out.print(".");
            if (0 == (i % lineLength)) {
                System.out.print(" |\n"); // just EOL marker
                System.out.flush();
            }
        }
        System.out.println();
    }

    private GeoGraph innerGridTest(boolean verbose, Random prng) {
        int nRows = 10 + iMod(prng.nextInt(), 101);
        int nClms = 10 + iMod(prng.nextInt(), 101);
        boolean shuffleP = true;
        double noise = 0.08; // highest noise with maxDist == 1.3
        GeoGraph gGraph = makeNoisyGridGraph(
                nRows, nClms, shuffleP, noise, verbose, prng.nextInt());
        return gGraph;
    }

    /**
     * Make a gridded GeoGraph, with additional dispatcher vertex outside the ring in NE
     *
     * @param nRows number of rows in the grid
     * @param nClms number of clms in the grid
     * @param shuffleP should they be shuffled?
     * @param noise random noise in otherwise-integral coordinates
     * @param sd seed for PRNG, possibly zero for irreproducible
     * @return a GeoGraph with the desired properties
     */
    private GeoGraph makeNoisyGridGraph(int nRows, int nClms,
            boolean shuffleP, double noise, boolean verbose, int sd) {
        int numGridPoints = nRows * nClms;
        double maxDist = 1.3; // 1.414 is perfect diagonal
        Random prng = makePRNG(sd, false);
        assertTrue(0.0 <= noise);
        assertTrue(noise <= 0.4); // keep some gap
        PointCoords pcs = makeNoisyGrid(nRows, nClms, shuffleP, noise, prng);
        assertEquals(pcs.xs.length, numGridPoints);
        assertEquals(pcs.ys.length, numGridPoints);

        // Undirected graph in which edges have weights, multiple edges between a vertex-pair are allowed, self-loops are allowed
        WeightedPseudograph<GeoNode, GeoEdge> wpg = new WeightedPseudograph<>(GeoEdge.class);

        List<GeoNode> nodeList = new ArrayList<>(numGridPoints);
        for (int i = 0; i < numGridPoints; i++) {
            double xi = pcs.xs[i];
            double yi = pcs.ys[i];
            GeoNode gNode = new GeoNode(makeRV3(xi, yi, 0.0));
            nodeList.add(gNode);
            wpg.addVertex(gNode);
        }

        Set<GeoNode> nodeSet = wpg.vertexSet();
        assertEquals(numGridPoints, nodeSet.size());

        // We connect each vertex to the closest few
        for (int n = 0; n < numGridPoints - 1; n++) {
            GeoNode gn = nodeList.get(n);
            for (int m = n + 1; m < numGridPoints; m++) {
                GeoNode gm = nodeList.get(m);
                double d = gn.eucDist(gm);
                if (d < maxDist) {
                    GeoEdge e = wpg.addEdge(gn, gm); // undirected edge
                    wpg.setEdgeWeight(e, d);
                }
            }
        }
        // this is the number of edges in a perfect grid
        int minEdges = 2 * nRows * nClms - (nRows + nClms);
        int numGridEdges = wpg.edgeSet().size();
        // to extraneous diagonals, tune 'noise' and 'maxDist'
        assertEquals(minEdges, numGridEdges);

        if (verbose) {
            System.out.printf("%d , %d , %d , %d \n",
                    nRows, nClms, numGridPoints, numGridEdges);
        }
        GeoGraph gGraph = new GeoGraph();
        gGraph.setGraph(wpg);

        return gGraph;
    }

}
// =============================================================================
