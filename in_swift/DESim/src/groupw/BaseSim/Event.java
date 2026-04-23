/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.BaseSim;


/**
 *
 * @author BenWise
 * An instantaneous, discrete event that changes the simulation
 * state at the time of processing.
 * An event might occur on a fairly regular cycle, as in vehicle dynamics.
 * An event might be driven by other events, as in a queuing model.
 * Events as such cannot be unscheduled, but the EntEvent of an Entity will
 * do nothing if the Entity is inactive (e.g. blown up).
 */
public abstract class Event extends CountedItem {

    /**
     * Create a new Event, with unique sequential ID number.
     *
     * @param s The scheduler which will manage this event
     * @param pTime The time at which this event "occurs", i.e. when it should be processed. It must be at or after the current simulation time.
     */
    public Event(Scheduler s, double pTime) {
        super();
        mySim = s;
        assert (mySim.getCurrTime() <= pTime);
        procTime = pTime;
    }

    /**
     * Do whatever this event is supposed to do.
     */
    abstract public void process();

    /**
     * Returns the time at which this event should be processed
     *
     * @return the time at which this event should be processed
     */
    public double getProcTime() {
        return procTime;
    }

    /**
     * The time at which this event should be processed. It must be at or after the current simulation time.
     */
    final protected double procTime;
    final protected Scheduler mySim;

}


// =============================================================================
