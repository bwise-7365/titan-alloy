/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Network;

import junit.framework.TestCase;
import org.junit.Test;

import java.io.File;
import java.util.List;

import groupw.Network.NWUtils.Tuple2;
import groupw.Network.NWUtils.PointCoords;

public class ReadMLTest extends TestCase {

    @Test
    public void testReadML() {
        System.out.printf("Testing Approx TSP-23 on reduced Mona Lisa. \n");
        long startTime = System.currentTimeMillis();
        // the currentDir when running a test in IntelliJ is the root directory of the tests.
        // Z.B. for the SimpleIADS tests the directory is "C:\home\bwise\GWGL\desim\DESim\test\",
        // not C:\home\bwise\GWGL\desim\DESim\test\groupw\SimpleIADS"
        String currentDir = System.getProperty("user.dir") + File.separator + "test" + File.separator;
        System.out.printf("Current directory: >%s< \n", currentDir);
        String ifName = "mona-lisa100K.csv";
        String ofName = "mona-lisa-reduced-tsp.csv";
        Tuple2<List<Double>, List<Double>>  rslt = ReadML.readCSV(currentDir, ifName, ofName);
        List<Double> xs = rslt.get0();
        List<Double> ys = rslt.get1();
        int numPoints = xs.size();
        double[] x2s = new double[numPoints];
        double[] y2s = new double[numPoints];
        for (int i=0; i<numPoints; i++) {
            x2s[i] = xs.get(i);
            y2s[i] = ys.get(i);
        }


        ApproxTSP bt0 = new ApproxTSP(numPoints);
        PointCoords pcs = new PointCoords(numPoints, x2s, y2s);

        ApproxTSP23 et0 = new ApproxTSP23(bt0.extendTour());
        double c0 = et0.costETour(pcs);
        System.out.printf("Initial random cost: %.2f \n", c0);
        System.out.flush();

        boolean verbose = true;
        int maxIter = 500; // never yet seen
        ApproxTSP23.ImprvTour it = ApproxTSP23.improve23(et0, pcs, verbose, maxIter);
        //boolean improved = it.improved; // never improved on the last iteration
        ApproxTSP23 et1 = (ApproxTSP23) it.newTour;
        double c1 = it.newCost;
        double s = c0 - c1;
        System.out.printf("Initial random cost: %.2f \n", c0);
        System.out.printf("Final semi-optimized cost: %.2f \n", c1);
        System.out.printf("Improved by %.2f \n", s);
        et1.saveWholeTour(currentDir, ofName, pcs);

        double dMax = et1.highestEdgeCost(pcs);
        System.out.printf("Longest edge has length %.6f \n", dMax);

        long endTime = System.currentTimeMillis();
        System.out.printf("All times are in milliseconds since Unix epoch.\n");
        System.out.println("Start time:   " + startTime);
        System.out.println("End time:     " + endTime);
        System.out.println("Elapsed time: " + (endTime-startTime));

    }
}

// =============================================================================
