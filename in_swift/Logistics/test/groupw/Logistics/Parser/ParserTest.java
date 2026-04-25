/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parser;

import groupw.Logistics.Dispatcher;
import groupw.Logistics.LogSprtNW;
import groupw.Logistics.LogSprtNW.SupportNode;
import groupw.Logistics.LogSprtNW.SupportArc;
import groupw.Logistics.LogDistNW;
import groupw.Logistics.LogDistNW.LogDistArc;
import groupw.Logistics.LogDistNW.LogDistNode;
import groupw.Logistics.Manifest;
import groupw.Logistics.Parsers.CsvFileSpec;
import groupw.Logistics.Parsers.CsvProcessor;
import groupw.Logistics.Unit;
import groupw.Logistics.UnitType;
import groupw.Network.NWUtils.Tuple2;
import static groupw.Logistics.Dispatcher.calcHeights;
import static groupw.Logistics.Parsers.CsvProcessor.getData;
import static groupw.Logistics.Parsers.CsvProcessor.getPathMap;
import static org.junit.Assert.*;
import java.awt.GraphicsEnvironment;
import java.io.File;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.*;
import java.util.logging.Level;
import java.util.logging.Logger;
import groupw.Logistics.Parsers.DuplicateItemException;
import logging.LoggerSetup;
import org.jgrapht.graph.DefaultEdge;
import org.jgrapht.graph.Pseudograph;
import org.jgrapht.graph.SimpleDirectedGraph;
import org.junit.BeforeClass;
import org.junit.FixMethodOrder;
import org.junit.Test;
import org.junit.runners.MethodSorters;

/**
 *
 * @author DavidHa
 */
@FixMethodOrder(MethodSorters.NAME_ASCENDING)
public class ParserTest {
    // setup path to one of the possible sets of test data
    private static final String userDir = System.getProperty("user.dir") + File.separator + "test" + File.separator + "groupw" + File.separator; // TODO: fix hard-coded path
    private static final String testFile = "Logistics" + File.separator + "Data" + File.separator + "configuration.csv";
    private static final String testPath = userDir + testFile;

    /**
     * testGetValidLogNW is supposed to read error-free data files (without crashing),
     * verify that no false parsing errors are 'detected'
     * and verify that all data structures are built correctly.
     * If you want to test correct detection of known errors, a test for that purpose and
     * corresponding erroneous data will be required.
     *
     * @throws DuplicateItemException
     */
    @Test
    public void testGetValidLogNW() throws DuplicateItemException {
        CsvProcessor processor = new CsvProcessor();
        String testDir = System.getProperty("user.dir");
        String fs = System.getProperty("file.separator");
        String dataPath = testDir + fs + "test" + fs + "groupw" + fs + "Logistics" + fs + "Data2" + fs + "configuration.csv"; // TODO: fix hard-coded path
        File file = new File(dataPath);
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, file.getAbsolutePath());
        processor.initPaths();
        useConfigFile();

        // NOTE WELL: the following code uses assertions for Data2/configuration.csv
        // To use a different configuration file, you must change or remove them.
        LogDistNW ldnw = makeLogDistNW();
        verifyLDN(ldnw);
        assertEquals(14, ldnw.getNodes().size()); // true for Data2

        LogSprtNW lsnw = makeLogSprtNW();
        verifyLSN(lsnw);
        assertEquals(16, lsnw.getNodes().size()); // true for Data2

        Set<Unit> units = getUnits();
        verifyUnits(units);
        assertEquals(16, units.size()); // true for Data2

        // get the unitMap as parsed earlier
        Map<String, Unit> unitMap = (Map<String, Unit>) CsvProcessor.getCsvDataCache().get(CsvFileSpec.UNIT);

        Set<Dispatcher> dispatchers = makeDispatchers(lsnw, ldnw, unitMap);
        assertEquals(6, dispatchers.size()); // true for Data2
        verifyDispatchers(dispatchers);

        // as of 2025-09, there is no easy way in jgraphT to find cycles of an undirected graph.
        //assertFalse(Dispatcher.checkDGraphTree(dGraph));

