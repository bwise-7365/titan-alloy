// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import groupw.DCVRP.Transport;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;

import static groupw.DCVRP.VRController.TheVRC;
import static groupw.LogAdj.TEntity.State.*;
import static groupw.Network.NWUtils.shuffle;

public class KEntity extends LEntity {
    public KEntity(LogisticalAdjudicator adj) {
        super(adj);
        scheduleAt(SEARCH_INTERVAL);
    }


    // ---- DES entry point ----

    @Override
    public void process() {
        double now = mySim.getCurrTime();

        //System.out.println("DEBUG KEntity.searching(): " + status()  + " at time " + String.format("%.4f", now));
        attackTargets();

        // add random delays to search interval.
        double dt = (1.0 + (searchTimeRange * mySim.prng.nextDouble())) * SEARCH_INTERVAL;
        scheduleAt(now + dt);
    }

    private void attackTargets() {
        double now = mySim.getCurrTime();
        if (null == targets) {
            buildTargetList();
        }
        //System.out.printf("%s attacking targets at time %.4f\n", status(), now);
        if (mySim.prng.nextDouble() <pHit) {
            TEntity tgt = hitTarget();
            if (null != tgt) {
                numHits++;
                //System.out.printf("%s was hit by %s at time %.4f (%d)\n", tgt.status(), this.status(), now, numHits);
                //System.out.flush();
            }
            else {
            //    System.out.printf("%s has no targets remaining at time %.4f\n", this.status(), now, numHits);
            }
        }
        else {
        //    System.out.printf("%s missed at time %.4f\n", status(), now);
        }
    }

    /**
     * Randomly target 10% of MV22, 10% of the rest, and set random order.
     *
     */
    private void buildTargetList() {
        List<TEntity> targetMV22 = new ArrayList<>();
        List<TEntity> targetOthers = new ArrayList<>();
        for (TEntity te : myAdj.listTEntity()) {
            if (te.myTrans.name.startsWith("MV22-")) {// Skip the many MV22's for now
                targetMV22.add(te);
            } else {
                targetOthers.add(te);
            }
        }

        targetMV22 = selectTargets(targetMV22);
        targetOthers = selectTargets(targetOthers);

        targets = new ArrayList<>();
        targets.addAll(targetMV22);
        targets.addAll(targetOthers);
        targets = shuffle(targets, mySim.prng);

        double numTargets = targets.size();
        double numCycles = 1300.0/((1.0 + searchTimeRange)*SEARCH_INTERVAL); // I know it is about 1300 seconds
        pHit = (5.0 * numTargets) / numCycles; // try to hit them early
    }

    private List<TEntity> selectTargets(List<TEntity> targets) {
        int nHits = (int)(0.5 + (targets.size() * hitFrac));
        List<TEntity> shuffled = shuffle(targets, mySim.prng);
        return shuffled.subList(0, nHits);
    }

    private TEntity hitTarget() {
        TEntity tgt = null;
        for (int i = 0; ((i<targets.size()) && (null == tgt)); i++) {
            TEntity v = targets.get(i);
            if (v.myTrans.isAliveP() && ((Moving == v.state) || (Loading == v.state) || (Unloading == v.state))) {
                v.myTrans.die(); // all its Serials as well
                tgt = v;
            }
        }
      return tgt;
    }

    @Override
    protected void setLoc(String name) {
        // ignore
    }

    @Override
    public String getCurrLoc() {
        return "----";
    }

    @Override
    String status() {
        return String.format("KEntity %d", myID);
    }


    List<TEntity> targets = null;
    double hitFrac = 0.15; // we plan to hit this fraction of the MV22 and of the non-MV22's
    double pHit = 0.0; // probability of hitting a target in one cycle
    int numHits = 0;
    double searchTimeRange = 0.33;// value of 0.10 == between 0% and 10% random increase in search interval
    double SEARCH_INTERVAL = 0.75; // slightly less than 45 minutes

}

// Copyright Group W, SPA. All Rights Reserved.
