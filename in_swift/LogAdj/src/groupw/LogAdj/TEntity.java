// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import groupw.BaseSim.Entity;
import groupw.BaseSim.Scheduler;

public class TEntity extends Entity {

    public enum State { Plan, Transfer, Move }

    public TEntity(Scheduler s) {
        super(s);
        state = State.Plan;
    }

    @Override
    public void process() {
        switch (state) {
            case Plan:     doPlan();     break;
            case Transfer: doTransfer(); break;
            case Move:     doMove();     break;
        }
    }

    public void doPlan() {
        System.out.printf("TEntity %d  t=%.4f  state=%s\n", getID(), mySim.getCurrTime(), state);
    }

    public void doTransfer() {
        System.out.printf("TEntity %d  t=%.4f  state=%s\n", getID(), mySim.getCurrTime(), state);
    }

    public void doMove() {
        System.out.printf("TEntity %d  t=%.4f  state=%s\n", getID(), mySim.getCurrTime(), state);
    }

    public State state;
}
// Copyright Group W, SPA. All Rights Reserved.