        // All the dispatchers have pointer to the same dGraph and heights, so we get any arbitrary one.
        Dispatcher[] dList = dispatchers.toArray(new Dispatcher[0]);
        SimpleDirectedGraph<String, DefaultEdge> dGraph = dList[0].getDGraph();
        List<Tuple2<String, Integer>> heights = dList[0].getHeight();
        for (Tuple2<String, Integer> t : heights) {
            System.out.println(t.get0() + " has height " + t.get1());
        }

        Dispatcher.rollUpDispatchers(dispatchers); // after their heights were set
        System.out.println("All Done.");
    }

    /**
     * Make the set of Dispatchers implied by the LogSprtNW and so on
     *
     * @param lsnw log support network
     * @param ldnw log distribution network
     * @param unitMap Map from unit-names to units
     * @return the new Dispatchers
     */
    public Set<Dispatcher> makeDispatchers(LogSprtNW lsnw, LogDistNW ldnw, Map<String, Unit> unitMap) {

        // the Set will merge duplicates, but typos might make them differ only in case.
        Set<SupportNode> sNodes = lsnw.getNodes();
        Set<String> uniqueNames = new HashSet<>(sNodes.size());
        for (SupportNode sn : sNodes) {
            String dName = sn.getDispatcherName();
            assertNotNull(dName);
            assertFalse(dName.isEmpty());
            uniqueNames.add(dName);
        }

        Set<Dispatcher> dispatchers = new HashSet<>(uniqueNames.size());
        for (String dName : uniqueNames) {
            Dispatcher d = new Dispatcher(dName, ldnw, lsnw, unitMap);
            d.setSupports();
            dispatchers.add(d);
        }
        SimpleDirectedGraph<String, DefaultEdge> dGraph = Dispatcher.makeDispatcherGraph(dispatchers);

        // Calculate heights, which is also the order in which Dispatchers should rollup.
        List<Tuple2<String, Integer>> heights = calcHeights(dGraph);
        for (Dispatcher d : dispatchers) {
            d.setDGraph(dGraph);
            d.setHeight(heights);
        }
        return dispatchers;
    }

    public void verifyDispatchers(Set<Dispatcher> dispatchers) {
        assertNotNull(dispatchers);
        assertFalse(dispatchers.isEmpty()); // need at least one
        int numDispatchers = dispatchers.size();
        System.out.printf("Found %d dispatcher names\n", numDispatchers);
        assertEquals(6, numDispatchers);
        List<String> dNames = new ArrayList<>(numDispatchers);
        for (Dispatcher d : dispatchers) {
            String dName = d.getName();
            dNames.add(dName);
            System.out.printf("  Dispatcher: ->%s<- \n", dName);
        }
        int numDNames = dNames.size();
        // Notice that we do a case-insensitive comparison to catch case-typos
        for (int i = 0; i < numDNames; i++) {
            String iName = dNames.get(i).toLowerCase();
            for (int j = i + 1; j < numDNames; j++) {
                String jName = dNames.get(j).toLowerCase();
                assertNotEquals(iName, jName);
            }
        }

        // All the dispatchers have pointer to the same dGraph, so we get any arbitrary one.
        Dispatcher[] dList = dispatchers.toArray(new Dispatcher[0]);
        SimpleDirectedGraph<String, DefaultEdge> dGraph = dList[0].getDGraph();
        assertNotNull(dGraph);
        assertEquals(dispatchers.size(), dGraph.vertexSet().size()); // always true
        assertFalse(Dispatcher.checkCyclic(dGraph)); // must be true for any valid support graph

        Set<DefaultEdge> eSet = dGraph.edgeSet();
        assertEquals(5, eSet.size()); // true for Data2
        for (DefaultEdge de : eSet) {
            String s = dGraph.getEdgeSource(de);
            String t = dGraph.getEdgeTarget(de);
            System.out.printf("Dispatcher %s supports %s\n", s, t);
        }

    }

    public Set<Unit> getUnits() {
        // copied from indicated files as templates

        // UnitTypeParser.java:78:        List<String> postures = (List<String>) CsvProcessor.csvDataCache.get(CsvFileSpec.POSTURE);
        List<String> postureNames = (List<String>) CsvProcessor.getCsvDataCache().get(CsvFileSpec.POSTURE);
        int numPostures = postureNames.size();
        System.out.printf("Found %d posture names\n", numPostures);
        assertEquals(2, numPostures);
        for (String pName : postureNames) {
            System.out.printf("  Posture name: ->%s<-\n", pName);
        }

        //UnitTypeParser.java:77:        Map<String, UnitType> unitTypes = (Map<String, UnitType>) CsvProcessor.csvDataCache.get(CsvFileSpec.UNITTYPE);
        Map<String, UnitType> unitTypeMap = (Map<String, UnitType>) CsvProcessor.getCsvDataCache().get(CsvFileSpec.UNITTYPE);
        System.out.printf("Found %d unitType names\n", unitTypeMap.size());
        assertEquals(3, unitTypeMap.size());
        for (Map.Entry<String, UnitType> entry : unitTypeMap.entrySet()) { // slightly more verbose
            System.out.printf("  UnitType name: ->%s<-\n", entry.getKey());
            UnitType ut = entry.getValue();
            Set<String> postures = ut.getPostures();
            assertEquals(numPostures, postures.size());
            Map<String, Manifest> dcrs = ut.dcRates;
            assertNotNull(dcrs);
        }

        //UnitParser.java:54:        Map<String, Unit> unitMap = (Map<String, Unit>) CsvProcessor.csvDataCache.get(CsvFileSpec.UNIT);
        Map<String, Unit> unitMap = (Map<String, Unit>) CsvProcessor.getCsvDataCache().get(CsvFileSpec.UNIT);
        int numUnits = unitMap.size();
        System.out.printf("Found %d unit names\n", numUnits);
        assertEquals(16, numUnits);
        Set<Unit> units = new HashSet<>(numUnits);
        String defaultPosture = "Unengaged"; // for Data2

        for (Map.Entry<String, Unit> entry : unitMap.entrySet()) {
            System.out.printf("  Unit name: ->%s<-\n", entry.getKey());
            Unit u = entry.getValue();
            assertNotNull(u);
            u.setPosture(defaultPosture); // just for testing
            assertEquals(u.getName(), entry.getKey());
            assertEquals(u.getType().getPostures().size(),
                    u.getType().dcRates.size());
            Manifest dcr = u.sStatus.consumptionRate;

            assertNotNull(dcr);
            u.sStatus.reorderLevel = dcr.makeScaled(2.0); // 2 days supply is reorder point
            u.sStatus.maxDesiredLevel = dcr.makeScaled(5.0); // 5 days supply is max desired
            u.sStatus.currentLevel = dcr.makeScaled(1.0); // start them all below reorder level
            units.add(u);
        }
        assertEquals(numUnits, units.size()); // only for Data2
        return units;
    }

    /**
     * Assuming the units came from Data2/configuration.csv, check them.
     */
    protected void verifyUnits(Set<Unit> units){
        assertNotNull(units);
    }

    /**
     * Make a new Log distribution network from the cached data,
     * obviously after reading CSV and populating the caches.
     * @return the new LogDistNW
     */
    public LogDistNW makeLogDistNW() {
        // map of node-names to node objects
        Map<String, LogDistNode> nmp = (Map<String, LogDistNode>) (CsvProcessor.getCsvDataCache().get(CsvFileSpec.LOGDISTNODE));
        Map<String, LogDistArc> amp = (Map<String, LogDistArc>) (CsvProcessor.getCsvDataCache().get(CsvFileSpec.LOGDISTARC));

        Pseudograph<LogDistNode, LogDistArc> ldGraph = new Pseudograph<>(LogDistArc.class);

        for (LogDistNode ldn : nmp.values()) {
            ldGraph.addVertex(ldn);
        }

        for (LogDistArc lda : amp.values()) {
            LogDistNode src = ldGraph.getEdgeSource(lda);
            LogDistNode tgt = ldGraph.getEdgeTarget(lda);
            // verify that the arc is not yet configured
            assertNull(src);
            assertNull(tgt);
            LogDistNode trueSrc = nmp.get(lda.srcNodeName);
            LogDistNode trueTgt = nmp.get(lda.tgtNodNamee);
            assertNotNull(trueSrc);
            assertNotNull(trueTgt);
            ldGraph.addEdge(trueSrc, trueTgt, lda);
            src = ldGraph.getEdgeSource(lda);
            tgt = ldGraph.getEdgeTarget(lda);
            // verify that the arc is now configured
            assertEquals(trueSrc, src);
            assertEquals(trueTgt, tgt);
        }

        System.out.printf("Number of dist nodes: %3d\n", ldGraph.vertexSet().size());
        System.out.printf("Number of dist edges: %3d\n", ldGraph.edgeSet().size());
        // These values are from the 'distcalc' code which
        // generated the CSV file of arcs.
        assertEquals(14, ldGraph.vertexSet().size());
        assertEquals(121, ldGraph.edgeSet().size());

        LogDistNW ldnw = new LogDistNW();
        ldnw.setGraph(ldGraph);
        return ldnw;
    }


    /**
     * Assuming the distribution network came from Data2/configuration.csv, check that
     * nodes have correct locations and domains, arcs have correct length and domains,
     * and so on.
     */
    protected void verifyLDN(LogDistNW ldnw) {
        CsvProcessor processor = CsvProcessor.getInstance(false); // do not create a new one
        assertNotNull(processor);  // verify that it was created and initialized earlier
        assertNotNull(ldnw);
    }

    /**
     * Make a new Log support network from the cached data,
     * obviously after reading CSV and populating the caches.
     * @return the new LogSprtNW
     */
    public LogSprtNW makeLogSprtNW() {
        // map of node-names to node objects
        Map<String, SupportNode> nmp = (Map<String, SupportNode>) (CsvProcessor.getCsvDataCache().get(CsvFileSpec.SUPPORTNODE));
        Map<String, SupportArc> amp = (Map<String, SupportArc>) (CsvProcessor.getCsvDataCache().get(CsvFileSpec.SUPPORTARC));

        SimpleDirectedGraph<SupportNode, SupportArc> lsGraph = new SimpleDirectedGraph<>(SupportArc.class);

        for (SupportNode ldn : nmp.values()) {
            lsGraph.addVertex(ldn);
        }

        for (SupportArc lda : amp.values()) {
            SupportNode src = lsGraph.getEdgeSource(lda);
            SupportNode tgt = lsGraph.getEdgeTarget(lda);
            // verify that the arc is not yet configured
            assertNull(src);
            assertNull(tgt);
            SupportNode trueSrc = nmp.get(lda.srcLogDistNode);
            SupportNode trueTgt = nmp.get(lda.tgtLogDistNode);
            assertNotNull(trueSrc);
            assertNotNull(trueTgt);
            lsGraph.addEdge(trueSrc, trueTgt, lda);
            src = lsGraph.getEdgeSource(lda);
            tgt = lsGraph.getEdgeTarget(lda);
            // verify that the arc is now configured
            assertEquals(trueSrc, src);
            assertEquals(trueTgt, tgt);
        }
        System.out.printf("Number of sprt nodes: %3d\n", lsGraph.vertexSet().size());
        System.out.printf("Number of sprt edges: %3d\n", lsGraph.edgeSet().size());
        assertEquals(16, lsGraph.vertexSet().size());
        assertEquals(21, lsGraph.edgeSet().size());

        LogSprtNW lsnw = new LogSprtNW();
        lsnw.setGraph(lsGraph);
        assertFalse(lsnw.cycleCheck());
        return lsnw;
    }

    /**
     * Assuming the support network came from Data2/configuration.csv, check that units
     * have all the required data, have it correctly, and don't have extra junk "data".
     */
    protected void verifyLSN(LogSprtNW lsnw) {
        CsvProcessor processor = CsvProcessor.getInstance(false); // do not create a new one
        assertNotNull(processor);  // verify that it was created and initialized earlier
        assertNotNull(lsnw);
        Set<SupportNode> nodes = lsnw.getNodes();

        List<String> expVT = new ArrayList<>();
        expVT.add("Truck-2Ton");
        expVT.add("Ship-Small");
        expVT.add("FWA-Large");
        expVT.add("FWA-Medium");
        expVT.add("Truck-10Ton");
        expVT.add("Ship-Medium");
        // for now, I check one specific node, knowing that the
        // same parser/data builder is used for all of them.
        for (SupportNode lsn : nodes) {
            if ("SeaPort-SD-1".equals(lsn.getLogDistNodeName())) {
                assertTrue(lsn.unitName.equals("Supply-Co-1A"));
                assertTrue(lsn.getDispatcherName().equals("Central"));

                // this will be 1027 if run alone, 1059 if run as part of the whole test suite
                assertTrue((1027 == lsn.getID()) || (1059 == lsn.getID()));

                // check the vehicles at this supportnode
                Map<String, Integer> vehicles = lsn.vehicles;
                assertEquals(6,  vehicles.size());
                assertEquals( 0, (int)(vehicles.get(expVT.get(0)))); // avoid compiler ambiguity
                assertEquals(20, (int)(vehicles.get(expVT.get(1))));
                assertEquals( 0, (int)(vehicles.get(expVT.get(2))));
                assertEquals( 0, (int)(vehicles.get(expVT.get(3))));
                assertEquals( 0, (int)(vehicles.get(expVT.get(4))));
                assertEquals( 1, (int)(vehicles.get(expVT.get(5))));
                assertNull(vehicles.get("Dispatcher")); // make sure that old error is fixed

                // check the log dist node where it should be placed
                LogDistNode ldn = lsn.getLogDistNode();

                // On Data\configuration.csv, this will be 1149 if run alone, 1284 if run as part of the whole test suite
                boolean okData = (1149 == ldn.getID())||(1284 == ldn.getID());
                // On Data2\configuration.csv, this will be 1257 if run alone, 1392 if run as part of the whole test suite
                boolean okData2 = (1257 == ldn.getID())||(1392 == ldn.getID());
                assertTrue(okData || okData2);
                assertTrue(ldn.name.equals(lsn.getLogDistNodeName())); // i.e. "SeaPort-SD-1" as above
                // The values expected below are only for Data2'
                double tolerance = 0.001;
                { Manifest m = ldn.storeMax;
                    assertEquals(4, m.uniqueItemCount());
                    assertEquals(12068.5, (double) (m.getAvailable("AmmoCase")), tolerance); // avoid ambiguity at compile-time
                    assertEquals(102940.0, (double) (m.getAvailable("FuelGallon")), tolerance);
                    assertEquals(637.5, (double) (m.getAvailable("MREPallet")), tolerance);
                    assertEquals(83932.5, (double) (m.getAvailable("WaterGallon")), tolerance);
                }
                { Manifest m = ldn.throughDaily;
                    assertEquals(4, m.uniqueItemCount());
                    assertEquals(48000, (double) (m.getAvailable("AmmoCase")), tolerance); // avoid ambiguity at compile-time
                    assertEquals(200000.0, (double) (m.getAvailable("FuelGallon")), tolerance);
                    assertEquals(1275.0, (double) (m.getAvailable("MREPallet")), tolerance);
                    assertEquals(175000.0, (double) (m.getAvailable("WaterGallon")), tolerance);
                }
            }
        }
        assertEquals(16, nodes.size());
    }

    public ParserTest() {
        // nothing yet
    }

    @BeforeClass
    public static void setupLogging() {
        LoggerSetup.configureLogger();
    }

    @Test
    public void testGetCSVFilesFromTestLocation() throws DuplicateItemException {
        Logger.getLogger(ParserTest.class.getName()).log(Level.INFO, "\n --- Test check csv file paths from test location. --- \n");
        CsvProcessor processor = new CsvProcessor();
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, testPath);
        processor.initPaths();
        getPathMap().entrySet().forEach(csvFileSpecEntry -> {
            assertFalse(csvFileSpecEntry.getValue().isEmpty());
        });
    }

    @Test
    public void testPopulateAndVerify() throws DuplicateItemException {
        testPopulateCSVFileCache();  // create a new CsvProcessor and populate the static class members with cached data
        testVerifyDataTest(); // try to verify the same cached data
    }

    /**
     * This uses the hard-coded value of CsvProcessor.LOG_CONFIGURATION_FILE_PATH.
     * It will create a new, empty CsvProcessor and populate the static class members with cached data.
     */
    public void testPopulateCSVFileCache() throws DuplicateItemException {
        Logger.getLogger(ParserTest.class.getName()).log(Level.INFO, "\n --- CsvFileSpec cache. --- \n");
        CsvProcessor processor = new CsvProcessor();
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, testPath);
        processor.initPaths();
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, testFile);
        for (Map.Entry<CsvFileSpec, String> csvFileSpecEntry : getPathMap().entrySet()) {
            assertFalse(csvFileSpecEntry.getValue().isEmpty());
            Path fullPath = Paths.get(userDir, csvFileSpecEntry.getValue());
            assertTrue(Files.exists(fullPath));
            csvFileSpecEntry.setValue(fullPath.toString());
        }

        for (CsvFileSpec fileSpec : CsvFileSpec.values()) {
            Object object = getData(fileSpec);
            Logger.getLogger(ParserTest.class.getName()).log(Level.WARNING, String.format("Testing %s for empty collection, map, or null object.", fileSpec.toString()));
            if (object instanceof List<?>) {
                assertFalse((((List<?>) object)).isEmpty());
            } else if (object instanceof Map<?, ?>) {
                assertFalse((((Map<?, ?>) object)).isEmpty());
            } else {
                assertNotNull(object);
            }
        }
    }

    /**
     * Assuming the static class members of the CsvProcessor class hold cached data,
     * try to verify its correctness.
     * @throws DuplicateItemException
     */
    public void testVerifyDataTest() throws DuplicateItemException {
        Logger.getLogger(ParserTest.class.getName()).log(Level.INFO, "\n --- Testing Data Verification --- \n");
        CsvProcessor processor = CsvProcessor.getInstance(false); // do not create a new one
        assertNotNull(processor);  // verify that it was created and initialized earlier
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, testPath);
        processor.initPaths();
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, testFile);
        List<String> errorLogs = new ArrayList<>();

        // when 'testPopulateCsvFileCache' is run before 'testVerifyDataTest' the
        // unitTypeMap is correctly populated with two kinds of units from the 'Data' directory,
        // Infantry and Artillery.
        // To get access to the CsvProcessor instance without accidentally clearing it,
        // we created the getInstance(false) static method.
        Map<String, UnitType> unitTypeMap = (Map<String, UnitType>) CsvProcessor.getCsvDataCache().get(CsvFileSpec.UNITTYPE);
        assert(null != unitTypeMap);
        assertEquals(2, unitTypeMap.size()); // only for 'Data' configuration

        // verify existence of each file and cache its contents
        for (Map.Entry<CsvFileSpec, String> csvFileSpecEntry : getPathMap().entrySet()) {
            Path fullPath = Paths.get(userDir, csvFileSpecEntry.getValue());
            assertTrue(Files.exists(fullPath));
            csvFileSpecEntry.setValue(fullPath.toString());
        }

        // read each and load into cache
        for (CsvFileSpec fileSpec : CsvFileSpec.csvFileMap.keySet()) {
            getData(fileSpec);
        }

        // verify each, including references to and from others
        for (CsvFileSpec csvFileSpec : CsvProcessor.getCsvDataCache().keySet()) {
            Logger.getLogger(ParserTest.class.getName()).log(Level.INFO, String.format("Verifying data from csv: %s", csvFileSpec.toString()));
            // see comments in useConfigFile() for explanation of why this unchecked conversion
            // warning would be difficult to eliminate
            List<String> newErrors = csvFileSpec.getParser().verifyData();

            // This test is supposed to run on error-free data
            // and verify that no false errors are detected.
            // If you want to test correct detection of known errors,
            // a test for that purpose (and erroneous data) will be required.
            assertEquals(0, newErrors.size());

            errorLogs.addAll(newErrors); // avoid 'unchecked method invocation' warning
        }
        int i = 0;
        for (String log : errorLogs) {
            Logger.getLogger(ParserTest.class.getName()).log(Level.INFO, String.format("%s %s", i, log));
            i++;
        }
    }

    /**
     * Test having the user choose a configuration file via GUI then parsing every file to which it points.
     * Notice that user interaction is required to pass the test.
     *
     * @throws DuplicateItemException
     */
    @Test
    public void testConfigFileChooser() throws DuplicateItemException {
        initFromConfigFile();
    }

    /**
     * Have the user choose a configuration file via GUI then parse every file to which it points.
     * Notice that user interaction is required.
     *
     * @throws DuplicateItemException
     */
    public void initFromConfigFile() throws DuplicateItemException {
        Logger.getLogger(ParserTest.class.getName()).log(Level.INFO, "\n --- Testing File Chooser --- \n");
        if (!GraphicsEnvironment.isHeadless()) {
            CsvProcessor processor = new CsvProcessor();
            processor.initWithFileChooser();
            useConfigFile();
        } else {
            Logger.getLogger(ParserTest.class.getName()).log(Level.INFO, "Running in headless mode. Skipping file chooser dialog test.");
        }
    }

    /**
     * After getting the name of a configuration file, use it.
     * The user might choose the file from a JFileChooser dialog (e.g. initFromConfigFile()),
     * or the code might provide it without user input (e.g. for testing).
     * Very similar to testVerifyDataTest, but it does more checks and does not use 'testPath' or 'testFile'.
     *
     * @throws DuplicateItemException
     */
    public void useConfigFile() throws DuplicateItemException {
        List<String> errorLogs = new ArrayList<>();
        // verify existence of each file and cache its contents
        for (Map.Entry<CsvFileSpec, String> csvFileSpecEntry : getPathMap().entrySet()) {
            if (csvFileSpecEntry.getKey().equals(CsvFileSpec.LOGCONFIGURATION)) {
                continue;
            }
            assertFalse(csvFileSpecEntry.getValue().isEmpty());
            Path fullPath = Paths.get(userDir, csvFileSpecEntry.getValue());
            assertTrue(Files.exists(fullPath));
            csvFileSpecEntry.setValue(fullPath.toString());
        }
        // read each, load into cache and verify non-empty
        for (CsvFileSpec fileSpec : CsvFileSpec.values()) {
            Object object = getData(fileSpec);
            if (object instanceof List<?>) {
                assertFalse((((List<?>) object)).isEmpty());
            } else if (object instanceof Map<?, ?>) {
                assertFalse((((Map<?, ?>) object)).isEmpty());
            } else {
                assertNotNull(object);
            }
        }
        // verify each, including references to and from others
        for (CsvFileSpec csvFileSpec : CsvProcessor.getCsvDataCache().keySet()) {
            Logger.getLogger(ParserTest.class.getName()).log(Level.INFO, String.format("Verifying data from csv: %s", csvFileSpec.toString()));
            CsvProcessor.CsvParserWrapper cfsp = csvFileSpec.getParser();
            // check each Parser's class definition to determine which flavor of CsvProcessor.CsvParserWrapper this is.
            //
            // For example, if csvFileSpec="SUPPORTARC", then see
            // SupportArcParser.java:30:public class SupportArcParser implements RowMapParser<String, SupportArc>
            // In this case, cfsp will be of type SupportArcParser, or more generally, RowMapParser<String,SupportArc>
            // which extends CsvProcessor.CsvParserWrapper<Map<String, SupportArc>>
            //
            // Similarly, if csvFileSpec="SUPPLYTYPE", then see
            // SupplyTypeParser.java:31:public class SupplyTypeParser implements RowParser<LoadSupplyDefs>
            // In this case, cfsp will be of type SupplyTypeParser,
            // which is a specialization of RowParser<T> which extends CsvProcessor.CsvParserWrapper<T>.
            // specifically CsvProcessor.CsvParserWrapper<LoadSupplyDefs>
            //
            // In CsvProcessor.java, one can see that there are three signatures:
            //  RowParser<T>
            //  RowListParser<T>
            //  RowMapParser<K, V>
            // each instantiated for various T, K or V types.
            //
            // So the T in CsvProcessor.CsvParserWrapper<T> depends on the runtime value of the string csvFileSpec.
            // The compiler cannot check it and warns that 'cfsp' is a 'raw type' without any <T> known at compile time.
            //
            // And that is why the following unchecked conversion warning would be difficult to eliminate.
            List<String> newErrors = cfsp.verifyData(); // unchecked conversion because 'cfsp' has raw type



            // This test is supposed to run on error-free data
            // and verify that no false errors are detected.
            // If you want to test correct detection of known errors,
            // a test for that purpose and erroneous data will be required.
            assertEquals(0, newErrors.size());

            errorLogs.addAll(newErrors);
        }
        int i = 0;
        for (String log : errorLogs) {
            Logger.getLogger(ParserTest.class.getName()).log(Level.INFO, String.format("%04d %s", i, log));
            i++;
        }
    }
}

/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
