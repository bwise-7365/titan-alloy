/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Network;

import groupw.Network.NWUtils.PointCoords;

import java.io.FileWriter;
import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import groupw.Network.NWUtils.Tuple2;
import static groupw.Network.NWUtils.iMod;
import static groupw.Network.NWUtils.makeNoisyGrid;
import static groupw.Network.NWUtils.makePRNG;
import static groupw.Network.NWUtils.makeRing;
import static groupw.Network.NWUtils.makeUniformRect;
import static groupw.Network.NWUtils.shuffle;
import static groupw.Network.NWUtils.makeTetrahedon;

/**
 *
 * This class provides utility functions for approximate solution of the TSP.
 * An ApproxTSP for the traveling salesman problem is just a list of indices to be
 * visited in order. To use with the unordered points of a GeoGraph, we have to
 * get an ordered List of GeoNodes, build a cost matrix, and so on.
 * See GeoGraphTest for the canonical example.
 *
 * @author BenWise
 *
 */
public class ApproxTSP {


    public ApproxTSP() {
        initialize();
    }

    public ApproxTSP(int n) {
        initialize();
        for (int i = 0; i < n; i++) {
            add(i);
        }
    }

    public ApproxTSP(List<Integer> ps) {
        initialize();
        int n = ps.size();
        for (int p : ps) {
            add(p);
        }
    }

    public ApproxTSP(ApproxTSP t0) {
        numPoints = t0.numPoints;
        points = new ArrayList<>(t0.points);
    }

    protected void initialize() {
        numPoints = 0;
        points = new ArrayList<>(17);
    }

    public List<Integer> getPoints() {
        return points;
    }

    ;


    public int getNumPoints() {
        numPoints = points.size();
        return numPoints;
    }

    public int getPoint(int i) {
        return points.get(i);
    }

    /**
     * Determine if this is a basic, loop-free tour.
     *
     * @return T if a basic ApproxTSP, F otherwise
     */
    public boolean bTourP() {
        boolean r = !eTourP();
        return r;
    }

    /**
     * Add up cost of each edge in the extended ApproxTSP to get total cost
     *
     * @param pcs
     * @return cost of the ApproxTSP
     */
    public double costETour(PointCoords pcs) {
        assert (eTourP());
        assert (numPoints == points.size());
        //System.out.printf("Calculating tour cost over %2d, %2d points\n", numPoints, points.size());
        double c = 0.0;
        for (int i = 0; i < numPoints - 1; i++) {
            int j = i + 1;
            assert (j < numPoints);
            int ndxI = points.get(i);
            int ndxJ = points.get(j);
            //System.out.printf("  %2d  %2d ->  %2d  %2d \n", i, j, ndxI, ndxJ);
            double dij = pcs.cost(ndxI, ndxJ); //cMat[ndxI][ndxJ];
            if (dij <= 0.0) {
                System.out.printf("Non-positive cost %d %d %.3f \n",
                        ndxI, ndxJ, dij);
                System.out.flush();
            }
            assert (0.0 < dij); // as i != j
            c = c + dij;
            assert (0.0 < c);
            //System.out.printf("Edge %2d %2d has cost %.2f for total %.2f \n",  ndxI, ndxJ, dij, dMax);
        }
        //System.out.printf("ECost: %.2f \n", dMax);
        return c;
    }

    public double highestEdgeCost(PointCoords pcs) {
        assert (eTourP());
        assert (numPoints == points.size());
        double dMax = 0.0;
        for (int i = 0; i < numPoints - 1; i++) {
            int j = i + 1;
            assert (j < numPoints);
            int ndxI = points.get(i);
            int ndxJ = points.get(j);
            double dij = pcs.cost(ndxI, ndxJ); // cMat[ndxI][ndxJ];
            assert (0.0 < dij);
            if (dMax < dij) {
                dMax = dij;
            }
            assert (0.0 < dMax);
        }
        return dMax;
    }

    static public Tuple2<ApproxTSP, PointCoords> ringTSP(boolean verbose, int sd) {
        Random prng = makePRNG(sd, verbose);
        // these intermediate variables are to ensure that it is the same size as gridTSP
        int nRows = (int) (10.0 + 40 * prng.nextDouble());
        int nClms = (int) (10.0 + 40 * prng.nextDouble());
        double rMin = 300.0;
        double rMax = 375.0;

        nRows = 15;
        nClms = 10;
        rMin = 400;
        rMax = 600;

        int nPoints = nRows * nClms;

        PointCoords pc = makeRing(nPoints, rMin, rMax, prng);
        if (verbose) {
            System.out.printf("Made ring of %d points in [%.2f, %.2f] ring\n", nPoints, rMin, rMax);
        }
        ApproxTSP bt0 = new ApproxTSP(nPoints);
        Tuple2<ApproxTSP, PointCoords> rslt = new Tuple2<>(bt0, pc);
        return rslt;
    }

