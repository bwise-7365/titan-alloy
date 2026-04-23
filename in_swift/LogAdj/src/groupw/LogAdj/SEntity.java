// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import groupw.BaseSim.Entity;
import groupw.BaseSim.Scheduler;

public class SEntity extends Entity {

    public enum State { Select, Wait, Move }

    public SEntity(Scheduler s) {
        super(s);
        state = State.Select;
    }

    @Override
    public void process() {
        switch (state) {
            case Select: doSelect(); break;
            case Wait:   doWait();   break;
            case Move:   doMove();   break;
        }
    }

    public void doSelect() {
        System.out.printf("SEntity %d  t=%.4f  state=%s\n", getID(), mySim.getCurrTime(), state);
    }

    public void doWait() {
        System.out.printf("SEntity %d  t=%.4f  state=%s\n", getID(), mySim.getCurrTime(), state);
    }

    public void doMove() {
        System.out.printf("SEntity %d  t=%.4f  state=%s\n", getID(), mySim.getCurrTime(), state);
    }

    public State state;
}
// Copyright Group W, SPA. All Rights Reserved.
