/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package groupw.BaseSim;

import groupw.Network.NWUtils.ReportingLevel;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static groupw.Network.NWUtils.ReportingLevel.Medium;
import static groupw.Network.NWUtils.ReportingLevel.Silent;
import static groupw.Network.NWUtils.rLevelLE;
import java.util.ArrayList;

/**
 * Parameterized Finite State Machines
 *
 * @author BenWise
 */
public class PFSM
extends CountedItem
{

    public PFSM()
    {
        states = new ArrayList<State>();
        currState = null;
    }

    /**
     * Perform the basic pFSM function of state transitions. Actions can be
     * attached to each transition and to each state.
     *
     * @return
     */
    public boolean step() {
        if (rLevelLE(Medium, this.rl)) {
            System.out.println("-----");
            System.out.printf("PFSM.step %d start \n", getID());
        }
        assert (null != currState);
        boolean changedP = false;
        int numTrans = currState.tests.size();

        for (int i = 0; (!changedP) && (i < numTrans); i++) {
            PFSM.Test t = currState.tests.get(i);
            if (rLevelLE(High, this.rl)) {
                System.out.printf("PFSM %5d state %d checking test %d, %s \n",
                        getID(), currState.getID(), t.getID(), t.getName());
            }
            assert (null != t);
            boolean c = t.myCheck.checkFN(); // run the required λ-function
            if (c) {
                changedP = true;
                PFSM.State s2 = currState.nextState.get(i);
                if (rLevelLE(High, this.rl)) {
                    System.out.printf("Test %d satisfied, next state %d, %s \n",
                            t.getID(), s2.getID(), s2.getName());
                }
                if (null != t.myAction) {
                    t.myAction.actFN(); // run the optional λ-function
                }
                setCurrState(s2);

            } else if (rLevelLE(High, this.rl)) {
                System.out.printf("Test %5d not satisfied\n",
                        t.getID());
            }
        } // end of loop over i
        if (null != currState.myAction) {
            currState.myAction.actFN(); // run the optional λ-function
        }

        if (rLevelLE(Medium, this.rl)) {
            System.out.printf("PFSM.step %d completed \n", getID());
            System.out.println("-----\n");
        }
        return changedP;
    }

    protected static interface Check {
        boolean checkFN();
    }

    protected static interface Action {
        void actFN();
    }

    /**
     * PFSM.Counted is a static nested class so that we do not need an instance of PFSM to access it.
     *
     * @author BenWise
     */
    static public class Component {

        public Component(String n, PFSM f)
        {
            name = n;
            myID = ItemRegistry.nextID(); // ItemCounter.nextID();
            myPFSM = f;
        }

        /**
         * Retrieve the unique ID number of this object
         *
         * @return ID number
         */
        public final long getID()
        {
            return myID;
        }

        public String getName()
        {
            return name;
        }
        final private long myID;
        final protected String name;
        final protected PFSM myPFSM;
    }

    private ArrayList<PFSM.State> states = null;
    private PFSM.State currState = null;
    public  ReportingLevel rl = Silent;

    protected static class State extends Component {

        public State(String n, PFSM f)
        {
            super(n, f);
            tests = new ArrayList<PFSM.Test>();
            nextState = new ArrayList<PFSM.State>();
        }

        /**
         * Add a transition (test and next state) to this state.
         * The name 'addTrans' is semantically redundant, but it makes
         * text-search easier than the more ambiguous 'add'.
         * @param t the test of this transition
         * @param s the next state if the transition succeeds
         */
        public void addTrans(Test t, State s)
        {
            assert (tests.size() == nextState.size());
            tests.add(t);
            nextState.add(s);
        }

        private final ArrayList<PFSM.Test> tests;
        private final ArrayList<PFSM.State> nextState;

        // Put your lambda function here in a factory method.
        protected Action myAction = null;
    }

    protected static class Test extends Component {

        protected Test(String n, PFSM f)
        {
            super(n, f);
        }

        // Put your lambda functions here in a factory method.
        protected Check myCheck = null;
        protected Action myAction = null;
    }

    final public void setCurrState(PFSM.State s) {
        if (null == s) { // do not skip over a programming error
            throw new RuntimeException("Null state set as current in PFSM "+getID());
        }
        if (!statePresent(s)) { // do not skip over a programming error
            throw new RuntimeException("Cannot set non-present state "+s.getID()+" in PFSM "+getID());
        }
        currState = s;
    }

    /**
     * Add a state, with no transitions, to this state machine.
     * The name 'addState' is semantically redundant, but it makes
     * text-search easier than the more ambiguous 'add'.
     * @param s the state to be added
     */
    final public void addState(PFSM.State s) {
        if (null == s) { // do not skip over a programming error
            throw new RuntimeException("Null state added to PFSM "+getID());
        }
        if (statePresent(s)) { // do not skip over a programming error
            throw new RuntimeException("Duplicate state "+s.getID()+" added to PFSM "+getID());
        }
        states.add(s);
    }

    public boolean statePresent(PFSM.State s) {
        if (null == s) { // do not skip over a programming error
            throw new RuntimeException("Null state checked in PFSM "+getID());
        }
        boolean presentP = false;
        for (State s2 : states) {
            if (s.name.equals(s2.name)) {
                presentP = true;
            }
        }
        return presentP;
    }
}


// =============================================================================
