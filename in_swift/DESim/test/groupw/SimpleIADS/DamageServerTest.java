/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.SimpleIADS;

import static groupw.Network.NWUtils.DefaultSeedPRNG;
import groupw.BaseSim.Scheduler;
import static groupw.SimpleIADS.SimpleAirFrame.makeSimpleFWA;
import static groupw.SimpleIADS.SimpleMissile.makeSimpleMissile;
import static groupw.BaseSim.DSUtils.KILOMETER;
import static groupw.BaseSim.DSUtils.makeUnitBoxRandomRV;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static groupw.Network.NWUtils.ReportingLevel.Medium;
import static groupw.Network.NWUtils.rLevelLE;

import org.apache.commons.math4.legacy.linear.RealVector;
import org.junit.Test;
import static org.junit.Assert.*;

/**
 *
 * @author bwise
 */
public class DamageServerTest {

    public DamageServerTest() {
    }

    /**
     * Test of applyDetonationDamage method, of class DamageServer.
     * Creates several dense crowds of air vehicles, sets off a detonation
     * in each, and assesses damage against them.
     */
    @Test
    public void test00() {
        System.out.println("\n\nStarting DamageServerTest.test00");
        int numDim = 3;
        int sd0 = DefaultSeedPRNG;
        //sd0 = 0;

        // because there may be more Test function run soon, these be cleared
        Scheduler sim = new Scheduler();
        DamageServer.makeDS(sim);


        sim.setPRNG(sd0);
        EntityData.parameterizeDamageServer();
        DamageServer.theDS.stochasticP = false;
        DamageServer.theDS.rLevel = High;
        DamageServer.theDS.rLevel = Medium;
        //DamageServer.theDS.rLevel = Silent;

        double tNow = sim.getCurrTime();
        int numMissiles = 8;
        int numFWA = 6;
        int nObj = 0;
        int missileCount = 0;
        int fwaCount = 0;

        // Notice that we mix Blue and Red missiles with
        // Blue and Red FWA to illustrate fratricide
        for (int i = 0; i < numMissiles; i++) {
            RealVector c1 = makeUnitBoxRandomRV(numDim, sim.prng);
            c1 = c1.mapMultiply(50.0 * KILOMETER); // rescale [-0.5, +0.5] to [-25, +25] Km

            for (int j = 0; j < numFWA; j++) {
                RealVector dv = makeUnitBoxRandomRV(numDim, sim.prng);
                dv = dv.mapMultiply(30.0); // rescale [-0.5, +0.5] to [-15, +15] meters
                RealVector c2 = dv.add(c1);

                String fwaType = "";
                switch (fwaCount % 4) {
                    case 0:
                        fwaType = EntityData.fwaBlue01;
                        break;
                    case 1:
                        fwaType = EntityData.fwaBlue02;
                        break;
                    case 2:
                        fwaType = EntityData.fwaRed01;
                        break;
                    case 3:
                        fwaType = EntityData.fwaRed02;
                        break;
                }
                SimpleAirFrame sf1 = makeSimpleFWA(fwaType, numDim, sim);
                sf1.setLastPosTime(c2, tNow);
                fwaCount++;
            }
            String missileType = "";
            switch (missileCount % 4) {
                case 0:
                    missileType = EntityData.a2aMissileBlue01;
                    break;
                case 1:
                    missileType = EntityData.a2aMissileBlue02;
                    break;
                case 2:
                    missileType = EntityData.a2aMissileRed01;
                    break;
                case 3:
                    missileType = EntityData.a2aMissileRed02;
                    break;
            }
            SimpleMissile sm1 = makeSimpleMissile(missileType, numDim, sim);
            sm1.setLastPosTime(c1, tNow);
            missileCount++;
            SimpleDetonation sDet1 = sm1.makeDetonation();

            sDet1.process();

            nObj = sim.getNumEntities();
            System.out.printf("There are now %d/%d objects\n", nObj, fwaCount);
            if (rLevelLE(Medium, DamageServer.theDS.rLevel)) {
                System.out.printf("\n");
            }
            System.out.flush();
        }

        // I happen to know that 29 of 48 FWA should remain after 8 missiles.
        // They are so crowded that one A2A missile typically hits several FWA.
        assertEquals(29, nObj);
        assertEquals(48, fwaCount);
        assertEquals(8, missileCount);

        DamageServer.clearDS();
        sim = null;
    }

}
// =============================================================================
