/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.BaseSim;

import static groupw.Network.NWUtils.DefaultSeedPRNG;
import static groupw.Network.NWUtils.ReportingLevel.Medium;
import static groupw.Network.NWUtils.makePRNG;
import static java.lang.Math.abs;
import static java.lang.Math.sqrt;
import java.util.Random;
import org.apache.commons.math4.legacy.linear.RealVector;
import static org.junit.Assert.assertTrue;
import org.junit.Test;

/**
 *
 * @author bwise
 */
public class TankerPlacementTest {
    

    @Test
    public void testTankerPlacement() {
        int numDim = 2;
        int numTest = 500;
        double usingFuelDistance = 500.0 * 1000.0; // 500 km
        double distanceRatio = 0.85;

        double placementDistance = usingFuelDistance * distanceRatio;

        double distTol = 0.001;

        int sd0 = DefaultSeedPRNG;
        //sd0 = 0;
        Random prng = makePRNG(sd0, Medium);
        for (int i = 1; i <= numTest; i++) {

            // Create an air base location 2-vector
            RealVector airBaseLoc = DSUtils.makeUnitBoxRandomRV(numDim, prng);
            airBaseLoc = airBaseLoc.mapSubtract(0.25);
            airBaseLoc.unitize(); // make the norm 1.0
            airBaseLoc = airBaseLoc.mapMultiply(2 * usingFuelDistance);

            // Create a CAP location 2-vector which is more than flight range away from base
            RealVector capLoc = airBaseLoc;
            RealVector delta = airBaseLoc.subtract(capLoc); // component-wise vector subtraction
            while (delta.getNorm() < placementDistance) {

                capLoc = DSUtils.makeUnitBoxRandomRV(numDim, prng);
                capLoc = airBaseLoc.mapSubtract(0.25);
                capLoc.unitize(); // make the norm 1.0
                capLoc = airBaseLoc.mapMultiply(2 * usingFuelDistance);
                delta = airBaseLoc.subtract(capLoc);
            }

            // mirror old code
            double deltaX = delta.getEntry(0);
            double deltaY = delta.getEntry(1);
            double radians = Math.atan2(deltaY, deltaX);
            double degrees = (radians * 180.0) / Math.PI;

            double xLoc1 = (placementDistance * Math.cos(radians)) + capLoc.getEntry(0);
            double yLoc1 = (placementDistance * Math.sin(radians)) + capLoc.getEntry(1);

            // redo it without trigonometry
            double currLength = sqrt((deltaX*deltaX)+(deltaY*deltaY));
            double rescaleFactor = placementDistance / currLength;

            double xLoc2 = (rescaleFactor * deltaX) + capLoc.getEntry(0);
            double yLoc2 = (rescaleFactor * deltaY) + capLoc.getEntry(1);

            //System.out.printf("XL1: %.3f  YL1: %.3f \n", xLoc1, yLoc1);
            //System.out.printf("XL2: %.3f  YL2: %.3f \n", xLoc2, yLoc2);

            assertTrue(abs(xLoc1 - xLoc2) < distTol);
            assertTrue(abs(yLoc1 - yLoc2) < distTol);
            
            if (0 == (i%100)){
                System.out.printf(".\n");
                System.out.flush();
            }
            else {
                System.out.printf(".");
            }
        }
        System.out.println("");
    }

}

// =============================================================================
