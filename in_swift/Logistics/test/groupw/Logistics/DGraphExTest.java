/*
 *  ---------------------------------------------------
 *         Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics;

import org.junit.Test;
import static org.junit.Assert.*;

/**
 *
 * @author BenWise
 */
public class DGraphExTest {

    public DGraphExTest() {
    }


    /*
    expected output from 'process'

Strongly connected components:
([i], [])
([h], [])
([e, f, g], [(e,f), (f,g), (g,e)])
([a, b, c, d], [(a,b), (b,d), (d,c), (c,a)])

Shortest path from i to c:
[(i : h), (h : e), (e : d), (d : c)]

Shortest path from c to i:
null
     */
    @Test
    public void testProcess() {
        System.out.println("\nStarting testProcess");
        DGraphEx dg = new DGraphEx();
        dg.setup();
        dg.process();
    }

    @Test
    public void testCycleCheck() {
        System.out.println("\nStarting testCycleCheck");
        DGraphEx dg = new DGraphEx();
        dg.setup();
        boolean rslt = dg.cycleCheck();
        if (rslt) {
            System.out.println("Correctly detected known cycle.");
        } else {
            System.out.println("Failed to detect known cycle.");
        }
        assertTrue(rslt);
    }

}
