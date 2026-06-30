



public class Main {
    public static void main(String[] args) {
        System.out.printf("Hello and welcome!");

        BDFP bdfp = new BDFP();

        long seed =System.currentTimeMillis(); // 654321
        bdfp.initRANDOM(10, 7, 20, seed);

        // NOTE: This is how you make a greedy flow plan
        bdfp.makeGreedyFP();

        boolean OK1 = bdfp.checkPlan();
        System.out.printf("Greedy Flow Plan:\n");
        bdfp.showPlanCompact();

        // NOTE: This is how you improve a flow plan
        bdfp.runSwap(true);


        boolean OK2 = bdfp.checkPlan();
        //bdfp.showPlanCompact();

        System.out.printf("End of demo.\n");
        System.out.flush();
    }
}

// end of file
