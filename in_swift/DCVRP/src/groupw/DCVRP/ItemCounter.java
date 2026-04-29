// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

public class ItemCounter {


    /**
     * Private constructor for singleton
     */
    private ItemCounter () {
        highestID = initialValue;
        theCounter = null;
    }

    /**
     * The numerical ID is mostly for identifying almost-anonymous
     * data structures during debugging. When running a simulation
     * over and over, it might be useful to let the ID numbers just
     * keep growing so that the ID number for the 7396-th entity
     * out of 18727 entities, on the 3748-th run can be used to trigger
     * a breakpoint on ID == 70196192, right before problematic behavior
     * occurs.
     */
    static public long makeID() {
        if (null == theCounter) {
            theCounter = new ItemCounter();
        }
        theCounter.highestID++;

        // counting up to 64-bit rollover is unlikely,
        // but it could be reset to just short of rollover.
        if (theCounter.highestID < initialValue) {
            theCounter.highestID = initialValue;
        }

        return theCounter.highestID;
    }
    /**
     * Rarely is it useful to reset the highest ID number.
     * Values below initialValue replaced by initialValue.
     */
    static public void reset(long n) {
        if (null == theCounter) {
            theCounter = new ItemCounter();
        }
        theCounter.highestID = (n < initialValue) ? initialValue : n;
    }

    private long highestID;
    static private ItemCounter theCounter = null;

    // arbitrary starting value to help line up output
    static private final long initialValue = 1000;

}

// =============================================================================

