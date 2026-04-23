// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import groupw.BaseSim.Entity;
import groupw.BaseSim.Scheduler;
import groupw.DCVRP.Serial;

public class SEntity extends Entity {

    public enum State { Select, Wait, Move, Delivered }

    public SEntity(Serial sr, Scheduler s) {
        super(s);
        state = State.Select;
        mySerial = sr;
    }

    @Override
    public void process() {
        switch (state) {
            case Select: doSelect(); break;
            case Wait:   doWait();   break;
            case Move:   doMove();   break;
            case Delivered:   finish();   break;
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

    public void finish() {
        System.out.printf("SEntity %d  t=%.4f  state=%s\n", getID(), mySim.getCurrTime(), state);
    }

    Serial mySerial = null;
    public State state;
}
// Copyright Group W, SPA. All Rights Reserved.
