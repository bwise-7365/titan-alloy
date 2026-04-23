/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Network.NWUtils.Tuple2;
import org.jgrapht.alg.cycle.CycleDetector;
import org.jgrapht.graph.DefaultEdge;
import org.jgrapht.graph.SimpleDirectedGraph;

import java.util.*;

/**
 * The Dispatcher class has two basic functions.
 * The first is to roll up demand for supplies from supported units to the
 * units responsible for supporting them.
 * The second is to push down supplies from the supporting units
 * to those they support, within the limits of vehicles available, own supplies,
 * time, distance, and accessibility of the physical network.
 */
public class Dispatcher {

    public String getName() {
        return name;
    }

    protected String name;
    protected LogDistNW ldnw;
    protected LogSprtNW lsnw;
    protected Map<String, Unit> unitMap;
    protected SimpleDirectedGraph<String, DefaultEdge> dGraph = null;
    protected List<Tuple2<String, Integer>> height = null;

    /**
     * Summed over this Dispatcher's units and all supported Dispatchers,
     * reorder level, maximum desired level, current level and consumption rate per 24 hours
     */
    public SupplyStatus sStatus = null;

    public List<Tuple2<String, Integer>> getHeight() {
        return height;
    }

    public void setHeight(List<Tuple2<String, Integer>> height) {
        this.height = height;
    }

    /**
     * Initialize the dispatcher with the complete distribution network, complete support network,
     * and units on the board.
     *
     * @param name
     * @param ldnw
     * @param lsnw
     * @param unitMap
     */
    public Dispatcher(String name, LogDistNW ldnw, LogSprtNW lsnw, Map<String, Unit> unitMap) {
        this.name = name;
        this.ldnw = ldnw;
        this.lsnw = lsnw;
        this.unitMap = unitMap;
    }

    /**
     * Find the set of 1 or more supporting units to be controlled by this Dispatcher.
     * Find the set of 0 or more supported units to be served by this Dispatcher.
     */
    public void setSupports() {
        // get all the support nodes which this dispatcher will control
        supporting = new HashSet<>(1);
        Set<LogSprtNW.SupportNode> sNodes = lsnw.getNodes();
        for (LogSprtNW.SupportNode sNode : sNodes) {
            if (name.equals(sNode.getDispatcherName())) {
                supporting.add(sNode);
            }
        }
        //System.out.printf("Found %d supporting nodes for dispatcher %s\n",  supporting.size(), name); // for debugging

        // the set of supported will be empty for front-line units
        supported = new HashSet<>(1);
        for (LogSprtNW.SupportNode sNode : supporting) {
            Set<LogSprtNW.SupportArc> arcs = lsnw.lsGraph.edgesOf(sNode);

            // find the arcs that point out from (not in to) this node
            // and record the supported nodes to which they point
            for (LogSprtNW.SupportArc a : arcs) {
                if (sNode.equals(a.getSrc())) {
                    supported.add(a.getTgt());
                }
            }
        }
        //System.out.printf("Found %d supported nodes for dispatcher %s\n",  supported.size(), name); // for debugging

    }

    /**
     * Get the supply, demand and rates from the lowest Dispatchers
     * then roll up the Tree of dispatchers to the highest. This tracks totals
     * for dispatchers and allocates responsibility across supplying nodes.
     */
    public static void rollUpDispatchers(Set<Dispatcher> dispatchers) {
        Map<String, Dispatcher> dMap = makeDispatcherMap(dispatchers);
        Dispatcher[] dList = dispatchers.toArray(new Dispatcher[0]);
        // roll up in order from the lowest to highest
        List<Tuple2<String, Integer>> heights = dList[0].getHeight();
        for (Tuple2<String, Integer> t : heights) {
            Dispatcher d = dMap.get(t.get0());
            d.rollUpDispatchers(dMap);
            for (LogSprtNW.SupportNode sn : d.supporting) {
                sn.rollUp(d.lsnw, d.unitMap);
            }
        }
    }

    /**
     * Rollup demands, levels, etc. through the Dispatcher graph,
     * without allocating any of them to the units in the dispatcher's group.
     * @param dMap map from dispatcher names to dispatchers
     */
    private void rollUpDispatchers(Map<String, Dispatcher> dMap) {
        System.out.printf("\nStarting rollup of Dispatcher %s \n", name);
        sStatus = new SupplyStatus();
        // Add up levels and rates for units actually in this dispatcher's group
        System.out.printf("Dispatcher %s has %d supporting units \n", name, supporting.size());
        for (LogSprtNW.SupportNode sNode : supporting) {
            Unit u = unitMap.get(sNode.unitName);
            sStatus = sStatus.add(u.sStatus);
        }

        // Add up the levels and rates for the supported dispatchers, if any.
        Set<DefaultEdge> out = dGraph.outgoingEdgesOf(name);
        System.out.printf("Dispatcher %s has %d supported dispatchers \n", name, out.size());
        for (DefaultEdge e : out) {
            String n = dGraph.getEdgeTarget(e);
            Dispatcher d = dMap.get(n);
            sStatus = sStatus.add(d.sStatus);
        }

        System.out.printf("Dispatcher %s (and subordinates) has current levels \n", name);
        System.out.printf("  %s \n", sStatus.currentLevel.toString());
        System.out.printf("Dispatcher %s updated \n", name); // debugging only
    }

public static Map<String, Dispatcher> makeDispatcherMap(Set<Dispatcher> dispatchers) {
    Map<String, Dispatcher> dMap = new HashMap<String, Dispatcher>();
    for (Dispatcher d : dispatchers) {
        dMap.put(d.name, d);
    }
    return dMap;
}

