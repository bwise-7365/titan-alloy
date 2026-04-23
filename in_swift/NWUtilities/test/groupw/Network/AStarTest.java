/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Network;

import groupw.Network.NWUtils.Tuple2;
import org.jgrapht.graph.DefaultEdge;
import org.junit.Test;
import org.jgrapht.graph.DirectedPseudograph;

import java.util.Map;
import java.util.HashMap;
import java.util.Random;
import java.util.List;
import java.util.ArrayList;

import static groupw.Network.NWUtils.makePRNG;
import static groupw.Network.NWUtils.nFromRC;
import static java.lang.Math.abs;
import static java.lang.Math.sqrt;
import static org.junit.Assert.*;

/**
 *
 * @author BenWise
 */
public class AStarTest {

    public AStarTest() {
    }

    @Test
    public void asTest1a() {
        double myEps = 0.001;
        Tuple2<AStar<City, DefaultEdge>, List<City>> result = makeGraph1();
        AStar<City, DefaultEdge> ast = result.get0();
        List<City> cities = result.get1();
        //Set<City> cities = ast.myGraph.vertexSet();
        Tuple2<AStar.Route<City, DefaultEdge>, AStar.SearchStatistics>
                searchResult = ast.findRoute(cities.get(0), cities.get(11));
        AStar.Route<City, DefaultEdge> route = searchResult.get0();
        assertNotNull(route); // for this case, I know the result
        int rl = route.arcs.size();
        double ecsf = route.exactCostSoFar;
        double ectg = route.estCostToGo;
        System.out.printf("Found route of length %d with cost %.4f/%.4f\n",
                rl, ecsf, ectg);
        assertEquals(5, rl); // for this case, I know the result
        assertTrue(abs(ecsf - 298.1549) < myEps); // for this case, I know the result
        assertTrue(abs(ectg) < myEps); // for this case, I know the result
        for (int i = 0; i < rl; i++) {
            DefaultEdge e = route.arcs.get(i);
            System.out.printf("  [%s,%s] %.2f \n",
                    ast.myGraph.getEdgeSource(e),
                    ast.myGraph.getEdgeTarget(e),
                    ast.getEdgeCost(e));
        }
        AStar.SearchStatistics stats = searchResult.get1();
        System.out.printf("Queue mean size %.4f +/- %.4f, maximum length %d over %d iterations \n",
                stats.pqMean, sqrt(stats.pqVariance), stats.pqMax, stats.iterations);
        assertTrue(abs(stats.pqMean - 7.6875) < myEps);
        assertTrue(abs(sqrt(stats.pqVariance) - 3.1169) < myEps);
        assertEquals(13, stats.pqMax);
        assertEquals(16, stats.iterations);
    }

    @Test
    public void asTest1b() {
        double myEps = 0.001;
        Tuple2<AStar<City, DefaultEdge>, List<City>> result = makeGraph1();
        AStar<City, DefaultEdge> ast = result.get0();
        List<City> cities = result.get1();
        int numCities = cities.size();
        assertEquals(numCities, ast.myGraph.vertexSet().size());
        for (int i = 0; i < numCities; i++) {
            for (int j = 0; j < numCities; j++) {
                if (i != j) {
                    Tuple2<AStar.Route<City, DefaultEdge>, AStar.SearchStatistics>
                            searchResult = ast.findRoute(cities.get(i), cities.get(j));
                    AStar.Route<City, DefaultEdge> route = searchResult.get0();
                    AStar.SearchStatistics stats = searchResult.get1();
                    System.out.printf("%2d->%2d, %8s->%8s: ", i, j, cities.get(i).name, cities.get(j).name);
                    System.out.printf("In %2d iterations, max size %2d, ", stats.iterations, stats.pqMax);
                    if (null != route) {
                        System.out.printf("found route of length %2d and cost %8.2f (%5.2f)\n",
                                route.arcs.size(), route.exactCostSoFar,
                                ((double) stats.iterations) / ((double) route.arcs.size()));
                        assertTrue(abs(route.estCostToGo) < myEps);
                        assertTrue(route.arcs.size() <= stats.iterations);
                    } else {
                        System.out.print("no route found\n");
                    }
                }
            }
        }
    }

    public static class City {
        public String name;
        public double X;
        public double Y;

        public City(String name, double y, double x) {
            this.name = name;
            this.Y = y;
            this.X = x;
        }

