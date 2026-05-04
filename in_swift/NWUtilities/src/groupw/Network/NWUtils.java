/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.Network;

import static groupw.Network.NWUtils.ReportingLevel.Medium;

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import static java.lang.Double.max;
import static java.lang.Math.*;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;

/**
 * Misc. utilities for graphs.
 *
 * @author BenWise
 */
public class NWUtils {

    public static enum ReportingLevel {
        Silent, Low, Medium, High, Debugging
    }

    public static boolean rLevelLE(ReportingLevel rlA, ReportingLevel rlB) {
        boolean le = (rlA.ordinal() <= rlB.ordinal());
        return le;
    }

    /**
     * Make a dense matrix of all point-to-point Euclidean distances
     *
     * @param xs List of X coordinates
     * @param ys List of Y coordinates
     * @return matrix of distances
     */
    static public double[][] makeEucDistMatrix(List<Double> xs, List<Double> ys) {
        int nPnts = xs.size();
        assert (ys.size() == nPnts);
        double[][] dMat = new double[nPnts][nPnts];
        for (int i = 0; i < nPnts; i++) {
            for (int j = i; j < nPnts; j++) {
                double dx = xs.get(i) - xs.get(j);
                double dy = ys.get(i) - ys.get(j);
                double d = sqrt((dx * dx) + (dy * dy));
                dMat[i][j] = d;
                dMat[j][i] = d;
            }
        }
        return dMat;
    }

    /**
     * Scatter points uniformly in a rectangle with sides in 11:8.5 ratio.
     * Avoid putting points too close together.
     * Calculate cost matrix assuming every node is connected to every node.
     *
     * @param nPnts number of points to be generated
     * @param prng pseudo random number generator
     * @return list of point coordinates and fully connected cost matrix
     */
    static public PointCoords makeUniformRect(int nPnts, Random prng) {
        assert (4 <= nPnts);
        double wFactor = 11.0;
        double hFactor = 8.5;
        double[] xs = new double[nPnts];
        double[] ys = new double[nPnts];
        
        // We want to avoid placing points too close together.
        // If they were in a grid with lines distSep apart,
        // then they would have distSep minimum separation
        // with distSep^2 area each. We choose the factor 's'
        // to satisfy (ws)*(wh) = n * d^2
        double distSep = 10.0; // arbitrary default
        double s = sqrt(nPnts / (wFactor*hFactor));// thus, (ws) * (hs) = nPnts at 1 meter^2 each.
        double xRange = wFactor * s;
        double yRange = hFactor * s;

        // With N points in 'area', they get d^2 each.
        // If that were a regular grid, they would be 'd' apart in x and in y,
        // as well as 1.414*d along diagonals.
        // so d/2 is a reasonable minimum separation.
        double area = xRange * yRange;
        double dist = sqrt(area/nPnts);
        double minD2 = (dist*dist)/4.0;
        int iter=0;
        int numXY = 0;
        while (numXY < nPnts) {
            iter++;
            //System.out.printf("Iteration %5d, size %3d \n", iter, xs.size());
            double x0 = prng.nextDouble() - 0.25;  // [-0.25 , +0.75]
            double x1 =xRange * x0;
            double y0 = prng.nextDouble() - 0.25;  // [-0.25 , +0.75]
            double y1 = yRange * y0;
            if (0 == numXY) {
                xs[numXY] = x1;
                ys[numXY] = y1;
                numXY++;
            }
            else {
                double sep = Double.MAX_VALUE;
                boolean tooClose = false;
                for (int i=0; (!tooClose) && (i<numXY); i++) {
                    double dx = x1 - xs[i];
                    double dy = y1 - ys[i];
                    if ((dx*dx)+(dy*dy) < minD2) {
                        tooClose = true;
                    }
                }
                if (!tooClose){
                    xs[numXY] = x1;
                    ys[numXY] = y1;
                    numXY++;
                }
            }

        }
        System.out.printf("Iteration %5d, size %3d \n", iter, numXY);

        PointCoords pc = new PointCoords(nPnts, xs, ys);
        return pc;
    }

