/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import groupw.Logistics.LogSprtNW.SupportNode;
import groupw.Logistics.LogSprtNW.SupportArc;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_C2_ARC_HEADERS;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_NODE_SOURCE_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_NODE_TARGET_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.NAME_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.SUPPORTARC_FILE;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;
import java.util.Arrays;

/**
 * Parses the LOGC2Arc csv
 *
 * @author DavidHa
 */
public class SupportArcParser implements RowMapParser<String, SupportArc> {

    @Override
    public Map<String, SupportArc> parse(CSVParser csvParser) throws DuplicateItemException {
        Map<String, SupportArc> supportsArcCache = new HashMap<>(); // LOGC2NW
        Map<String, Integer> headerMap = csvParser.getHeaderMap();
        CsvProcessor.validateHeaders(SUPPORTARC_FILE, headerMap, Arrays.asList(LOG_C2_ARC_HEADERS));

        for (CSVRecord record : csvParser.getRecords()) {
            String name = record.get(headerMap.get(NAME_HEADER));
            SupportArc supportArc = null;
            if (supportsArcCache.get(name) == null) {
                supportArc = new SupportArc();
                supportsArcCache.put(name, supportArc);
            } else {
                throw new DuplicateItemException("Found a duplicate LogSupportArc: " + name);
            }
            String logNodeSource = record.get(headerMap.get(LOG_NODE_SOURCE_HEADER));
            String logNodeTarget = record.get(headerMap.get(LOG_NODE_TARGET_HEADER));

            supportArc.name = name;
            supportArc.srcLogDistNode = logNodeSource;
            supportArc.tgtLogDistNode = logNodeTarget;
        }

        return supportsArcCache;
    }

    @Override
    public List<String> verifyData() {
        List<String> errorLogs = new ArrayList<>();
        Map<String, SupportArc> logC2Arcs = (Map<String, SupportArc>) CsvProcessor.csvDataCache.get(CsvFileSpec.SUPPORTARC);

        Map<String, SupportNode> logC2Nodes = (Map<String, SupportNode>) CsvProcessor.csvDataCache.get(CsvFileSpec.SUPPORTNODE);

        for (SupportArc supportArc : logC2Arcs.values()) {
            String logC2ArcName = supportArc.name;
            String logSupportSource = supportArc.srcLogDistNode;
            String logSupportTarget = supportArc.tgtLogDistNode;
            if (!logSupportSource.isEmpty() && !logC2Nodes.containsKey(logSupportSource)) {
                String errorLog = String.format("Missing source LogC2Node: " + logSupportSource + " for LogC2Arc: %s \n", logC2ArcName);
                errorLogs.add(errorLog);
            }

            if (!logSupportTarget.isEmpty() && !logC2Nodes.containsKey(logSupportTarget)) {
                String errorLog = String.format("Missing target LogDistNode: " + logSupportTarget + " for LogC2Arc: %s \n", logC2ArcName);
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
