/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import groupw.Logistics.LogDistNW.LogDistArc;
import groupw.Logistics.LogDistNW.LogDistNode;
import static groupw.Logistics.Parsers.CsvFileSpec.DOMAINS_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.INTERMEDIATE_COORDINATES_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.LOGDISTARC_FILE;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_DIST_ARC_HEADERS;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_NODE_SOURCE_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_NODE_TARGET_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.MAX_LOAD_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.MAX_SPEED_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.MAX_THROUGHPUT_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.NAME_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.OPEN_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.TRUE_LENGTH_HEADER;
import groupw.Network.NWUtils.Tuple;
import groupw.Network.NWUtils.Tuple3;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;

/**
 *
 * @author DavidHa
 */
public class LogDistArcParser implements RowMapParser<String, LogDistArc> {

    @Override
    public Map<String, LogDistArc> parse(CSVParser csvParser) throws DuplicateItemException {

        Map<String, LogDistArc> logDistArcCache = new HashMap<>();
        Map<String, Integer> headerMap = csvParser.getHeaderMap();
        CsvProcessor.validateHeaders(LOGDISTARC_FILE, headerMap, Arrays.asList(LOG_DIST_ARC_HEADERS));
        for (CSVRecord record : csvParser.getRecords()) {
            String name = record.get(headerMap.get(NAME_HEADER));
            LogDistArc logDistArc = null;
            if (logDistArcCache.get(name) == null) {
                logDistArc = new LogDistArc();
                logDistArcCache.put(name, logDistArc);
            } else {
                throw new DuplicateItemException("Found a duplicate LogDistArc: " + name);
            }

            logDistArc = logDistArcCache.get(name);
            double maxSpeed = Double.parseDouble(record.get(headerMap.get(MAX_SPEED_HEADER)));

            double trueLength = Double.parseDouble(record.get(headerMap.get(TRUE_LENGTH_HEADER)));

            double throughDaily = Double.parseDouble(record.get(headerMap.get(MAX_THROUGHPUT_HEADER)));

            double loadMax = Double.parseDouble(record.get(headerMap.get(MAX_LOAD_HEADER)));

            boolean open = Boolean.parseBoolean(record.get(headerMap.get(OPEN_HEADER)));
            String domain = record.get(headerMap.get(DOMAINS_HEADER));

            String logNodeSource = record.get(headerMap.get(LOG_NODE_SOURCE_HEADER));
            String logNodeTarget = record.get(headerMap.get(LOG_NODE_TARGET_HEADER));
            String[] intermediateCoords = record.get(headerMap.get(INTERMEDIATE_COORDINATES_HEADER)).split(",");
            List<Tuple> intermediateCoordinates = new ArrayList<>();
            
            double coordA;
            double coordB;
            double coordC = 0.0;
            if (intermediateCoords.length % 3 == 0) {
                for (int i = 0; i < intermediateCoords.length; i = i + 3) {
                    coordA = Double.parseDouble(intermediateCoords[i]);
                    coordB = Double.parseDouble(intermediateCoords[i + 1]);
                    coordC = Double.parseDouble(intermediateCoords[i + 2]);
                    intermediateCoordinates.add(new Tuple3<>(coordA, coordB, coordC));
                }
            } else if (intermediateCoords.length % 2 == 0) {
                for (int i = 0; i < intermediateCoords.length; i = i + 2) {
                    coordA = Double.parseDouble(intermediateCoords[i]);
                    coordB = Double.parseDouble(intermediateCoords[i + 1]);
                    intermediateCoordinates.add(new Tuple3<>(coordA, coordB, coordC));
                }
            } else if ((intermediateCoords.length % 3) != 0
                    && (intermediateCoords.length % 2) != 0
                    && (intermediateCoords.length != 1)) {
                Logger.getLogger(LogDistArcParser.class.getName()).log(Level.SEVERE, String.format("Error parsing the log dist arc file. The coordinates provided %s is not divisible by two or three. The latitude and longitude could not correctly be parsed.", Arrays.toString(intermediateCoords)));
            }
            logDistArc.name = name;
            logDistArc.maxSpeed = maxSpeed;
            logDistArc.loadMax = loadMax;
            logDistArc.trueLength = trueLength;
            logDistArc.throughDaily = throughDaily;
            logDistArc.open = open;
            logDistArc.srcNodeName = logNodeSource;
            logDistArc.tgtNodNamee = logNodeTarget;
            logDistArc.domain = domain;
            logDistArc.intermediateCoordinates = intermediateCoordinates;
        }

        return logDistArcCache;
    }

    @Override
    public List<String> verifyData() {
        List<String> errorLogs = new ArrayList<>();
        Map<String, LogDistNode> logDistNodes = (Map<String, LogDistNode>) CsvProcessor.csvDataCache.get(CsvFileSpec.LOGDISTNODE);
        Map<String, LogDistArc> logDistArcs = (Map<String, LogDistArc>) CsvProcessor.csvDataCache.get(CsvFileSpec.LOGDISTARC);
        for (LogDistArc logDistArc : logDistArcs.values()) {
            String logDistArcName = logDistArc.name;
            String logNodeSource = logDistArc.srcNodeName;
            String logNodeTarget = logDistArc.tgtNodNamee;
            String errorLog = ""; 
            if(!logNodeSource.isEmpty() && !logDistNodes.containsKey(logNodeSource)){
                    errorLog = errorLog + String.format("Missing source LogDistNode: " + logNodeSource +  " for LogC2Arc: %s \n", logDistArcName);
                    errorLogs.add(errorLog);
            }
            if(!logNodeTarget.isEmpty() && !logDistNodes.containsKey(logNodeTarget)){
                    errorLog = errorLog + String.format("Missing target LogDistNode: " + logNodeTarget + " for LogC2Arc: %s \n", logDistArcName);
                    errorLogs.add(errorLog);
            }
        }
        return errorLogs;
    }

}
/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
