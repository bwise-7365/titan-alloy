/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.BaseSim;

/**
 * Anything that has a unique ID number
 */
abstract public class CountedItem {
    public CountedItem() {
        myID = ItemRegistry.nextID();
        ItemRegistry.register(this, myID);
    }
    protected final long myID;
    public long getID() { return myID;}
}
// =============================================================================
