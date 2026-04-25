/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import groupw.Logistics.LoadSupplyDefs;
import groupw.Logistics.LogDistNW.LogDistNode;
import groupw.Logistics.Manifest;

import static groupw.BaseSim.DSUtils.makeRV3;
import static groupw.Logistics.Parsers.CsvFileSpec.COORDINATES_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.LOGDISTNODE_FILE;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_DIST_NODE_HEADERS;
import static groupw.Logistics.Parsers.CsvFileSpec.MAX_STORAGE_CAPACITY_SUPPLY_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.MAX_THROUGHPUT_RATE_SUPPLY_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.NAME_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.OPEN_HEADER;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;
import groupw.Network.NWUtils.Tuple;

/**
 *
 * @author DavidHa
 */
public class LogDistNodeParser implements RowMapParser<String, LogDistNode> {

    @Override
    public Map<String, LogDistNode> parse(CSVParser csvParser) throws DuplicateItemException {

        Map<String, LogDistNode> logDistNodeCache = new HashMap<>();
        Map<String, Integer> headerMap = csvParser.getHeaderMap();
        CsvProcessor.validateHeaders(LOGDISTNODE_FILE, headerMap, Arrays.asList(LOG_DIST_NODE_HEADERS));
        Pattern pattern = Pattern.compile("(\\w+)(StorageCapacity|Throughput)", Pattern.CASE_INSENSITIVE);

        List<CSVRecord> records = csvParser.getRecords();
        for (CSVRecord record : records) {
            String name = record.get(headerMap.get(NAME_HEADER));
            String openRecord = record.get(headerMap.get(OPEN_HEADER));
            String coordinatesRecord = record.get(headerMap.get(COORDINATES_HEADER));

            LogDistNode logDistNode = null;
            if (logDistNodeCache.get(name) == null) {
                logDistNode = new LogDistNode(name);
                logDistNodeCache.put(name, logDistNode);
            } else {
                throw new DuplicateItemException("Found a duplicate LogDistNode: " + name);
            }

            boolean open = openRecord.isEmpty() ? true : Boolean.parseBoolean(openRecord);
            logDistNode.open = open;
            String[] coords = coordinatesRecord.isEmpty() ? new String[]{} : coordinatesRecord.split(",");
            if (coords.length % 3 != 0 && coords.length % 2 != 0) {
                Logger.getLogger(LogDistArcParser.class.getName()).log(Level.SEVERE, String.format("Error parsing the log dist arc file. The coordinates provided %s is not divisible by three or by two. The latitude and longitude could not correctly be parsed.", Arrays.toString(coords)));
            } else {
                double coordA = coords[0].isEmpty() ? 0.0 : Double.parseDouble(coords[0]);
                double coordB = coords[1].isEmpty() ? 0.0 : Double.parseDouble(coords[1]);
                double coordC = 0.0; 
                if (coords.length % 3 == 0) {
                    coordC = coords[2].isEmpty() ? 0.0 : Double.parseDouble(coords[2]);
                }
                logDistNode.coords = makeRV3(coordA, coordB, coordC);
            }

            if (null == logDistNode.storeMax) {
                logDistNode.storeMax =  new Manifest();
            }
            if (null == logDistNode.throughDaily) {
                logDistNode.throughDaily =  new Manifest();
            }

            for (String header : csvParser.getHeaderMap().keySet()) {
                Matcher matcher = pattern.matcher(header);
                if (matcher.matches()) {
                    String typeStr;
                    String attrStr;
                    double qty;
                    try {
                        typeStr = matcher.group(1);
                        attrStr = matcher.group(2);
                        String value = record.get(header);
                        qty = value.isEmpty() ? 0.0 : Double.parseDouble(value);
                    } catch (IllegalArgumentException e) {
                        throw new IllegalArgumentException(e);
                    }
                    switch (attrStr) {
                        case MAX_STORAGE_CAPACITY_SUPPLY_HEADER:
                            logDistNode.storeMax.addInventory(typeStr, (Double)qty);
                            break;
                        case MAX_THROUGHPUT_RATE_SUPPLY_HEADER:
                            logDistNode.throughDaily.addInventory(typeStr, (Double)qty);
                            break;
                        default:
                            throw new IllegalArgumentException("Unrecognized LogDistNode attribute: " + attrStr);
                            //break;  // unreachable
                    }
                }
            }

        }
        return logDistNodeCache;
    }

    @Override
    public List<String> verifyData() {
        List<String> errorLogs = new ArrayList<>();
        Map<String, LogDistNode> logDistNodes = (Map<String, LogDistNode>) CsvProcessor.csvDataCache.get(CsvFileSpec.LOGDISTNODE);
        LoadSupplyDefs lsd = (LoadSupplyDefs) CsvProcessor.csvDataCache.get(CsvFileSpec.SUPPLYTYPE); 
        Map<String, Tuple> supplyTypes = lsd.supplies;

        for (LogDistNode logDistNode : logDistNodes.values()) {
            for (String manifestItem : logDistNode.storeMax.getItemNames()) {
                if (!supplyTypes.containsKey(manifestItem)) {
                    String errorLog = String.format("Error: was unable to find supply type %s for LogDistNode: %s in the supply type definition csv.", manifestItem, logDistNode.name);
                    errorLogs.add(errorLog);
                }
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
