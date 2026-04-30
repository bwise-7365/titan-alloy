/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.BaseSim;



import groupw.Network.NWUtils.ReportingLevel;
import java.util.PriorityQueue;
import java.util.Comparator;
import java.util.Random;
import java.util.HashMap;
import java.util.Map;
import java.util.ArrayList;
import org.apache.commons.math4.legacy.linear.RealVector;

import static groupw.Network.NWUtils.ReportingLevel.Silent;
import static groupw.Network.NWUtils.ReportingLevel.Low;
import static groupw.Network.NWUtils.ReportingLevel.Medium;
import static groupw.Network.NWUtils.ReportingLevel.High;
import static groupw.Network.NWUtils.makePRNG;
import static groupw.Network.NWUtils.rLevelLE;

/**
 * The Scheduler manages a priority queue of events and tracks the associated persistent entities.
 * There may be multiple Schedulers running in parallel, so each is entirely self-contained.
 * All times are in seconds, distances in meters, weights in kilograms, volumes in liters.
 *
 * @author BenWise
 */
public class Scheduler {

    /**
     * The basic class to manage the discrete event queue.
     */
    public Scheduler() {
        Comparator<Event> comparator = new EventComparator();
        eventQueue = new PriorityQueue<>(10, comparator);
        ItemRegistry.initialize();
        hulls = new HashMap<>();
    }

    /**
     * Insert the event into the event queue with earliest time first. In
     * case of exactly tied times, the lower unique ID number is considered
     * earlier.
     * The name 'addEvent' is semantically redundant, but it makes
     * text-search easier than the more ambiguous 'add'.
     *
     * @param e Event to be inserted into the event queue
     */
    public void addEvent(Event e) { // note that processing time must be set before adding it to the evntQueue
        eventQueue.add(e);
    }


    /**
     * Store a hull so that it can be retrieved by ID number.
     * It should have auto-registered as a CountedItem
     *
     * @param h The hull to be recorded
     */
    public void registerHull(Hull h) {
        hulls.put(h.getID(), h);
    }

    public void deregisterHull(Hull h) {
        h.setActive(false);
        hulls.remove(h.getID());
    }

    /**
     * Eliminate with extreme prejudice.
     *
     * @param h
     */
    public void eliminateHull(Hull h) {
        deregisterHull(h);
        deregisterEntity(h.getID());
    }

    /**
     * Given an integer ID, retrieve the corresponding registered Entity (dead
     * or alive)
     *
     * @param ID integer ID of an Entity
     * @return the corresponding Entity
     */
    public Entity getEnt(long ID) {
        Entity ent = (Entity)ItemRegistry.getItem(ID);
        if (null == ent) {
            System.out.printf("Entity %d was not found\n", ID);
        }
        return ent;
    }

    public int getNumEntities() {
        return ItemRegistry.numItems();
    }

    /**
     * Given an integer ID, retrieve the corresponding registered Entity (dead
     * or alive)
     *
     * @param ID integer ID of an Entity
     * @return the corresponding Entity
     */
    public Hull getHull(long ID) {
        Hull h = (Hull) (ItemRegistry.getItem(ID));
        if (null == h) {
            System.out.printf("Hull %d was not found\n", ID);
        }
        return h;
    }

    /**
     * Given an integer ID, remove that Entity from the entity map. If it is in
     * the hull map, remove it from there as well.
     *
     * @param ID integer ID of an Entity
     */
    public void deregisterEntity(long ID) {
        ItemRegistry.deregister(ID);
    }

    /**
     * Process events until the event queue is empty or stop() is called.
     */
    public void run() {
        continueP = true;
        while (continueP) {
            step();
        }
        if (rLevelLE(Low, schedRLevel)) {
            System.out.printf("%s  desim.Scheduler finished run(); last event at %.2f sec\n",
                    this.timeStamp(), currTime);
        }
    }

    /**
     * Process events until the specified maximum time is reached, the event
     * queue is empty or stop() is called. If events are scheduled for 1159 and
     * 1201 and the end time is 1200, only the first will be processed. If
     * events are scheduled for 1159, 1200 and 1201 and the end time is 1200,
     * only the first and second will be processed.
     *
     * @param endTime Time in seconds at which to stop
     */
    public void run(double endTime) {
        continueP = (0.0 < endTime) && (0 < eventQueue.size());
        while (continueP) {
            step(endTime);
        }
        if (rLevelLE(Low, schedRLevel)) {
            System.out.printf("%s  desim.Scheduler finished run(endTime); last event at %.2f sec\n",
                    this.timeStamp(), currTime);
            System.out.printf("There are %d events in the queue. \n",
                    queueSize());
        }
    }

    /**
     * Process one event.
     */
    public void step() {
        // If not empty, pop off the first event and process it.
        // Notice that the discrete event occurs at exactly the time
        // at which was scheduled to occur.
        continueP = (0 < eventQueue.size());
        if (continueP) {
            if (rLevelLE(High, schedRLevel)) {
                System.out.printf("%s Step() at time %.6f with %3d events \n",
                        timeStamp(), getCurrTime(), eventQueue.size());
            }
            Event e = eventQueue.peek();
            eventQueue.remove();
            double et = e.getProcTime();
            assert (currTime <= et); // no past events
            currTime = et; // guarantee that the currTime == procTime
            e.process();
        } else {
            stop();
        }
    }