        public double dist(City other) {
            double dx = this.X - other.X;
            double dy = this.Y - other.Y;
            return sqrt((dx * dx) + (dy * dy));
        }

        @Override
        public String toString() {
            return "[" + name + " " + X + " " + Y + "]";
        }
    }

    /**
     * Version 1 of AStar test graph
     */
    public static class ASGV1 extends AStar<City, DefaultEdge> {


        public Map<City, Double> nodeCost;
        public Map<DefaultEdge, Double> edgeCost;

        public ASGV1(DirectedPseudograph<City, DefaultEdge> g,
                     Map<City, Double> nodeCost,
                     Map<DefaultEdge, Double> edgeCost) {
            super(g);
            this.nodeCost = nodeCost;
            this.edgeCost = edgeCost;
        }

        @Override
        public double getNodeCost(City n) {
            return nodeCost.get(n);
        }

        @Override
        public double getEdgeCost(DefaultEdge e) {
            return edgeCost.get(e);
        }

        @Override
        public double estCost(City n1, City n2) {
            return n1.dist(n2);
        }
    }

    private Tuple2<AStar<City, DefaultEdge>, List<City>> makeGraph1() {
        int numCities = 12;
        int numEdges = 26;
        Map<City, Double> nodeCost = new HashMap<>(numCities);
        Map<DefaultEdge, Double> edgeCost = new HashMap<>(numEdges);
        DirectedPseudograph<City, DefaultEdge> myGraph = new DirectedPseudograph<>(DefaultEdge.class);
        Random prng = makePRNG(1173, true); // must be reproducible for debugging

        City athens = new City("Athens", 34, 168);
        City berlin = new City("Berlin", 30, 66);
        City canada = new City("Canada", 99, 132);
        City dresden = new City("Dresden", 157, 110);
        City england = new City("England", 32, 112);
        City france = new City("France", 162, 73);
        City greece = new City("Greece", 111, 184);
        City hungary = new City("Hungary", 157, 160);
        City istanbul = new City("Istanbul", 53, 26);
        City jackson = new City("Jackson", 103, 87);
        City kansas = new City("Kansas", 94, 57);
        City london = new City("London", 160, 41);

        List<City> cities = new ArrayList<>(12);
        cities.add(athens);
        cities.add(berlin);
        cities.add(canada);
        cities.add(dresden);
        cities.add(england);
        cities.add(france);
        cities.add(greece);
        cities.add(hungary);
        cities.add(istanbul);
        cities.add(jackson);
        cities.add(kansas);
        cities.add(london);


        nodeCost.put(athens, 4.29);
        nodeCost.put(berlin, 4.05);
        nodeCost.put(canada, 2.86);
        nodeCost.put(dresden, 8.53);
        nodeCost.put(england, 4.22);
        nodeCost.put(france, 3.35);
        nodeCost.put(greece, 4.52);
        nodeCost.put(hungary, 2.89);
        nodeCost.put(istanbul, 5.48);
        nodeCost.put(jackson, 6.9);
        nodeCost.put(kansas, 9.7);
        nodeCost.put(london, 6.3);

        System.out.printf("Now have %d cities: \n", nodeCost.size());
        for (Map.Entry<City, Double> cd : nodeCost.entrySet()) {
            //System.out.printf("%s \n", cd.getKey().toString());
            myGraph.addVertex(cd.getKey());
        }
        assertEquals(numCities, myGraph.vertexSet().size());

        List<Tuple2<City, City>> rawEdges = new ArrayList<>(numEdges);
        rawEdges.add(new Tuple2<>(athens, athens));
        rawEdges.add(new Tuple2<>(athens, england));
        rawEdges.add(new Tuple2<>(athens, greece));

        rawEdges.add(new Tuple2<>(berlin, istanbul));

        rawEdges.add(new Tuple2<>(canada, athens));
        rawEdges.add(new Tuple2<>(canada, hungary));
        rawEdges.add(new Tuple2<>(canada, jackson));

        rawEdges.add(new Tuple2<>(dresden, dresden));
        rawEdges.add(new Tuple2<>(dresden, france));
        rawEdges.add(new Tuple2<>(dresden, france));

        rawEdges.add(new Tuple2<>(england, berlin));
        rawEdges.add(new Tuple2<>(england, berlin));
        rawEdges.add(new Tuple2<>(england, canada));
        rawEdges.add(new Tuple2<>(england, england));

        rawEdges.add(new Tuple2<>(france, canada));
        rawEdges.add(new Tuple2<>(france, london));

        rawEdges.add(new Tuple2<>(greece, canada));
        rawEdges.add(new Tuple2<>(greece, hungary));

        rawEdges.add(new Tuple2<>(hungary, dresden));

        rawEdges.add(new Tuple2<>(istanbul, kansas));
        rawEdges.add(new Tuple2<>(istanbul, london));

        rawEdges.add(new Tuple2<>(jackson, berlin));
        rawEdges.add(new Tuple2<>(jackson, england));

        rawEdges.add(new Tuple2<>(kansas, london));

        rawEdges.add(new Tuple2<>(london, jackson));
        rawEdges.add(new Tuple2<>(london, london));

        double distMin = 5.0;
        double distRange = 20.0;
        double distFracNoise = 0.25;
        for (Tuple2<City, City> re : rawEdges) {
            double d = re.get0().dist(re.get1());
            if (d < distMin) { // e.g. self-loop with d==0
                d = distMin + (distRange * prng.nextDouble());
            } else {
                d = d * (1.0 + (distFracNoise * prng.nextDouble()));
            }
            DefaultEdge e = myGraph.addEdge(re.get0(), re.get1());
            edgeCost.put(e, d);
        }

        System.out.printf("Now have %d edges: \n", edgeCost.size());
        assertEquals(numEdges, myGraph.edgeSet().size());
        /*
        for (Map.Entry<DefaultEdge, Double> ed : edgeCost.entrySet()) {
            DefaultEdge e = ed.getKey();
            double d = ed.getValue();
            System.out.printf("%8s -> %8s %6.2f\n",
                    myGraph.getEdgeSource(e).name,
                    myGraph.getEdgeTarget(e).name,
                    d);
        }
        */

        AStar<City, DefaultEdge> ast = new ASGV1(myGraph, nodeCost, edgeCost);
        return new Tuple2<>(ast, cities);
    }

