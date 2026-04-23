// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import groupw.DCVRP.ReadDCVRScenarioCSV;
import org.junit.Test;

public class LogisticalAdjudicatorTest {

    @Test
    public void testCreate() {
        ReadDCVRScenarioCSV.ScenarioRecord sRec = ReadDCVRScenarioCSV.readStandardTestCase();
        LogisticalAdjudicator la = new LogisticalAdjudicator(sRec);
    }
}
// Copyright Group W, SPA. All Rights Reserved.