    static public PointCoords makeTetrahedon(int N, int M, boolean modifiedP, Random prng) {
        int initSize = 3 * N + 2 * M;
        List<Double> xs = new ArrayList<>(initSize);
        List<Double> ys = new ArrayList<>(initSize);

        double topX = N / 2.0;
        double topY = sqrt(3.0) * topX;

        // bottom edge
        for (int i = 0; i < N; i++) {
            xs.add(Double.valueOf(i)); // OK in Java 8 and 17
            //xs.add(new Double(i)); // deprecated in Java 17
            ys.add(0.0);
        }

        // right edge
        for (int i = 0; i < N; i++) {
            double theta = ((double) (N - i)) / ((double) N);
            double x = N * theta + (topX * (1.0 - theta));
            double y = topY * (1.0 - theta);
            xs.add(x);
            ys.add(y);
        }

        // left edge
        for (int i = 0; i < N; i++) {
            double theta = ((double) (N - i)) / ((double) N);
            double x = topX * theta;
            double y = topY * theta;
            xs.add(x);
            ys.add(y);

        }

        double gamma = N / (sqrt(3.0) * M);
        double distThresh = max(10.0, 4.0 * (1.0 + gamma));

        // center
        double xc = N / 2.0;
        double yc = topY / 3.0;

        xs.add(xc);
        ys.add(yc);

        int countLL = 0;
        int countLR = 0;
        int countTC = 0;
        for (int i = 1; i < M; i++) {
            double x, y, dx, dy, d;
            double theta = ((double) (M - i)) / ((double) M);
            // lower left
            x = xc * (1.0 - theta);
            y = yc * (1.0 - theta);
            dx = x - 0.0;
            dy = y - 0.0;
            d = sqrt((dx * dx) + (dy * dy));
            if (!modifiedP || (distThresh < d)) {
                xs.add(x);
                ys.add(y);
                countLL++;
            }

            // lower right
            x = (N * theta) + (xc * (1.0 - theta));
            y = yc * (1.0 - theta);
            dx = x - N;
            dy = y - 0.0;
            d = sqrt((dx * dx) + (dy * dy));
            if (!modifiedP || (distThresh < d)) {
                xs.add(x);
                ys.add(y);
                countLR++;
            }

            // top center
            x = xc;
            y = (topY * theta) + (yc * (1.0 - theta));
            dx = x - topX;
            dy = y - topY;
            d = sqrt((dx * dx) + (dy * dy));
            if (!modifiedP || (distThresh < d)) {
                xs.add(x);
                ys.add(y);
                countTC++;
            }
        }
        System.out.printf("LL: %d \n", countLL);
        System.out.printf("LR: %d \n", countLR);
        System.out.printf("TC: %d \n", countTC);
        int nPnts = xs.size();

        if (null != prng) {
            System.out.println("Shuffling tetrahedron points");
            List< Tuple2<Double, Double>> pnts = new ArrayList<>(nPnts);
            for (int i = 0; i < nPnts; i++) {
                Tuple2<Double, Double> pr = new Tuple2<>(xs.get(i), ys.get(i));
                pnts.add(pr);
            }
            pnts = shuffle(pnts, prng);
            xs = new ArrayList<>(nPnts);
            ys = new ArrayList<>(nPnts);
            for (int i=0; i<nPnts; i++){
                xs.add(pnts.get(i).get0());
                ys.add(pnts.get(i).get1());
            }
        }

        // TODO: change above code to build double[] and avoid conversion here
        double[] xs2 = new double[nPnts];
        double[] ys2 = new double[nPnts];
        for (int i=0; i<nPnts; i++) {
            xs2[i] = xs.get(i);
            ys2[i] = ys.get(i);
        }
        PointCoords pc = new PointCoords(nPnts, xs2, ys2);
        return pc;
    }