    /**
     * Version 2 of AStar test graph
     */
    public static class ASGV2 extends AStar<Tuple2<Integer, Integer>, DefaultEdge> {


        public int nRows;
        public int nClms;
        public Map<DefaultEdge, Double> edgeCost;

        public ASGV2(DirectedPseudograph<Tuple2<Integer, Integer>, DefaultEdge> g,
                     int nRows, int nClms,
                     Map<DefaultEdge, Double> edgeCost) {
            super(g);
            this.nRows = nRows;
            this.nClms = nClms;
            this.edgeCost = edgeCost;
        }

        @Override
        public double getNodeCost(Tuple2<Integer, Integer> n) {
            return 0.0;
        }

        @Override
        public double getEdgeCost(DefaultEdge e) {
            return edgeCost.get(e);
        }

        /**
         * Returns underestimate of cost to go in a grid-graph.
		 * If it were perfectly regular, Manhattan distance would be exact.
		 * Euclidean distance is such an underestimate that it gives terrible performance
		 * (but useful for debugging). Therefore, we use weighted average of
		 * Manhattan and Euclidean distances.
         * @param n1 start node
         * @param n2 finish node
         * @return
         */
        @Override
        public double estCost(Tuple2<Integer, Integer> n1, Tuple2<Integer, Integer> n2) {
            double dx = abs(n1.get0() - n2.get0());
            double dy = abs(n1.get1() - n2.get1());
            double dManhattan = dx + dy;
            double dEuclid = sqrt((dx * dx) + (dy * dy));
            return (4.0 * dManhattan + dEuclid) / 5.0;
        }
    }


