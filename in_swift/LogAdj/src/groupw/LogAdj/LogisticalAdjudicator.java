// Copyright Group W, SPA. All Rights Reserved.
package groupw.LogAdj;

import groupw.BaseSim.Scheduler;
import groupw.DCVRP.ItineraryBuilder;
import groupw.DCVRP.ReadDCVRScenarioCSV;
import groupw.DCVRP.VRController;

public class LogisticalAdjudicator {

    public LogisticalAdjudicator(ReadDCVRScenarioCSV.ScenarioRecord scenRec) {
        this.scenRec = scenRec;
        VRController.TheVRC = null;
        ItineraryBuilder.TheIB = null;
        mySim = new Scheduler();
    }

    public final ReadDCVRScenarioCSV.ScenarioRecord scenRec;
    public final Scheduler mySim;
}
// Copyright Group W, SPA. All Rights Reserved.
