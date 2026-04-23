package groupw.BaseSim;

import static groupw.BaseSim.DSUtils.SECONDS_PER_HOUR;

/**
 *
 * @author BenWise
 */
public class EntEvent
        extends Event {

    /**
     * Construct an event for an Entity
     *
     * @param ent The Entity whose process() method will be called
     * @param s Scheduler managing this event
     * @param pTime Time at which the event should be processed
     */
    public EntEvent(Entity ent, Scheduler s, double pTime) {
        super(s, pTime);
        myEntity = ent;
    }

    /**
     * Process the event by calling the entity's process() method, but only if
     * the entity is active (alive). Scheduling another event is the
     * responsibility of the Entity, i.e. of its Entity::process method.
     */
    @Override
    public void process() {
        System.out.printf("%s Processing EntEvent %4d \n",
                mySim.timeStamp(), getID());
        if (myEntity.isActive()) {
            System.out.printf("Entity %4d is active and processing at time %.4f sec / %.2f hours \n",
                    myEntity.getID(), mySim.getCurrTime(), mySim.getCurrTime() / SECONDS_PER_HOUR);
            myEntity.process();
        } else {
            System.out.printf("Entity %4d is inactive and not processing at time %.4f sec / %.2f hours \n",
                    myEntity.getID(), mySim.getCurrTime(), mySim.getCurrTime() / SECONDS_PER_HOUR);
        }
    }


    final protected Entity myEntity;
}


// =============================================================================
