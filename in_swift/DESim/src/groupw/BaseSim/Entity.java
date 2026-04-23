/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.BaseSim;

/**
 * A generic persistent entity which creates and is affected by
 * events. It might not have a physical position, e.g. a planning algorithm that
 * needs to collect and record state information between planning events.
 * Note that we 'register' each Entity with the Scheduler using the ID number.
 *
 * @author BenWise
 */
public abstract class Entity
    extends CountedItem
{

    public Entity(Scheduler s) {
        super(); // get ID and register it
        active = true; // reasonable default at creation time
        mySim = s;
    }


    /**
     * Do whatever this Entity is supposed to do. This method should not be
     * called without ensuring that the Entity is active (alive).
     */
    abstract public void process();

    public Scheduler getSim() {
        return mySim;
    }
    ;


    /**
     * Return T if this Entity is active (alive), F otherwise.
     *
     * @return T if active, F otherwise
     */
    public boolean isActive() {
        return active;
    }

    /**
     * Set this Entity to be active (alive) or inactive (dead)
     *
     * @param a T if active, F otherwise
     */
    public void setActive(boolean a) {
        active = a;
    }

    final protected Scheduler mySim;
    protected boolean active;
}


// =============================================================================
