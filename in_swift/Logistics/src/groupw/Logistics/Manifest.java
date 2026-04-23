/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.Logistics;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;


/**
 *
 * @author JoshuaSteakelum
 */
public class Manifest {

    private final HashMap<String, Double> itemListing;

    /**
     * Store the set of Item,Quantity
     */
    public Manifest() {
        itemListing = new HashMap<>();
    }


    /**
     * Adds an item defined by a name string to the logistical inventory, with a given quantity.
     * If it already exists in the inventory, it adds it to the existing quantity. Otherwise, it creates it.
     *
     * @param name     The name string of the item to add
     * @param quantity The quantity or amount to add
     */
    public final void addInventory(String name, Double quantity) {
        if (!(quantity > 0.0)) {
            System.out.println("Warning: attempt to add " + quantity + " of " + name + " to a Manifest");
            return;
        }
        if (itemListing.containsKey(name)) {
            // add to what we have
            itemListing.replace(name, (Double) (itemListing.get(name) + quantity));
        } else {
            // we don't have any, so register it
            itemListing.put(name, quantity);
        }
    }

    /**
     * Given a map of how much 'resource' (weight, deck space, etc.) each item
     * requires, calculate the total resources to hold this manifest.
     * @param rqrts Map from resource name to amount required
     * @return total amount required for this manifest
     */
    public final double resourceWeight(final Map<String, Double> rqrts){
        double totalRsources = 0.0;
        for (String item : rqrts.keySet()){
            double rw = rqrts.get(item);
            double q = this.getAvailable(item);
            totalRsources = totalRsources + (q * rw);
        }
        return totalRsources;
    }

    /**
     * Create a new Manifest which is a scaled version of this one.
     * If the factor is zero or negative, return empty Manifest.
     * @param f factor by which to multiply
     * @return new, rescaled Manifest
     */
    public final Manifest makeScaled(double f) {
        Manifest m2 = new Manifest();
        if (f > 0.0) {
            for (Map.Entry<String, Double> e : itemListing.entrySet()) {
                double d = e.getValue() * f;
                if (d > 0.0) {
                    m2.itemListing.put(e.getKey(), d);
                }
            }
        }
        return m2;
    }

    /**
     * Non-destructively add together two Manifest objects.
     * Either or both are allowed to be NULL.
     * @param m1 manifest to be added
     * @param m2 manifest to be added
     * @return combined manifest
     */
    public static Manifest add(final Manifest m1,  final Manifest m2) {
        Manifest m3 = new Manifest();
        if (null != m1) {
            m3.itemListing.putAll(m1.itemListing);
        }
        if (null != m2) {
            for (Map.Entry<String, Double> e : m2.itemListing.entrySet()) {
                m3.addInventory(e.getKey(), e.getValue());
            }
        }
        return m3;
    }

    /**
     * Non-destructively subtract two Manifest objects.
     * Notice that if M2 'wants' to take out more of an item than M1 has,
     * then the result will accurately show zero remaining in M3.
     * If M1 has 100 and M2 'wants' 150, then there will be zero
     * left after just 100 are removed.
     * Either or both are allowed to be NULL.
     *
     * @param m1 manifest from which to remove items
     * @param m2 manifest of items to be removed
     * @return manifest of what would remain
     */
    public static Manifest sub(final Manifest m1, final Manifest m2) {
        Manifest m3 = new Manifest();  // zero items
        // if m1 is empty, then the result will be empty regardless of m2
        if ((null != m1) && !m1.isEmpty()) {
            m3 = Manifest.add(m3, m1);
            if ((null != m2) && !m2.isEmpty()) {
                for (Map.Entry<String , Double> e : m2.itemListing.entrySet()) {
                    m3.subtractInventory(e.getKey(), e.getValue());
                }
            }
        }
        return m3;
    }

