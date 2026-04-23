/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics;

import groupw.Logistics.LogDistNW.LogDistArc;
import groupw.Logistics.LogDistNW.LogDistNode;
import groupw.Logistics.Parsers.CsvFileSpec;
import groupw.Logistics.Parsers.CsvProcessor;
import static groupw.Network.NWUtils.DefaultSeedPRNG;
import static groupw.Network.NWUtils.makePRNG;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Random;

import groupw.Logistics.Parsers.DuplicateItemException;
import logging.LoggerSetup;
import org.jgrapht.graph.Pseudograph;
import org.junit.Test;
import static org.junit.Assert.*;
import org.junit.Before;

/**
 *
 * @author BenWise
 */
public class LogDistNWTest {

    private CsvProcessor processor = new CsvProcessor();
    private Map<String, LogDistNode> logDistNodeMap = new HashMap<>();
    private Map<String, LogDistArc> logDistArcMap = new HashMap<>();

    public LogDistNWTest() {

    }

    @Before
    public void setup() throws DuplicateItemException {
        LoggerSetup.configureLogger();
        processor.initTest();
        logDistNodeMap = (Map<String, LogDistNode>) CsvProcessor.getData(CsvFileSpec.LOGDISTNODE);
        logDistArcMap = (Map<String, LogDistArc>) CsvProcessor.getData(CsvFileSpec.LOGDISTARC);
    }

    @Test
    public void testNW00() {
        makeNW00();
    }

    public LogDistNW makeNW00() {
        int sd = DefaultSeedPRNG;
        Random prng = makePRNG(sd, true);
        Pseudograph<LogDistNode, LogDistArc> ldGraph = new Pseudograph<>(LogDistArc.class);

        //System.out.printf("Break here\n");

        // Planes fly from airbase to the airports, shuttle back and forth between
        // airports and islands, then return to base.
        // Boats travel from the seaport to the islands and between islands.
        for (LogDistNode logDistNode : logDistNodeMap.values()) {
            ldGraph.addVertex(logDistNode);
        }

        assertEquals(14, ldGraph.vertexSet().size());

        List<LogDistNode> APODs = new ArrayList<>(1);
        APODs.add(logDistNodeMap.get("AirBase-G"));
        APODs.add(logDistNodeMap.get("AirBase-SJ"));
        APODs.add(logDistNodeMap.get("AirBase-SD"));

        List<LogDistNode> SPODs = new ArrayList<>(1);
        SPODs.add(logDistNodeMap.get("SeaPort-G-1"));
        SPODs.add(logDistNodeMap.get("SeaPort-SD-1"));
        SPODs.add(logDistNodeMap.get("SeaPort-SD-2"));
        SPODs.add(logDistNodeMap.get("SeaPort-SD-3"));
        SPODs.add(logDistNodeMap.get("SeaPort-SJ-1"));

        List<LogDistNode> islands = new ArrayList<>(1);
        islands.add(logDistNodeMap.get("Island-1-USVI"));
        islands.add(logDistNodeMap.get("Island-2-Dominica"));
        islands.add(logDistNodeMap.get("Island-3-A&B"));
        islands.add(logDistNodeMap.get("Island-4-Mrtnq"));
        islands.add(logDistNodeMap.get("Island-5-Lucia"));
        islands.add(logDistNodeMap.get("Island-6-Vincent"));

        // count arcs from this APOD to all (if any) SPODs
        int arcCount = 0;
        LogDistNode airbaseSD = logDistNodeMap.get("AirBase-SD");
        for (LogDistArc arc : logDistArcMap.values()) {
            if (arc.srcNodeName.equalsIgnoreCase(airbaseSD.name)) {
                for (LogDistNode tgt : SPODs) {
                    assertNotNull(tgt);
                    if (arc.tgtNodNamee.equalsIgnoreCase(tgt.name)) {
                        LogDistNode t1 = arc.getSrc();
                        assertNull(t1); // verify not yet configured
                        LogDistNode t2 = arc.getTgt();
                        assertNull(t2); // verify not yet configured
                        arcCount++;
                    }
                }
            }
        }
        assertEquals(3, arcCount);

        // insert all node->node arcs, skipping self-arcs
        for (LogDistArc arc : logDistArcMap.values()) {
            String arcSrcName = arc.srcNodeName;
            String arcTgtName = arc.tgtNodNamee;
            for (LogDistNode src : logDistNodeMap.values()) {
                for (LogDistNode tgt : logDistNodeMap.values()) {
                    String srcName = src.name;
                    String tgtName = tgt.name;
                    if (!srcName.equalsIgnoreCase(tgtName)) {
                        if (srcName.equalsIgnoreCase(arcSrcName) && tgtName.equalsIgnoreCase(arcTgtName)) {
                            ldGraph.addEdge(src, tgt, arc);
                        }
                    }
                }
            }
        }
        int ess =  ldGraph.edgeSet().size();
        assertEquals(121, ess); // for this graph, links "0001" to "0121" are defined in CSV file

        LogDistNW ldnw = new LogDistNW();
        ldnw.ldGraph = ldGraph;
        return ldnw;
    }

}
// =============================================================================