    /**
     * Place points on a grid, with controllable noise. Calculate cost matrix
     * assuming every node is connected to every node.
     *
     * @param nRows number of rows in the grid
     * @param nClms number of columns in the grid
     * @param shuffleP should they be shuffled?
     * @param noise random variability around each grid point
     * @param prng pseudo random number generator
     * @return list of point coordinates and fully connected cost matrix
     */
    static public PointCoords makeNoisyGrid(int nRows, int nClms,
            boolean shuffleP, double noise, Random prng) {
        assert (2 <= nRows);
        assert (2 <= nClms);
        int nPnts = nRows * nClms;
        List<Integer> indices = new ArrayList<>(nPnts);
        for (int i = 0; i < nPnts; i++) {
            indices.add(i);
        }
        if (shuffleP) {
            indices = shuffle(indices, prng);
        }
        List<Double> xs = new ArrayList<>(nPnts);
        List<Double> ys = new ArrayList<>(nPnts);

        // with matrix (nr, nc), we encode the usual way,
        // then add a small amount of wiggle to
        // the otherwise-integer locations
        assert ((0 <= noise) && (noise < 0.5));
        for (int k = 0; k < nPnts; k++) {
            int n = indices.get(k); // potentially shuffled
            Tuple2<Integer, Integer> ijPair = rcFromN(n, nRows, nClms);
            int i = ijPair.get0();
            int j = ijPair.get1();
            double x = j + (noise * prng.nextDouble());
            double y = i + (noise * prng.nextDouble());
            //System.out.printf("%2d , %2d , %5.2f , %5.2f \n", j, i, x, y);
            xs.add(x);
            ys.add(y);
        }

        // TODO: change above code to build double[] and avoid conversion here
        double[] xs2 = new double[nPnts];
        double[] ys2 = new double[nPnts];
        for (int i=0; i<nPnts; i++) {
            xs2[i] = xs.get(i);
            ys2[i] = ys.get(i);
        }
        PointCoords pc = new PointCoords(nPnts, xs2, ys2);
        return pc;
    }

    static public int nFromRC(int r, int c, int nRows, int nClms) {
        return (c + (r * nClms));
    }

    static public Tuple2<Integer, Integer> rcFromN(int n, int nRows, int nClms) {
        int c = iMod(n, nClms);
        int r = (n - c) / nClms;
        Tuple2<Integer, Integer> rslt = new Tuple2<>(r, c);
        return rslt;
    }

    /**
     * Place points in a ring, not quite uniformly. Ensure we have at least one
     * point in each octant. Calculate cost matrix assuming every node is
     * connected to every node.
     *
     * @param nPnts number of points to generate
     * @param rMin minimum distance to the origin
     * @param rMax maximum distance to the origin
     * @param prng pseudo random number generator
     * @return list of point coordinates and fully connected cost matrix
     */
    static public PointCoords makeRing(int nPnts, double rMin, double rMax, Random prng) {
        List<Double> xs = new ArrayList<>(nPnts);
        List<Double> ys = new ArrayList<>(nPnts);

        double minSeparation = (rMax - rMin) / 3.0;

        // Eight cardinal points at outer edge in this order:
        // 0: SW
        // 1: West
        // 2: NW
        // 3: South
        // 4: North
        // 5: SE
        // 6: East
        // 7: NE
        // So a clockwise circle might be
        // (4,7), (7,6), (6,5), (5,3), (3,0), (0,1), (1,2), (2, 4)
        //
        for (int ix = -1; ix <= +1; ix++) {
            for (int iy = -1; iy <= +1; iy++) {
                if ((0 != ix) || (0 != iy)) {
                    double s = sqrt((ix * ix) + (iy * iy));

                    double x = (ix * rMax) / s;
                    double y = (iy * rMax) / s;
                    xs.add(x);
                    ys.add(y);
                }
            }
        }
        int trialCounter = 0;
        int tCounter = 0;
        // we already have eight points
        for (int k = 8; xs.size() < nPnts; k++) {
            // uniform in a square
            double x = prng.nextDouble() - 0.5;
            double y = prng.nextDouble() - 0.5;
            if (0 == (k % 2)) {
                // rotate 45 degrees CCW: uniform in a diamond
                double x2 = (x - y);
                double y2 = (x + y);
                x = x2;
                y = y2;
            }
            double s = sqrt((x * x) + (y * y));
            double r = rMin + (rMax - rMin) * prng.nextDouble();

            x = (x * r) / s;
            y = (y * r) / s;
            double dSquared = Double.POSITIVE_INFINITY;
            for (int n = 0; n < xs.size(); n++) {
                double dx = x - xs.get(n);
                double dy = y - ys.get(n);
                double d2 = (dx * dx) + (dy * dy);
                dSquared = min(d2, dSquared);
            }
            tCounter++;
            trialCounter++;
            if (nPnts < tCounter) {
                minSeparation = 0.618034 * minSeparation;
                tCounter = 0;
            }
            if (minSeparation < sqrt(dSquared)) {
                xs.add(x);
                ys.add(y);
            }
        }

        // TODO: change above code to build double[] and avoid conversion here
        double[] xs2 = new double[nPnts];
        double[] ys2 = new double[nPnts];
        for (int i=0; i<nPnts; i++) {
            xs2[i] = xs.get(i);
            ys2[i] = ys.get(i);
        }
        PointCoords pc = new PointCoords(nPnts, xs2, ys2);
        return pc;
    }