    static public Tuple2<ApproxTSP, PointCoords> gridTSP(int nRows, int nClms, boolean verbose, int sd) {
        Random prng = makePRNG(sd, verbose);
        double noise = 0.0; //0.2;

        int nPoints = nRows * nClms;
        boolean shuffleP = true;
        PointCoords pc = makeNoisyGrid(nRows, nClms, shuffleP, noise, prng);
        if (verbose) {
            System.out.printf("Made noisy grid for [%3d, %3d] array of %d \n", nRows, nClms, nPoints);
        }
        ApproxTSP bt0 = new ApproxTSP(nPoints);
        Tuple2<ApproxTSP, PointCoords> rslt = new Tuple2<>(bt0, pc);
        return rslt;
    }

    static public Tuple2<ApproxTSP, PointCoords> rectTSP(int nRows, int nClms, boolean verbose, int sd) {
        Random prng = makePRNG(sd, verbose);
        double noise = 0.0; //0.2;

        int nPoints = nRows * nClms;
        boolean shuffleP = true;
        PointCoords pc = null;
        if (true) {
            pc = makeUniformRect(nPoints, prng);
            System.out.printf("Made uniform rectangle of %d random points\n", nPoints);
        }
        else {
            pc = makeNoisyGrid(nRows, nClms, shuffleP, noise, prng);
            System.out.printf("Made noisy grid of %d x %d (%d)  points\n",
                    nRows, nClms, nPoints);
        }
        ApproxTSP bt0 = new ApproxTSP(nPoints);
        Tuple2<ApproxTSP, PointCoords> rslt = new Tuple2<>(bt0, pc);
        return rslt;
    }

    /**
     * Create the patterns described in "Hard to solve instances of the Euclidean Traveling Salesman Problem"
     * by Stefan Hougardy and Xianghi Zhong, 2020.
     * My ApproxTSP23 heuristic appears to achieve the optima they show for T(17,17) and T'(48,24)
     * in under a second. They report hours for their method to solve the same problem.
     *
     * @param verbose
     * @param sd
     * @return
     */
    static public Tuple2<ApproxTSP, PointCoords> tetrahedon(boolean verbose,
            int outerLength, int innerLength,
            boolean modifiedP, int sd) {
        Random prng = makePRNG(sd, verbose);
        PointCoords pc = makeTetrahedon(outerLength, innerLength, modifiedP, prng);
        int nPoints = pc.nPoints;
        if (verbose) {
            if (modifiedP) {
                System.out.printf("Made modified tetrahedron (%d, %d) with %d points\n", outerLength, innerLength, nPoints);
            } else {
                System.out.printf("Made basic tetrahedron (%d, %d) with %d points\n", outerLength, innerLength, nPoints);
            }
        }
        ApproxTSP bt0 = new ApproxTSP(nPoints);
        Tuple2<ApproxTSP, PointCoords> rslt = new Tuple2<>(bt0, pc);
        return rslt;
    }

    /**
     * Determine if this Tour is an 'extended tour', i.e. a loop where the first
     * and last points match.
     *
     * @return T if an extended ApproxTSP, F otherwise
     */
    public boolean eTourP() {
        assert (1 < numPoints);
        assert (points.size() == numPoints);
        int pFirst = points.get(0);
        int pLast = points.get(numPoints - 1);
        return (pFirst == pLast);
    }

    public void add(int k) {
        assert (numPoints == points.size());
        points.add(k);
        numPoints = points.size();
    }

    public ApproxTSP extendTour() {
        assert (bTourP());
        ApproxTSP t2 = new ApproxTSP();
        t2.points = points;
        t2.numPoints = points.size();
        int pFirst = points.get(0);
        t2.add(pFirst);
        return t2;
    }

    public ApproxTSP baseTour() {
        assert (eTourP());
        ApproxTSP t2 = new ApproxTSP();
        int n2 = numPoints - 1;
        t2.points = new ArrayList<Integer>(n2);
        for (int i = 0; i < n2; i++) {
            int p = points.get(i);
            t2.add(p);
        }
        assert (n2 == t2.points.size());
        return t2;
    }

    public ApproxTSP shuffleBaseTour(Random prng) {
        assert (bTourP());
        ApproxTSP t2 = new ApproxTSP(this);
        t2.points = shuffle(t2.points, prng);
        return t2;
    }

