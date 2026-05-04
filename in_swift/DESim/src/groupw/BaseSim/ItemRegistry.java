/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */
package groupw.BaseSim;

import java.util.HashMap;
import java.util.Map;

/**
 *
 */
public class ItemRegistry {

    /**
     * Initialize the singleton.
     * Notice that redundant calls are ignored.
     */
    public static void initialize() {
        if (null == theItemRegistry) {
            theItemRegistry = new ItemRegistry();
        }
    }

    /**
     * Private constructor for singleton
     */
    private ItemRegistry () {
        items = new HashMap<>(1000);
        highestID = 1000; // arbitrary starting value to help line up output
    }

    // necessary for complete determinism: reset between runs
    static public void reset() {
        theItemRegistry = null;
    }
    /**
     * Return the next unused item ID number.
     * Notice that the ItemRegistry will automatically initialize on first call.
     * @return
     */
    static public synchronized long nextID() {
        if (null == theItemRegistry){
            initialize();
        }
        return theItemRegistry.highestID++;
    }

    public static boolean isRegistered(long id) {
        if (null == theItemRegistry) {
            ItemRegistry.initialize();
            return false;
        } else {
            CountedItem ci = theItemRegistry.items.get(id);
            return (null != ci);
        }
    }

    public static int numItems() {
        return theItemRegistry.items.size();
    }

    /**
     * Get a registered item, or NULL if not registered.
     * @param id
     * @return
     */
    public static CountedItem getItem(long id){
        return theItemRegistry.items.get(id);
    }

    /**
     * Each item is automatically registered upon creation, so this
     * should not be called directly.
     * @param item
     * @param id
     */
    public static void register(CountedItem item, long id) {
//        System.out.printf("  Registering %d\n", id);
        if (null == theItemRegistry) {
            // This should never happen, as the ID should have come from the ItemRegistry
            throw  new RuntimeException("Tried to register item before initializing ItemRegistry");
        }
        if (isRegistered(id)) {
            throw  new RuntimeException("Tried to re-register item "+id+" which is already in ItemRegistry");
        }
        theItemRegistry.items.put(id, item);
//        System.out.printf("  Item %d is registered\n", id);
    }

    /**
     * Remove a counted item from the registry.
     * This is usually done because an Entity is destroyed, SimpleMessage deleted, etc.
     * @param id
     */
    public static void deregister(long id) {
//        System.out.printf("Deregistering %d\n", id);
        if (!isRegistered(id)) {
            throw  new RuntimeException("Tried to de-register item not in ItemRegistry");
        }
        theItemRegistry.items.remove(id);
//        System.out.printf("Item %d is deregistered\n", id);
    }


    private long highestID;
    private static ItemRegistry theItemRegistry = null;
    private Map<Long, CountedItem> items;
}
// =============================================================================