    /**
     * Very simple reading of a file into a String. Returns NULL if failure
     *
     * @param path String containing the path to the file
     * @return String of complete file contents, or NULL if failure.
     */
    public static String simpleReadFile(String path) {
        String rslt = null;
        BufferedReader bfro;
        try {
            bfro = new BufferedReader(
                    new FileReader(path));

            String st;
            while ((st = bfro.readLine()) != null) {
                if (null == rslt) {
                    rslt = st;
                } else {
                    rslt = rslt + st + "\n";
                }
            }

        } catch (IOException ex) { // FileNotFoundException is subclass
            //Logger.getLogger(J9.class.getName()).log(Level.SEVERE, null, ex);
            System.err.printf("File path '%s' could not be read.\n", path);
            rslt = null; // not even ""
        }

        return rslt;
    }

    /**
     * Get a path string with proper escape characters for Windows
     *
     * @return string for path
     */
    public static String usableCurrDirPath() {
        String ud1 = System.getProperty("user.dir");
        // something like  'C:\home\bwise\GWGL\stalingrad_2025_src'
        // This is unusable because '\' is an escape character
        //System.out.printf("UD1: %s\n", ud1);

        String ud1b = ud1.replace("\\", "\\\\"); // replace "\" with "\\", with proper escape characters
        // something like 'C:\\home\\bwise\\GWGL\\stalingrad_2025_src'

        return ud1b;
    }

    /**
     * A basic class holding a List of 2D points.
     * It is for testing only; use RealVector for production.
     */
    static public class PointCoords {

        public PointCoords(int nPoints, double[] xs, double[] ys) {
            this.nPoints = nPoints;
            this.xs = new double[nPoints] ; // xs;
            this.ys = new double[nPoints] ; // ys;
            for (int i = 0; i < nPoints; i++) {
                this.xs[i] = xs[i];
                this.ys[i] = ys[i];
            }
        }
        public int nPoints = 0;
        public double[] xs;
        public double[] ys;


        /**
         * Interface to return directed 'cost' from node #i to #j.
         * Could be overridden by things like A*, Floyd-Warshall, etc.
         * @param i index of start node
         * @param j index of end node
         * @return directed cost
         */
        public double cost(int i, int j){
            return eucCost(i,j);
        }

        /**
         * Return straight-line distance between node #i and node #j
         * in the given point-set
         * @param i index of start node
         * @param j index of end node
         * @return straight-line cost
         */
        public double eucCost(int i, int j){
            double dx = abs(xs[i] - xs[j]);
            double dy = abs(ys[i] - ys[j]);
            return sqrt((dx*dx)+(dy*dy));
            //return (max(dx, dy)+min(dx,dy)/3.0);
        }
    }

    // Very generic utilities below this line


    /**
     * Return standard math remainder r, where 0 <= r < m,
     * and n = r + (k*m) for some k
     *
     * @param n number to be reduced
     * @param m modulus
     * @return non-negative remainder
     */
    static public int iMod(int n, int m) {
        if (m <= 0) {
            throw new IllegalArgumentException("Modulus must be positive: " + m);
        }
        int r = n % m; // just remainder after division
        while (r < 0) {
            r = r + m;
        }
        return r;
    }

    /**
     * Round x to a multiple of f
     *
     * @param x number to be rounded
     * @param f multiplier to use
     * @return x rounded to a multiple of f
     */
    static public int roundFactor(double x, int f) {
        int r = f * ((int) (0.5 + (x / ((double) f))));
        return r;
    }

    /**
     * Symmetric fractional difference is calibrated so that symFracDiff(100,
     * 101) is about 0.01, i.e. 1%
     *
     * @param x a real number
     * @param y a real number
     * @return symmetric fractional difference IFF at least one input is not
     * zero
     */
    public static double symFracDiff(double x, double y) {
        double num = 2.0 * abs(x - y);
        double dnm = abs(x) + abs(y);
        return num / dnm;
    }

    static public final int DefaultSeedPRNG = 27185305;

    static public Random makePRNG(int sd, Boolean verbose) {
        sd = makeSeed(sd);

        if (verbose) {
            System.out.printf("Using PRNG seed %9d \n", sd);
        }
        Random prng = new Random();
        prng.setSeed(sd);
        return prng;
    }