    public SimpleDirectedGraph<String, DefaultEdge> getDGraph() {
        return dGraph;
    }

    public void setDGraph(SimpleDirectedGraph<String, DefaultEdge> dGraph) {
        this.dGraph = dGraph;
    }

    public static SimpleDirectedGraph<String, DefaultEdge> makeDispatcherGraph(Set<Dispatcher> dispatchers) {
        SimpleDirectedGraph<String, DefaultEdge> dGraph = new SimpleDirectedGraph<>(DefaultEdge.class);
        for (Dispatcher d : dispatchers) {
            dGraph.addVertex(d.getName());
        }
        for (Dispatcher d : dispatchers) {
            Set<String> subs = new HashSet<>();
            for (LogSprtNW.SupportNode sn : d.supported) {
                subs.add(sn.getDispatcherName());
            }
            for (String sub : subs) {
                Set<DefaultEdge> currEdges = dGraph.getAllEdges(d.getName(), sub);
                if (currEdges.isEmpty()) {
                    dGraph.addEdge(d.getName(), sub);
                }
            }
        }
        return dGraph;
    }
/*
// as of 2025-09, there is no easy way in jgraphT to find cycles of an undirected graph.
    public static boolean checkDGraphTree(SimpleDirectedGraph<String, DefaultEdge> dGraph) {
        SimpleGraph<String, DefaultEdge>  sGraph = new SimpleGraph<>(DefaultEdge.class);
        for (String n : dGraph.vertexSet()) {
            sGraph.addVertex(n);
        }
        for (DefaultEdge e : dGraph.edgeSet()) {
            sGraph.addEdge(dGraph.getEdgeSource(e), dGraph.getEdgeTarget(e));
        }
        CycleDetector<String, DefaultEdge> cd = new CycleDetector<>(sGraph);
        boolean result = cd.detectCycles();
        return result;
    }
    */

    /**
     * Determine if there are any cycles in the Dispatcher graph
     *
     * @param dGraph directed graph of which Dispatcher supports which
     * @return true iff a cycle was found
     */
    public static boolean checkCyclic(SimpleDirectedGraph<String, DefaultEdge> dGraph) {
        CycleDetector<String, DefaultEdge> cd = new CycleDetector<>(dGraph);
        return cd.detectCycles();
    }

    /**
     * Assuming the graph has already been verified as acyclic, assign heights to each Dispatcher.
     * Breadth first search and jgraphT's getDepth does not do what we want, because
     * we do not initially know which are the 'root' (highest) Dispatcher(s).
     *
     * @param dGraph directed graph of which Dispatcher supports which
     * @return ordered list of (Dispatcher, height) pairs, lowest height first.
     */
    public static List<Tuple2<String, Integer>> calcHeights(SimpleDirectedGraph<String, DefaultEdge> dGraph) {
        Map<String, Integer> heights = new HashMap<>();
        Set<String> vertices = dGraph.vertexSet();
        Set<DefaultEdge> edges = dGraph.edgeSet();

        // all vertices have negative initial height
        for (String dName : vertices) {
            heights.put(dName, -1);
        }

        // mark height of edges as zero
        for (String dName : vertices) {
            Set<DefaultEdge> arcsToSubs = dGraph.outgoingEdgesOf(dName);
            if (arcsToSubs.isEmpty()) {
                heights.put(dName, 0);
            }
        }
        boolean changed = true;
        while (changed) {
            changed = false;
            for (String dName : vertices) {
                if (heights.get(dName) < 0) { // this one not yet marked
                    Set<DefaultEdge> arcsToSubs = dGraph.outgoingEdgesOf(dName);
                    boolean foundNegative = false;
                    int maxHeight = -1;
                    for (DefaultEdge e : arcsToSubs) {
                        String sub = dGraph.getEdgeTarget(e);
                        int tmpHeight = heights.get(sub);
                        if (tmpHeight < 0) {
                            foundNegative = true;
                        } else {
                            maxHeight = Math.max(maxHeight, tmpHeight);
                        }
                    }
                    if (!foundNegative) {
                        heights.put(dName, maxHeight + 1);
                        changed = true;
                    }
                }
            }
        }
        List<Tuple2<String, Integer>> heightList = new ArrayList<>(heights.size());
        for (Map.Entry<String, Integer> entry : heights.entrySet()) {
            heightList.add(new Tuple2<>(entry.getKey(), entry.getValue()));
        }
        heightList.sort((s1, s2) -> Integer.compare(s1.get1(), s2.get1()));
        return heightList;
    }
    // --------------


    protected Set<LogSprtNW.SupportNode> supporting = null;
    protected Set<LogSprtNW.SupportNode> supported = null;
}


// =============================================================================
