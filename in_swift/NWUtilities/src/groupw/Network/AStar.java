/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Network;

import groupw.Network.NWUtils.Tuple2;
import org.jgrapht.graph.DirectedPseudograph;

import java.util.PriorityQueue;
import java.util.HashSet;
import java.util.Set;
import java.util.List;
import java.util.ArrayList;

/**
 * Abstract class to perform A* search over a given graph.
 * The cost of a path is assumed to be the sum of the node and edge costs.
 * We use directed edges because transport networks sometimes have different
 * costs in different directions (e.g. uphill versus downhill, one lane closed, one-way roads, etc.).
 * We use functions for the costs because costs can be highly dynamic
 * (e.g. congestion dependent on how many vehicles are already planning to use a edge, and when)
 * and we do not want to create lots of slightly different copies of the graph.
 * @param <NType> the type of nodes for the underlying directed pseudo-graph
 * @param <EType> the type of directed edges for the underlying pseudo-graph
 */
abstract public class AStar<NType, EType> {

    public static class SearchStatistics {
        public double pqMean = 0.0;
        public double pqVariance = 0.0;
        public int pqMax = 0;
        public int iterations = 0;

        public SearchStatistics(double pqMean, double pqVariance, int pqMax, int iterations) {
            this.pqMean = pqMean;
            this.pqVariance = pqVariance;
            this.pqMax = pqMax;
            this.iterations = iterations;
        }
    }

    public AStar(DirectedPseudograph<NType, EType> g) {
        myGraph = g;
    }

    public DirectedPseudograph<NType, EType> myGraph;

    /**
     * Non-negative cost to traverse a node
     *
     * @param n the node to be traversed
     * @return non-negative cost of traversing this node (usually positive)
     */
    abstract public double getNodeCost(NType n);

    /**
     * Non-negative (usually positive) cost to traverse a directed edge
     *
     * @param e the directed edge to be traversed
     * @return non-negative cost of traversing this edge (almost always positive)
     */
    abstract public double getEdgeCost(EType e);

    /**
     * This MUST be non-negative (usually positive) underestimate to cost from
     * N1 to N2, assuming one is already at N1. Thus, nodeCost to enter N1 never is included,
     * but nodeCost to enter N2 likely is included.
     * This MUST never be zero unless n1 == n2.
     *
     * @param n1 start node
     * @param n2 finish node
     * @return non-negative estimate of cost, usually underestimated
     */
    abstract public double estCost(NType n1, NType n2);

    public Tuple2<Route<NType, EType>, SearchStatistics> findRoute(NType startNode, NType goalNode) {
        PriorityQueue<Route<NType, EType>> routeQueue = new PriorityQueue<>();
        double myEps = 0.001;
        int searchIteration = 0; // Integer.MAX_VALUE is 2^31 - 1
        long pqSize = 0; // Long.MAX_VALUE is 2^63 - 1
        long pqMax = 0;
        long pq1Sum = 0;
        long pq2Sum = 0;
        Set<Route<NType, EType>> irs = initialRoutes(startNode, goalNode);
        routeQueue.addAll(irs);

        pqSize = routeQueue.size();
        pqMax = pqSize; // so far
        searchIteration = searchIteration + 1;
        pq1Sum = pq1Sum + pqSize;
        pq2Sum = pq2Sum + (pqSize * pqSize);

        boolean searchDone = false;
        int maxIterations = 500 * (myGraph.vertexSet().size() + myGraph.edgeSet().size());
        Route<NType, EType> bestRoute = null;
        while (!searchDone) {
            boolean foundGoal = false;
            Route<NType, EType> r = routeQueue.poll(); // pop it off

            if (null != r) {
                foundGoal = r.hasNode(goalNode);
                if (foundGoal) {
                    bestRoute = r;
                    searchDone = true;
                } else {
                    Set<Route<NType, EType>> kids = extensions(r, goalNode);
                    routeQueue.addAll(kids);
                }
            } else {
                searchDone = true;
            }

            pqSize = routeQueue.size();
            pqMax = Math.max(pqSize, pqMax);
            searchIteration = searchIteration + 1;
            pq1Sum = pq1Sum + pqSize;
            pq2Sum = pq2Sum + (pqSize * pqSize); // 64-bit avoids rollover to negative when 46340.95 < pqSize


            if (maxIterations < searchIteration) {
                throw new RuntimeException("Exceeded iteration limit of " + maxIterations);
            }
        }
        while (!routeQueue.isEmpty()) {
            Route<NType, EType> r = routeQueue.poll();
            //System.out.printf("Remaining size %3d, this cost %8.3f \n", routeQueue.size(), r.getEstCost());
        }
        double pq1Mean = ((double) pq1Sum) / ((double) searchIteration);
        double pq2Mean = ((double) pq2Sum) / ((double) searchIteration);
        double pqVariance = pq2Mean - (pq1Mean * pq1Mean);
        if (pqVariance < 0.0) {
            throw new RuntimeException("Negative variance in A* queue size");
        }
        SearchStatistics stats = new SearchStatistics(pq1Mean, pqVariance, (int) pqMax, (int) searchIteration);
        return new Tuple2<>(bestRoute, stats);
    }