    /**
     * Removes some quantity of an item from the inventory.
     * Throws an exception if an attempt to subtract more of a quantity than is contained is made.
     *
     * @param name     The item string to subtract
     * @param quantity How much to subtract
     */
    public final void subtractInventory(String name, Double quantity) {
        if (itemListing.containsKey(name)) {
            // if we have some amount at all
            Double available = itemListing.get(name);
            if (quantity > available) {
                // we're using more than we have!
                throw new ArithmeticException("Attempted to subtract " + quantity + " " + name + " from " + available + " available");
            } else if (quantity.equals(available)) {
                // using the last of what we have, remove that object from our inventory
                itemListing.remove(name);
            } else { // we have more than we are using, safest option
                itemListing.replace(name, (Double) (available - quantity));
            }
        } else {
            if (quantity > 0.0) {
                throw new ArithmeticException("Attempted to subtract " + quantity + " " + name + " from none available");
            }
            // do nothing - we're trying to subtract 0.0 from 0.0
        }
    }

    /**
     * Report how much of the desired quantity is available, without withdrawing anything.
     * If we want 10 of an item,
     * but only have 9 in inventory, report what is available (9).
     *
     * @param name     The name string of the desired item
     * @param quantity How much is desired, likely for use
     * @return The amount that is available, as a Double
     */
    public final Double getDesired(String name, Double quantity) {
        // if we want 10 but only have 9, give us the last 9
        Double q2;
        if (itemListing.containsKey(name)) {
            q2 = (Double) (Math.min(quantity, itemListing.get(name)));
        } else {
            q2 = (Double) (0.0);
        }
        return q2;
    }

    /**
     * Find out how much is available, without withdrawing anything.
     * @param name The name string of the desired item
     * @return The amount that is available, as a Double
     */
    public final Double getAvailable(String name) {
        if (itemListing.containsKey(name)) {
            return itemListing.get(name);
        } else {
            //Review should we return a negative so we know this does not have the key
            return (Double) 0.0;
        }
    }

    /**
     * Subtract/use up a desired amount of an item, based on availability.
     * Shortcut for getDesired and subtractInventory - first sees if enough is
     * available, then uses what is available regardless.
     *
     * @param name     The name string of the desired item
     * @param quantity How much is desired for use
     * @return If enough was available to be used
     */
    public final boolean useDesired(String name, Double quantity) {
        // if we have it and enough, use as much as we have up to desired
        // then tell them back if we had as much as we wanted

        Double available = getDesired(name, quantity);
        if (available > 0.0) {
            subtractInventory(name, available);
            return (available.compareTo(quantity)) == 0;
        }
        return false;

    }

    /**
     * Erase everything on the packing slip
     */
    public final void clear() {
        itemListing.clear();
    }

    /**
     * Is there anything on our slip?
     *
     * @return true if the slip is empty
     */
    public final boolean isEmpty() {
        return itemListing.isEmpty();
    }

    /**
     * Present a set of all unique items on the packing slip
     *
     * @return Set of strings pertaining to unique item names
     */
    public final Set<String> getItemNames() {
        return itemListing.keySet();
    }

    /**
     * Get the count of unique items on the slip, e.g "lines" on the slip.
     * NOTE: This will change as the Manifest is modified.
     *
     * @return Total number (not quantity) of unique slip items
     */
    public final int uniqueItemCount() {
        return itemListing.size();
    }

    /**
     * Simply show the (Item,Quantity) pairs present, but
     * rounding to four decimal places.
     *
     * @return String of (item, quantity) pairs
     */
    public String toString() {
        List<String> itemList = new ArrayList<>(itemListing.keySet());
        int numItems = itemList.size();
        StringBuilder sb = new StringBuilder();
        sb.append("{");
        for (int i=0; i<numItems; i++) {
            String s1 = itemList.get(i);
            sb.append(String.format("%s=%.4f", s1, itemListing.get(s1)));
            if (i != numItems - 1) {
                sb.append(", ");
            }

        }
        sb.append("}");
        return sb.toString();
    }
    
    /*
    public String toString(){
        return this.Listing.toString();
    }
    */
    
    
    /* implement this if necessary - simply compare the Listing maps, but it's not publicly presented.
    * Notice that we need some 'tolerance' to determine if quantities are close enough to
    * be considered equal (double values always have round-off problems).
    @Override
    public boolean equals(Object o){
        if(!(o instanceof Manifest)){
            return false;
        }
        if (o == this){
            return true;
        }
        return Listing.equals((Manifest)o.Listing);
    }
    */

    /**
     * @return the itemListing
     */
    public HashMap<String, Double> getListing() {
        return itemListing;
    }

}
// =============================================================================
