import java.time.Instant;

public class Main {
    public static void main(String[] args) {
        System.out.printf("Hello and welcome!");


        long time1 = Instant.now().toEpochMilli();
        BDFP bdfp = new BDFP();

        long seed =System.currentTimeMillis(); // 654321
        bdfp.initRANDOM(50, 20, 40, seed);
        // 50, 20, 40
        // 100, 50, 75

        // NOTE: This is how you make a greedy flow plan
        bdfp.makeGreedyFP();


        long time2 = Instant.now().toEpochMilli();
        System.out.printf("Completed greedy plan in %.4f seconds\n", (double)(time2 - time1) / (double)1000.0F);

        boolean OK1 = bdfp.checkPlan();
        System.out.printf("Greedy Flow Plan:\n");
        bdfp.showPlanCompact();

        // NOTE: This is how you improve a flow plan
        bdfp.runSwap(true);


        long time3 = Instant.now().toEpochMilli();
        System.out.printf("Performed swaps in %.2f seconds\n", (double)(time3 - time2) / (double)1000.0F);


        boolean OK2 = bdfp.checkPlan();
        //bdfp.showPlanCompact();

        System.out.printf("End of demo.\n");
        System.out.flush();
    }
}

// end of file
