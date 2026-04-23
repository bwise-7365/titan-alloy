/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Network;

import static groupw.Network.NWUtils.DefaultSeedPRNG;

import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

import groupw.Network.NWUtils.Tuple2;
import groupw.Network.NWUtils.PointCoords;

public class ApproxTSPFITest {


    @Test
    public void testOverall() {
        boolean verbose = true;
        int sd = DefaultSeedPRNG;
        int nRows = 25;
        int nClms = 30;
        //Tuple2<ApproxTSP, PointCoords> rslt =  ApproxTSP.ringTSP(verbose, sd);
        Tuple2<ApproxTSP, PointCoords> rslt =  ApproxTSP.gridTSP(nRows, nClms, verbose, sd);
        //Tuple2<ApproxTSP, PointCoords> rslt = ApproxTSP.rectTSP(nRows, nClms, verbose, sd);

        long startTime = System.currentTimeMillis();

        PointCoords pcs = rslt.get1();
        //double[][] dm0 = pcs.dMat;
        //List<Double> xs = pcs.xs;
        //List<Double> ys = pcs.ys;
        int nPointsTotal = pcs.nPoints; // xs.size();

        ApproxTSPFI et0 = new ApproxTSPFI();
        et0.add(0);
        et0.add(0);

        while(et0.points.size() < nPointsTotal) {
            ApproxTSPFI.extendETour(et0, nPointsTotal, pcs);
        }
        double c0 = et0.costETour(pcs);
        System.out.printf("Final semi-optimized cost over %d points: %.2f \n", et0.numPoints, c0);
        if (et0.numPoints < 5000) {
            et0.showWholeTour( pcs);
        }


        double dMax = et0.highestEdgeCost(pcs);
        System.out.printf("Longest edge has length %.6f \n", dMax);

        long endTime = System.currentTimeMillis();
        System.out.printf("All times are in milliseconds since Unix epoch.\n");
        System.out.println("Start time:   " + startTime);
        System.out.println("End time:     " + endTime);
        System.out.println("Elapsed time: " + (endTime-startTime));

    }

    //@Test
    public void testInsert() {
        List<Integer> myList = new ArrayList<>(5);
        myList.add(0);
        myList.add(1);
        myList.add(2);
        myList.add(3);
        myList.add(4);

        myList.add(2, 17);
        
        
        for (int i=0; i<myList.size(); i++){
            System.out.printf("%2d:  %3d \n", i ,myList.get(i));
        }
    }
}

// =============================================================================