    /**
     * Get the first set of 1-edge routes from the start node,
     * with cost estimates relative to the goal node.
     *
     * @param sNode start node
     * @param gNode goal node
     * @return set of 1-edge routes
     */
    public Set<Route<NType, EType>> initialRoutes(NType sNode, NType gNode) {
        Set<EType> outgoing = myGraph.outgoingEdgesOf(sNode);
        Set<Route<NType, EType>> s = new HashSet<>(outgoing.size());
        double sCost = getNodeCost(sNode);
        for (EType e : outgoing) {
            NType n1 = myGraph.getEdgeTarget(e);
            if (!n1.equals(sNode)) {
                Route<NType, EType> rt1 = new Route<>(new ArrayList<EType>(1), myGraph);
                rt1.arcs.add(e);
                double eCost = getEdgeCost(e);
                double nCost = getNodeCost(n1);
                rt1.exactCostSoFar = sCost + eCost + nCost; // extra vars to aid debugging
                rt1.estCostToGo = estCost(n1, gNode);
                s.add(rt1);
            }
        }
        return s;
    }

    /**
     * Return a set of new routes that extend this one without creating loops.
     * Costs are updated relative to the provided goal node
     *
     * @param rt0   route to be extended
     * @param gNode goal to be reached
     * @return set of new routes
     */
    public Set<Route<NType, EType>> extensions(Route<NType, EType> rt0, NType gNode) {
        int length0 = rt0.arcs.size();
        // because the underlying graph is directed, this is some kind of directed edge
        EType dirEdge = rt0.arcs.get(length0 - 1);
        NType n0 = myGraph.getEdgeTarget(dirEdge);
        Set<EType> outgoing = myGraph.outgoingEdgesOf(n0);
        Set<Route<NType, EType>> s = new HashSet<>(outgoing.size());
        for (EType e : outgoing) {
            NType n1 = myGraph.getEdgeTarget(e);
            boolean loopP = rt0.hasNode(n1);
            if (!loopP) {
                Route<NType, EType> rt1 = extend(rt0, e, gNode);
                s.add(rt1);
            }
        }
        return s;
    }

    /**
     * Extend a route by appending an edge.
     * The target of the last node in the route must match the start of the new edge.
     *
     * @param rt1   the route to be extended
     * @param e1    the edge to append to the last node of the route
     * @param gNode the goal node of the AStar search
     * @return extended route with updated cost estimates
     */
    public Route<NType, EType> extend(Route<NType, EType> rt1, EType e1, NType gNode) {
        EType e0 = rt1.arcs.get(rt1.arcs.size() - 1);
        NType n0 = myGraph.getEdgeTarget(e0);
        NType n1 = myGraph.getEdgeSource(e1);
        if (!n0.equals(n1)) {
            throw new RuntimeException("Last node of route must connect to extending edge");
        }
        Route<NType, EType> r2 = new Route<>(new ArrayList<>(rt1.arcs.size() + 1), myGraph);
        r2.arcs.addAll(rt1.arcs);
        r2.arcs.add(e1);
        double eCost = getEdgeCost(e1);
        NType n2 = myGraph.getEdgeTarget(e1);
        double nCost = getNodeCost(n2);
        r2.exactCostSoFar = rt1.exactCostSoFar + eCost + nCost; // extra vars to aid debugging
        r2.estCostToGo = estCost(n2, gNode);
        return r2;
    }

    /**
     * A Route is an ordered list of directed edges, plus the associated costs.
     * Uses the 'natural order' of Double: lowest estimated cost first.
     *
     * @param <NType> the Node type of the underlying directed graph
     * @param <EType> the Edge type of the underlying directed graph
     */
    static public class Route<NType, EType> implements Comparable<Route<NType, EType>> {
        public List<EType> arcs = null;
        public double exactCostSoFar = 0.0;
        public double estCostToGo = 0.0;

        DirectedPseudograph<NType, EType> myGraph = null;

        public Route(List<EType> arcs, DirectedPseudograph<NType, EType> myGraph) {
            this.arcs = arcs;
            this.myGraph = myGraph;
        }

        /**
         * Return the estimated start-to-finish cost of this unfinished route
         *
         * @return estimated cost
         */
        public double getEstCost() {
            return exactCostSoFar + estCostToGo;
        }

        public boolean hasNode(NType nd) {
            boolean found = false;
            int n = arcs.size();
            for (int i = 0; (!found) && (i < n); i++) {
                EType e = arcs.get(i);
                if (nd.equals(myGraph.getEdgeSource(e)) || nd.equals(myGraph.getEdgeTarget(e))) {
                    found = true;
                }
            }
            return found;
        }

        /**
         * Method to determine if 'this' route should be before 'other' in priority queue
         *
         * @param other the object to be compared.
         * @return -1, 0 or +1 as per Double.compare of getEstCost
         */
        @Override
        public int compareTo(Route<NType, EType> other) {
            // Usually compare(x,y) = sign(x-y), but it handles NaN, +/- infinity, etc.
            // As of 2025-08, +0.0 is more than -0.0
            return Double.compare(this.getEstCost(), other.getEstCost());
        }
    }
}


// =============================================================================
