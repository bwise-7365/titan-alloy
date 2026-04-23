/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.BaseSim;

import java.util.Comparator;

/**
 *
 * @author BenWise
 */
public class EventComparator implements Comparator<Event> {

    /**
     * Compare two events first by their times, then by ID numbers if times are tied.
     * Because ID numbers are guaranteed to be unique, there will never be ties
     * unless X == Y (which will fail an 'assert').
     *
     * @param x An event
     * @param y An event
     * @return -1 is X should go before Y; +1 if Y should go before X.
     */
    @Override
    public int compare(Event x, Event y) {
        // Assume neither event is null. Real code should probably be more robust
        boolean xEarlier = (x.getProcTime() < y.getProcTime());
        boolean yEarlier = (y.getProcTime() < x.getProcTime());
        boolean compEvnt = (x.getID() < y.getID());

        assert (x.getID() != y.getID());

        if (xEarlier) {
            return -1;
        } else if (yEarlier) {
            return +1;
        } else if (compEvnt) {
            return -1;
        } else {
            return +1;
        }
    }
}


// =============================================================================