    /**
     * Process one event, but not after the specified maximum time.
     *
     * @param maxTime Time in seconds at which to stop
     */
    public void step(double maxTime) {
        continueP = (getCurrTime() <= maxTime) && (0 < eventQueue.size());
        if (continueP) {
            if (rLevelLE(High, schedRLevel)) {
                System.out.printf("%s Step(%.4f) at time %.6f with %3d events \n",
                        timeStamp(), maxTime, getCurrTime(), eventQueue.size());
            }
            Event e = eventQueue.peek();
            eventQueue.remove();
            double et = e.getProcTime();
            if (et <= maxTime) {
                assert (currTime <= et);
                currTime = et;
                e.process();
            } else {
                stop();
            }
        }
    }

    /**
     * Determine whether the event evntQueue is empty.
     *
     * @return T if the event evntQueue is empty, F otherwise
     */
    public boolean empty() {
        return eventQueue.isEmpty();
    }

    public void clearEvents() {
        // Try to ease garbage collection by dereferencing
        while (0 < eventQueue.size()) {
            eventQueue.remove();
        }
    }

    public int queueSize() {
        return eventQueue.size();
    }

    /**
     * Convenience function so that Entities and Events can stop all
     * event-processing the simulation. Any currently running event code will
     * continue, but no further events will be processed.
     */
    public void stop() {
        continueP = false;
    }

    private boolean continueP = true;
    private final PriorityQueue<Event> eventQueue;
    private double currTime = 0.0;
    public ReportingLevel schedRLevel = Silent;

    /**
     * Map storing all the entities (dead or alive, physical or disembodied) in
     * the simulation. A killed tank might remain on the battlefield, but a
     * detonated bomb is totally gone.
     */
    //public Map<Integer, Entity> entities;

    /**
     * The pseudorandom number generator to be used by everything in this
     * simulation.
     */
    public Random prng = null;

    /**
     * Map storing Entities that have specific physical locations.
     * They are also CountedItems in the ItemRegistry, but
     * that includes a lot of things without location attributes.
     */
    public Map<Long, Hull> hulls;

    /**
     * Scan every hull within a given range of a point and return their entity
     * ID's. This is used by both damage models and sensor models.
     * Something based on quad-tree or oct-tree would be more efficient.
     *
     * @param c Reference point from which to search
     * @param r range, in meters, to search
     * @param t sim-time to dead reckon
     * @return Vector of Entity ID's for hulls in range
     */
    public ArrayList<Long> getHullsInRange(RealVector c, double r, double t) {
        ArrayList<Long> ids = new ArrayList<>(17);
        for (Map.Entry<Long, Hull> entry : hulls.entrySet()) {
            RealVector p = entry.getValue().drCurrPos(t);
            double dist = p.getDistance(c);
            if (dist < r) {
                ids.add(entry.getKey());
            }
        }
        return ids;
    }

    /**
     * The time of the current or most recently processed event.
     *
     * @return current time in seconds.
     */
    public double getCurrTime() {
        return currTime;
    }

    /**
     * Generate a 14-char formatted string of the days, 24-hour time, and
     * seconds to 2 decimals as DDD:HHMM:SS.SS If it passes midnight at the end
     * of day 999, it will be longer.
     *
     * @return Formatted time string
     */
    public String timeStamp() {
        return DSUtils.timeStamp(currTime);
    }

    /**
     * Use the included PRNG to generate a uniform double.
     *
     * @param vMin minimum value of returned doubles
     * @param vMax maximum value of returned doubles
     * @return an number approximately uniform between vMin and vMax
     */
    public double uniform(double vMin, double vMax) {
        assert (null != prng);
        return DSUtils.uniform(vMin, vMax, prng);
    }

    /**
     * Returns True with probability 'p'
     *
     * @param p probability between zero and one
     * @return True with probability p, False otherwise
     */
    public boolean prob(double p) {
        double pTol = 1e-10; // tolerance for round-off error
        assert (0.0 <= (p + pTol));
        assert (p <= (1.0 + pTol));
        boolean rslt;
        if (p <= 0) {
            rslt = false;
        } else if (1.0 <= p) {
            rslt = true;
        } else {
            double pt = uniform(0.0, 1.0);
            rslt = (pt <= p);
        }
        return rslt;
    }

    /**
     * Use the included PRNG to generate a negative exponential double.
     *
     * @param vMean average value of returned doubles
     * @return a negative exponential double with average value vMean
     */
    public double negExp(double vMean) {
        assert (null != prng);
        return DSUtils.negExp(vMean, prng);
    }

    /**
     * Set the pseudorandom number generator to be used, which must not be null.
     *
     * @param p pseudorandom number generator to be used.
     */
    private void setPRNG(Random p) {
        assert (null != p);
        prng = p;
    }

    /**
     * Set the pseudorandom number generator to be used.
     *
     * @param sd pseudorandom number seed to be used. Zero mean
     * non-reproducible.
     */
    public void setPRNG(int sd) {
        prng = makePRNG(sd, Medium);
    }

}

// =============================================================================