    public void showWholeTour( PointCoords pcs) {
        double c0 = costETour(pcs);
        System.out.printf("Cost of tour over %5d points: %.4f \n",
                numPoints, c0);
        for (int i = 0; i < numPoints; i++) {
            int k = points.get(i);
            double x = pcs.xs[k];
            double y = pcs.ys[k];
            System.out.printf("%5d , %5d , %7.2f , %7.2f  \n", i, k, x, y);
        }
    }

    public void saveWholeTour(String currentDir, String ofName, PointCoords pcs) {
        double c0 = costETour(pcs);
        try (PrintWriter w = new PrintWriter(new FileWriter(currentDir + ofName))) {

            System.out.printf("Cost of tour over %5d points: %.4f ... ",
                    numPoints, c0);
            w.printf("Cost of tour over %5d points: %.4f \n", numPoints, c0);
            for (int i = 0; i < numPoints; i++) {
                int k = points.get(i);
                double x = pcs.xs[k];
                double y = pcs.ys[k];
                w.printf("%5d , %5d , %7.2f , %7.2f  \n", i, k, x, y);
            }
            System.out.printf("done\n");
        } catch (IOException e) {
            System.err.println("Error writing to file: " + e.getMessage());
        }

    }

    public void showEdgeIndices() {
        for (int i=1; i<numPoints; i++) {
            int j = points.get(i-1);
            int k = points.get(i);
            System.out.printf("(%3d, %3d) \n", j, k);
        }
    }

    static public ApproxTSP rotateETourLeft(ApproxTSP et1) {
        assert (et1.eTourP());
        ApproxTSP bt1 = et1.baseTour();
        ApproxTSP bt2 = rotateBTourLeft(bt1);
        ApproxTSP et2 = bt2.extendTour();
        return et2;
    }

    static protected ApproxTSP rotateBTourLeft(ApproxTSP bt1) {
        assert (bt1.bTourP());
        int n = bt1.numPoints;
        ApproxTSP bt2 = new ApproxTSP();
        for (int i = 1; i < n; i++) {
            int p = bt1.points.get(i);
            bt2.add(p);
        }
        int p = bt1.points.get(0);
        bt2.add(p);
        return bt2;
    }

    /**
     * If 'item' is in the tour, rotate a copy so that 'item' is in position 0.
     * Otherwise, return a copy of the original.
     *
     * @param item point to be rotated to front
     * @param t1 Tour to be rotated if 'item' is present
     * @return either a new, rotated tour or a copy of the original t1
     */
    static public ApproxTSP rotateToFront(int item, ApproxTSP t1) {
        ApproxTSP t2;
        if (t1.eTourP()) {
            ApproxTSP bt1 = t1.baseTour();
            ApproxTSP bt2 = rotateToFrontBT(item, bt1);
            t2 = bt2.extendTour();
        } else { // base tour
            t2 = rotateToFrontBT(item, t1);
        }
        return t2;
    }

    static protected ApproxTSP rotateToFrontBT(int item, ApproxTSP t1) {
        ApproxTSP t2 = new ApproxTSP(t1);
        if (t1.points.contains(item)) {
            int numPoints = t2.numPoints;
            int nRotate = -1;
            for (int i = 0; (i < numPoints) && (nRotate < 0); i++) {
                if (item == t1.points.get(i)) {
                    nRotate = i;
                }
            }
            for (int i = 0; i < numPoints; i++) {
                int j = iMod(i - nRotate, numPoints); // always non-negative
                t2.points.set(j, t1.points.get(i));
            }
        }
        return t2;
    }
    // -------------------------------------------------------------------------
    // Miscellaneous

    // Because Java has no C++-style 'tuple',
    // we define here all the tuples required
    /**
     * Basic structure to carry tuple of results
     */
    static public class ImprvTour {

        public ImprvTour(boolean improved, int numImproved, int numIter, ApproxTSP newTour) {
            this.improved = improved;
            this.numImproved = numImproved;
            this.numIter = numIter;
            this.newTour = newTour;
        }

        public boolean improved = false;
        public int numImproved = 0;
        public int numIter = 0;
        public ApproxTSP newTour;
        public double newCost = 0.0;
    }

    static protected class IndexImprv {

        public IndexImprv(int index, double improvement) {
            this.index = index;
            this.improvement = improvement;
        }
        protected int index;
        protected double improvement;
    }
    public int numPoints = -1;
    public List<Integer> points;

}


// =============================================================================
