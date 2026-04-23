/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.BaseSim;

import org.apache.commons.math4.legacy.linear.RealVector;
import org.junit.Test;

import static groupw.BaseSim.DSUtils.makeRV2;
import static org.junit.Assert.*;

/**
 *
 * @author bwise
 */
public class DSUtilsTest {
    
    public DSUtilsTest() {
    }


    @Test
    public void testSegmentIntersect() {
        RealVector A = makeRV2(10, 10);
        RealVector B = makeRV2(30, 30);
        RealVector C = makeRV2(30, 10);
        RealVector D = makeRV2(10, 30);
        RealVector E = makeRV2(40, 10);
        RealVector F = makeRV2(40, 30);
        RealVector G = makeRV2(50, 10);
        RealVector H = makeRV2(50, 30);
        double thresh = 1.0E-10;


        assertTrue(DSUtils.segmentIntersect(A, B, C, D, thresh));
        assertTrue(DSUtils.segmentIntersect(A, C, A, D, thresh));
        assertTrue(DSUtils.segmentIntersect(A, B, D, C, thresh));
        assertTrue(DSUtils.segmentIntersect(B, D, B, C, thresh));
        assertTrue(DSUtils.segmentIntersect(A, C, A, B, thresh));

        assertFalse(DSUtils.segmentIntersect(A, C, B, F, thresh));
        assertFalse(DSUtils.segmentIntersect(A, C, E, G, thresh));
        assertFalse(DSUtils.segmentIntersect(C, A, E, B, thresh));
        assertFalse(DSUtils.segmentIntersect(C, F, E, H, thresh));
    }

    
}

// =============================================================================
