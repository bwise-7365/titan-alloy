// Copyright Group W, SPA. All Rights Reserved.
package groupw.DCVRP;

import org.junit.Test;

import java.util.ArrayList;
import java.util.List;
import java.util.Random;
import java.util.function.Function;

import static groupw.DCVRP.VRController.TheVRC;
import static java.lang.Math.abs;

import static groupw.DCVRP.ReadDCVRScenarioCSV.readStandardTestCase;
import static groupw.DCVRP.Backlog.Reservation;
import static groupw.Network.NWUtils.DefaultSeedPRNG;
import static groupw.Network.NWUtils.makePRNG;

/**
 *
 * @author BenWise
 */
public class BacklogTest {

    public BacklogTest() {
    }
    
    /*
    @BeforeClass
    public static void setUpClass() {
    }
    
    @AfterClass
    public static void tearDownClass() {
    }
    
    @Before
    public void setUp() {
    }
    
    @After
    public void tearDown() {
    }
    */

    @Test
    public void testBacklogTimes() {
        ReadDCVRScenarioCSV.ScenarioRecord sRec = readStandardTestCase();
        int sd = DefaultSeedPRNG;
        VRController.initialize(sRec, sd);
        ItineraryBuilder.initialize();

        String vName = "CH53-02-PLT"; // plausible capacity
        List<String> serialNames = new ArrayList<>();
        serialNames.add("serial_00");
        serialNames.add("serial_01");
        serialNames.add("serial_02");
        serialNames.add("serial_03");
        serialNames.add("serial_04");
        serialNames.add("serial_05");
        serialNames.add("serial_06");
        serialNames.add("serial_07");

        String destName = "dummyDest";
        String midName = "dummyMid";
        double sArea = 70.8;
        double sWght = 3840.0;
        double rt = 27.3;
        double deltaRT = 1.0;
        double tol = 0.001;

        Backlog bLog = new Backlog(vName);
        double time0 = bLog.hypoAverageRTT(rt + deltaRT);
        assert (abs(rt + deltaRT - time0) < tol);

        // If no real reservations in the backlog, then any hypothetical
        // reservation would go on the first trip.
        int tNum00 = bLog.hypoTripNumber(vName, sArea, sWght);
        assert (1 == tNum00);

        for (String s : serialNames) {
            Reservation r = new Reservation(vName,
                    s, destName, midName, sArea, sWght, rt);
            r.roundTripTime = r.roundTripTime + (deltaRT * TheVRC.prng.nextDouble());
            bLog.appendReservation(r);
        }

        int ndx03 = bLog.reservationIndex("serial_03");
        assert (3 == ndx03); // first is zero
        double rt03 = bLog.averageRTT(ndx03);
        assert (abs(rt03 - 27.7526) < tol);

        int tNum01 = bLog.hypoTripNumber(vName, sArea, sWght);
        assert (3 == tNum01);
        double hAT01 = bLog.hypoArrivalTime(vName, sArea, sWght, rt);
        assert (abs(hAT01 - 69.2763) < tol);

        bLog.clear();
        serialNames.add("serial_08");
        serialNames.add("serial_09");
        serialNames.add("serial_10");
        serialNames.add("serial_11");
        serialNames.add("serial_12");
        serialNames.add("serial_13");
        serialNames.add("serial_14");
        serialNames.add("serial_15");
        serialNames.add("serial_16");
        serialNames.add("serial_17");
        serialNames.add("serial_18");
        for (String s : serialNames) {
            Backlog.Reservation r = new Backlog.Reservation(vName,
                    s, destName, midName, sArea, sWght, rt);
            r.roundTripTime = r.roundTripTime + (deltaRT * TheVRC.prng.nextDouble());
            bLog.appendReservation(r);
        }
        assert (19 == bLog.numReservations());
        int tNum02 = bLog.hypoTripNumber(vName, sArea, sWght);
        assert (6 == tNum02);
        double hAT02 = bLog.hypoArrivalTime(vName, sArea, sWght, rt);
        assert (abs(hAT02 - 153.7042) < tol);

        int ndx04 = bLog.reservationIndex("serial_13");
        assert (13 == ndx04);
        int num04 = bLog.tripNumber(vName, ndx04);
        double artt04 = bLog.averageRTT(ndx04);
        double at04 = bLog.arrivalTime(vName, ndx04);
        assert (abs((num04 - 0.5) * artt04 - at04) < tol);
    }

    @FunctionalInterface
    private interface MyFI {
        int applyFN(int n);
    }

    @FunctionalInterface
    private interface MyFI2 {
        int applyFN(int n, int m);
    }

    @Test
    /**
     * This 'test' is mostly a way to experiment with lambda functions
     */
    public void testMinSearch() {

        MyFI2 bar = (int p, int q) -> {
            return (p * q);
        };

        int sd = 909037988;
        Random prng = makePRNG(sd, true);
        int numInts = 20;
        int maxVal = 25;
        List<Integer> allInts = new ArrayList<>(numInts);
        for (int i = 0; i < numInts; i++) {
            int vi = 1 + prng.nextInt(maxVal);
            allInts.add(vi);
            System.out.printf("%3d: %4d \n", i, vi);
        }

        int a = 17;
        int b = 23;
        System.out.printf("%d * %d -> %d\n", a, b, bar.applyFN(a, b));

        int k = numInts / 2; // Java 17 correctly infers 'int' type, Java 8 does not.

        // Note lambda-closure in reference to 'allInts' variable.
        // Note that it cannot use primitive 'int' types.
        Function<Integer, Integer> m1 = (Integer n) -> {
            return allInts.get(n);
        };
        int z = m1.apply(k);
        System.out.printf("%d -> %d\n", k, z);

        // more succinct lambda closure that does use primitive 'int' type
        MyFI m2 = (int n) -> {
            return allInts.get(n);
        };

        int minVal = 1 + maxVal;
        List<Integer> minIndices = new ArrayList<>(1);
        for (int i = 0; i < numInts; i++) {
            int vi = m2.applyFN(i); // m1.apply(i);
            if (vi < minVal) {
                minVal = vi;
                minIndices = new ArrayList<>(1);
                minIndices.add(i);
                System.out.printf("Reset minimum %3d: %4d \n", i, vi);
            } else if (vi == minVal) {
                minIndices.add(i);
            }
        }

        for (int j : minIndices) {
            System.out.printf("%3d ", j);
        }
        System.out.println();
    }

}

// =============================================================================
