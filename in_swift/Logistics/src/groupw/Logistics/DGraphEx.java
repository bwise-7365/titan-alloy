/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

/*
 * Adapted from https://jgrapht.org/guide/UserOverview#choosing-vertex-and-edge-types
 *
 */
package groupw.Logistics;

import java.util.List;
import org.jgrapht.Graph;
import org.jgrapht.alg.connectivity.KosarajuStrongConnectivityInspector;
import org.jgrapht.alg.cycle.CycleDetector;
import org.jgrapht.alg.interfaces.ShortestPathAlgorithm.SingleSourcePaths;
import org.jgrapht.alg.interfaces.StrongConnectivityAlgorithm;
import org.jgrapht.alg.shortestpath.DijkstraShortestPath;
import org.jgrapht.graph.*;

/**
 *
 * @author BenWise
 */
public class DGraphEx {

    public DGraphEx() {
        dGraph = null;
    }

    public void setup() {
        // constructs a DefaultDirectedGraph<String, DefaultEdge> with the specified vertices and edges
        dGraph = new DefaultDirectedGraph<>(DefaultEdge.class);
        dGraph.addVertex("a");
        dGraph.addVertex("b");
        dGraph.addVertex("c");
        dGraph.addVertex("d");
        dGraph.addVertex("e");
        dGraph.addVertex("f");
        dGraph.addVertex("g");
        dGraph.addVertex("h");
        dGraph.addVertex("i");
        dGraph.addEdge("a", "b");
        dGraph.addEdge("b", "d");
        dGraph.addEdge("d", "c");
        dGraph.addEdge("c", "a");
        dGraph.addEdge("e", "d");
        dGraph.addEdge("e", "f");
        dGraph.addEdge("f", "g");
        dGraph.addEdge("g", "e");
        dGraph.addEdge("h", "e");
        dGraph.addEdge("i", "h");
    }

    public void process() {// computes all the strongly connected components of the directed graph
        StrongConnectivityAlgorithm<String, DefaultEdge> scAlg
                = new KosarajuStrongConnectivityInspector<>(dGraph);
        List<Graph<String, DefaultEdge>> stronglyConnectedSubgraphs
                = scAlg.getStronglyConnectedComponents();

        // prints the strongly connected components
        System.out.println("Strongly connected components:");
        for (int i = 0; i < stronglyConnectedSubgraphs.size(); i++) {
            System.out.println(stronglyConnectedSubgraphs.get(i));
        }
        System.out.println();

        // Prints the shortest path from vertex i to vertex c. This certainly
        // exists for our particular directed graph.
        System.out.println("Shortest path from i to c:");
        DijkstraShortestPath<String, DefaultEdge> dijkstraAlg
                = new DijkstraShortestPath<>(dGraph);
        SingleSourcePaths<String, DefaultEdge> iPaths = dijkstraAlg.getPaths("i");
        System.out.println(iPaths.getPath("c") + "\n");

        // Prints the shortest path from vertex c to vertex i. This path does
        // NOT exist for our particular directed graph. Hence the path is
        // empty and the result must be null.
        System.out.println("Shortest path from c to i:");
        SingleSourcePaths<String, DefaultEdge> cPaths = dijkstraAlg.getPaths("c");
        System.out.println(cPaths.getPath("i"));

    }

    public boolean cycleCheck() {
        CycleDetector<String, DefaultEdge> cd = new CycleDetector<>(dGraph);
        boolean rslt = cd.detectCycles();
        return rslt;
    }

    DefaultDirectedGraph<String, DefaultEdge> dGraph;
}


// =============================================================================