    static public int makeSeed(int sd) {
        if (0 == sd) {
            final int m = 1000 * 1000 * 1000;
            Random prng0 = new Random(); // seeded with a value 'very likely to be distinct'
            while (0 == sd) {
                sd = abs(prng0.nextInt());
                sd = iMod(sd, m);
            }
        }
        else {
            sd = abs(sd);
        }
        return sd;
    }

    static public double roundN(double x, int n){
        double y = 0.0;
        if (0.0 < x) {
            double f = Math.pow(10, n);
            y = Math.round(f*x)/f;
        }
        if (x < 0.0) {
            y = -roundN(-x, n);
        }
        return y;
    }

    static public Random makePRNG(int sd, ReportingLevel rl) {
        sd = makeSeed(sd);

        if (rLevelLE(Medium, rl)) {
            System.out.printf("Using PRNG seed %09d \n", sd);
        }
        Random prng = new Random();
        prng.setSeed(sd);
        return prng;
    }

    /**
     * Non-destructively shuffle a List of things using Fisher-Yates algorithm, as presented by
     * Knuth as "Algorithm P (Shuffling)".
     *
     * @param lstA List<T> to be shuffled
     * @param prng source of randomness
     * @param <T> the type of thing to be shuffled
     * @return a new, shuffled List<T>
     */
    static public <T> List<T> shuffle(List<T> lstA, Random prng) {
        int n = lstA.size();
        List<T> lstB = new ArrayList<>(n);
        for (int i = 0; i < n; i++) {
            lstB.add(lstA.get(i));
        }
        for (int i = 0; i < n - 1; i++) {
            int j = i + prng.nextInt(n - i); // i <= j <= n-1
            T ei = lstB.get(i);
            T ej = lstB.get(j);
            lstB.set(i, ej);
            lstB.set(j, ei);
        }
        return lstB;
    }

    /**
     * Generic for 2-tuple.
     * With Java 17 (Sept 2021), use a Record instead.
     *
     * @param <A>
     * @param <B>
     */
    static public class Tuple2<A, B> implements Tuple {

        private final A first;
        private final B second;

        public Tuple2(A first, B second) {
            this.first = first;
            this.second = second;
        }

        public A get0() {
            return first;
        }
        public B get1() {
            return second;
        }

        @Override
        public int getSize() {
            return 2;
        }
    }

    /**
     * Generic for 3-tuple.
     * With Java 17 (Sept 2021), use a Record instead.
     *
     * @param <A>
     * @param <B>
     * @param <C>
     */
    static public class Tuple3<A, B, C> implements Tuple {

        private final A first;
        private final B second;
        private final C third;

        public Tuple3(A first, B second, C third) {
            this.first = first;
            this.second = second;
            this.third = third;
        }

        public A get0() {
            return first;
        }
        public B get1() {
            return second;
        }
        public C get2() {
            return third;
        }

        @Override
        public int getSize() {
            return 3;
        }
    }

    /**
     * Generic for 4-tuple.
     * With Java 17 (Sept 2021), use a Record instead.
     *
     * @param <A>
     * @param <B>
     * @param <C>
     * @param <D>
     */
    static public class Tuple4<A, B, C, D> implements Tuple {

        private final A first;
        private final B second;
        private final C third;
        private final D fourth;

        public Tuple4(A first, B second, C third, D fourth) {
            this.first = first;
            this.second = second;
            this.third = third;
            this.fourth = fourth;
        }

        public A get0() {
            return first;
        }
        public B get1() {
            return second;
        }
        public C get2() {
            return third;
        }
        public D get3() {
            return fourth;
        }

        @Override
        public int getSize() {
            return 4;
        }
    }

    /**
     * Generic for 5-tuple.
     * With Java 17 (Sept 2021), use a Record instead.
     *
     * @param <A>
     * @param <B>
     * @param <C>
     * @param <D>
     * @param <E>
     */
    static public class Tuple5<A, B, C, D, E> implements Tuple {

        private final A first;
        private final B second;
        private final C third;
        private final D fourth;
        private final E fifth;

        public Tuple5(A first, B second, C third, D fourth, E fifth) {
            this.first = first;
            this.second = second;
            this.third = third;
            this.fourth = fourth;
            this.fifth = fifth;
        }

        public A get0() {
            return first;
        }
        public B get1() {
            return second;
        }
        public C get2() {
            return third;
        }
        public D get3() {
            return fourth;
        }
        public E get4() {
            return fifth;
        }

        @Override
        public int getSize() {
            return 5;
        }
    }

    public static interface Tuple {
        public int getSize();
    }
}


// =============================================================================