    public Tuple2<AStar<Tuple2<Integer, Integer>, DefaultEdge>,
            List<Tuple2<Integer, Integer>>> makeGraph2() {
        int nRows = 100; // 56;
        int nClms = 162; //90;
        double nWidth = 0.4;
        double scale = 100;

        DirectedPseudograph<Tuple2<Integer, Integer>, DefaultEdge> myGraph = new DirectedPseudograph<>(DefaultEdge.class);
        Random prng = makePRNG(1173, true); // must be reproducible for debugging
        List<Integer> rOffSet = new ArrayList<>(4);
        List<Integer> cOffSet = new ArrayList<>(4);
        rOffSet.add(0);
        rOffSet.add(0);
        rOffSet.add(+1);
        rOffSet.add(-1);
        cOffSet.add(+1);
        cOffSet.add(-1);
        cOffSet.add(0);
        cOffSet.add(0);

        List<Tuple2<Integer, Integer>> xy = new ArrayList<>(nRows * nClms);
        for (int r = 0; r < nRows; r++) {
            for (int c = 0; c < nClms; c++) {
                int n = nFromRC(r, c, nRows, nClms);
                assertEquals(n, xy.size());
                int x = ((int) (scale * (1.0 + c + nWidth * (prng.nextDouble() - 0.5))));
                int y = ((int) (scale * (1.0 + r + nWidth * (prng.nextDouble() - 0.5))));
                xy.add(new Tuple2<Integer, Integer>(x, y));
                myGraph.addVertex(xy.get(n));
            }
        }
        int nVertices = myGraph.vertexSet().size();
        System.out.printf("Now have %4d vertices\n", nVertices);
        assertEquals(nVertices, nRows * nClms);

        for (int r1 = 0; r1 < nRows; r1++) {
            for (int c1 = 0; c1 < nClms; c1++) {
                for (int k = 0; k < 4; k++) {
                    int r2 = r1 + rOffSet.get(k);
                    int c2 = c1 + cOffSet.get(k);
                    if ((0 <= r2) && (r2 < nRows) && (0 <= c2) && (c2 < nClms)) {
                        int n1 = nFromRC(r1, c1, nRows, nClms);
                        int n2 = nFromRC(r2, c2, nRows, nClms);
                        myGraph.addEdge(xy.get(n1), xy.get(n2));
                    }
                }
            }
        }
        int nEdges = myGraph.edgeSet().size();
        System.out.printf("Now have %5d edges\n", nEdges);
        assertEquals(nEdges, estimateEdges(nRows, nClms));
        Map<DefaultEdge, Double> eCost = new HashMap<>(nEdges);
        for (DefaultEdge e : myGraph.edgeSet()) {
            Tuple2<Integer, Integer> n1 = myGraph.getEdgeSource(e);
            Tuple2<Integer, Integer> n2 = myGraph.getEdgeTarget(e);
            double dx = n1.get0() - n2.get0();
            double dy = n1.get1() - n2.get1();
            double d = sqrt((dx * dx) + (dy * dy));
            eCost.put(e, d);
        }
        AStar<Tuple2<Integer, Integer>, DefaultEdge> asg = new ASGV2(myGraph, nRows, nClms, eCost);

        //Tuple2<AStar<Tuple2<Integer, Integer>, DefaultEdge>, List<Tuple2<Integer, Integer>>> makeResult = new Tuple2<>(asg, xy);
        return new Tuple2<>(asg, xy);
    }

    @Test
    public void asTest2a() {
        // the next three lines show why Java 10 added the 'var' keyword for type-inference
        Tuple2<AStar<Tuple2<Integer, Integer>, DefaultEdge>,
                List<Tuple2<Integer, Integer>>> makeResult = makeGraph2();
        AStar<Tuple2<Integer, Integer>, DefaultEdge> asg = makeResult.get0();
        List<Tuple2<Integer, Integer>> xy = makeResult.get1();

        Tuple2<Integer, Integer> xyFirst = xy.get(0);
        Tuple2<Integer, Integer> xyLast = xy.get(xy.size() - 1);
        System.out.printf("Lower left vertex:  %5d %5d \n",
                xyFirst.get0(), xyFirst.get1());
        System.out.printf("Upper right vertex: %5d %5d \n",
                xyLast.get0(), xyLast.get1());


        Tuple2<AStar.Route<Tuple2<Integer, Integer>, DefaultEdge>, AStar.SearchStatistics> searchResult = asg.findRoute(xyFirst, xyLast);
        AStar.Route<Tuple2<Integer, Integer>, DefaultEdge> route = searchResult.get0();
        AStar.SearchStatistics stats = searchResult.get1();
        System.out.printf("PQueue mean: %.2f, stdv: %.2f, maximum: %8d, iterations: %8d\n",
                stats.pqMean, sqrt(stats.pqVariance), stats.pqMax, stats.iterations);
        System.out.printf("Route length: %4d  cost %.2f \n",
                route.arcs.size(), route.exactCostSoFar);
    }

    /**
     * Number of directed horizontal and vertical edges in n*m grid
     *
     * @param n number of rows
     * @param m number of columns
     * @return number of directed edges
     */
    private int estimateEdges(int n, int m) {
        return (4 * n * m) - (2 * (n + m));
    }
}

// =============================================================================
