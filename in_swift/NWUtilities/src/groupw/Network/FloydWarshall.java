package groupw.Network;

import java.util.ArrayList;
import java.util.List;

/**
 * The multi-source Floyd-Warshall algorithm.
 * Though JGraphT has an F-W implementation, we need a version that
 * has lots of special cases of what links are or are not available.
 * Rather than generate a new graph for each case, we build a new cost matrix
 * for each case.
 * @author BenWise
 */
public class FloydWarshall {

    /**
     * Given a matrix of costs, calculate the minimum cost of routes
     * between all nodes. Caches enough information to quickly reconstruct the
     * min-cost routes later.
     * The cost matrix need not be symmetric.
     *
     * @param numPoints number of points in a graph
     * @param costMat matrix of costs, mostly non-negative
     */
    public void calcMinCost(int numPoints, double[][] costMat, boolean verbose) {
        minCost = new double[numPoints][numPoints];
        midNode = new int[numPoints][numPoints];

        for (int i = 0; i < numPoints; i++) {
            for (int j = 0; j < numPoints; j++) {
                if (i == j) {
                    minCost[i][j] = 0;
                    midNode[i][j] = i;
                } else {
                    minCost[i][j] = costMat[i][j];  // possibly Double.MAX_VALUE
                    // if there is a direct link i-j that is the shortest link,
                    // then this will never be reset. Hence, we set it to
                    // the correct answer in that case. All others will be reset.
                    midNode[i][j] = Integer.min(i, j);
                }
            }
        }

        // This triple-loop is the actual Floyd-Warshall algorithm
        for (int k = 0; k < numPoints; k++) {
            for (int i = 0; i < numPoints; i++) {
                for (int j = 0; j < numPoints; j++) {
                    double cik = minCost[i][k];
                    double ckj = minCost[k][j];
                    if ((cik < Double.MAX_VALUE) && (ckj < Double.MAX_VALUE)) {
                        double c = cik + ckj;
                        if (c < minCost[i][j]) {
                            //System.out.printf("Best mid point between [%4d->%4d] is %3d\n", i, j, k);
                            minCost[i][j] = c;
                            midNode[i][j] = k;
                        }
                    }
                }
            }
        }
    }


    /**
     * Using the cached minimum-cost matrix, calculate the shortest path from A to B.
     * This algorithm has runtime approximately N * lg(N) for path length, N.
     * With lots of (A,B) pairs to define paths, it would probably only be worthwhile
     * to cache some of the most-used paths (if any).
     *
     * @param a index of a node
     * @param b index of a node
     * @return List of nodes, in order, on the path from A to B
     */
    public List<Integer> calcMinPath(int a, int b) {
        List<Integer> path;
        int c = getMidNode(a, b);
        if ((a == c) || (b == c)) { // direct link is shortest path
            path = new ArrayList<>(2);
            path.add(a);
            path.add(b);
        } else {
            path = calcMinPath(a, c);
            List<Integer> pathCB = calcMinPath(c, b);
            int lengthCB = pathCB.size();
            for (int i = 1; i < lengthCB; i++) { // skip duplicate node 'C'
                path.add(pathCB.get(i));
            }
        }
        return path;
    }
    public double getMinCost(int i, int j) {
        return minCost[i][j];
    }

    public double[][] getMinCostMatrix() {
        return minCost;
    }

    public int getMidNode(int i, int j) {
        return midNode[i][j];
    }

    protected double[][] minCost = null;
    protected int[][] midNode = null;
}



// =============================================================================
