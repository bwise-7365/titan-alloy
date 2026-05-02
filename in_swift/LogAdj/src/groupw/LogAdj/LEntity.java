// Copyright Group W, SPA. All Rights Reserved.

package groupw.LogAdj;

import groupw.BaseSim.Entity;
import groupw.BaseSim.Scheduler;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

public abstract class LEntity extends Entity {
    // trigger breakpoints for debugging
    static final  List<String>  monitoredSerials = // things that end up in weird locations
            new ArrayList<>(Arrays.asList(
            //"2-INF-BDE-JLTV-055",
            //"5-INF-BDE-D7-001"
            ));
    static final List<String> monitoredTransports = // things that took them there
            new ArrayList<>(Arrays.asList(
            //"MV22-18-USV",
            //"MV22-15-DMN",
            //"KC130-18-NG"
            ));


    abstract String status();

    public LEntity(Scheduler s) {
        super(s);
    }
}


// Copyright Group W, SPA. All Rights Reserved.