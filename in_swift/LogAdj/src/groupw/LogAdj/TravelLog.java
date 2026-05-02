// Copyright Group W, SPA. All Rights Reserved.

package groupw.DCVRP;

import java.util.ArrayList;
import java.util.List;

// TODO: Verify that no two src.nodeName are equal
// TODO: Verify that no two dst.nodeName are equal

/**
 * TravelLog is built by a Serial as it moves from node to node on Transports.
 */
public class TravelLog  implements CountedItem {

    public TravelLog() {
        idNum = ItemCounter.makeID();
        legs = new ArrayList<Leg>() ;
    }

    public static class Stop {
        public Stop(String nn, double t) {
            nodeName = nn;
            time = t;
        }
        final String nodeName;
        final double time;
    }

    public static class Leg {
        public Leg(Stop src, String transName, Stop dst) {
            this.src = src;
            this.transName = transName;
            this.dst = dst;
        }
        Stop src;
        String transName;
        Stop dst;
    }

    public int numLegs() {
        return legs.size();
    }

    List<Leg> legs;

    public boolean validP() {
        boolean ok = validLegs();
        ok = ok && validStopOvers();
        ok = ok && validNoLoops();
        return ok;
    }

    private boolean validLegs() {
        boolean ok = true;
        int n = legs.size();
        for (int i = 0; (i < n) && ok; i++) {
            if (!(legs.get(i).src.time < legs.get(i).dst.time)) { // travel time always positive
                ok = false;
            }
            if (legs.get(i).src.nodeName.equalsIgnoreCase(legs.get(i).dst.nodeName)) {
                ok = false;
            }
        }
        return ok;
    }

    private boolean validStopOvers() {
        boolean ok = true;
        int n = legs.size();
        if (1 < n) {
            for (int i = 1; (i < n) && ok; i++) {
                if (!legs.get(i - 1).dst.nodeName.equals(legs.get(i).src.nodeName)) { // end of previous match current start
                    ok = false;
                }
                if (!(legs.get(i - 1).dst.time <= legs.get(i).src.time)) { // time of previous end <= time of current start
                    ok = false;
                }
            }
        }
        return ok;
    }

    private boolean validNoLoops() {
        boolean ok = true;
        int n = legs.size();
        if (1 < n) {
            for (int i = 0; (i < n) && ok; i++) {
                for (int j = i + 1; (j < n) && ok; j++) {
                    if (legs.get(i).src.nodeName.equalsIgnoreCase(legs.get(j).src.nodeName)) {
                        ok = false;
                    }
                    if (legs.get(i).dst.nodeName.equalsIgnoreCase(legs.get(j).dst.nodeName)) {
                        ok = false;
                    }
                }
            }
        }
        return ok;
    }

    /**
     * The numerical ID is mostly for identifying almost-anonymous
     * data structures during debugging. See the comments on CountedItem class.
     */
    @Override
    public long getID() {
        return idNum;
    }

    private final long idNum;
}


// Copyright Group W, SPA. All Rights Reserved.
