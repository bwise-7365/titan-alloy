/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

/*
Log Support Node data is parsed incorrectly.

SupportNode objects have name=="no name", when they should have the name of the unit.

SupportNode objects have "Dispatcher" as a type of vehicle in the (vehicle, quantity) set.

The matching of vehicle types and vehicles quantities is "off by one", probably because it 'considers' "Dispatcher"
to be a vehicle-type name. When parsing Data2\logsupportnode.csv, this means SD-1 has 20 FWA-Large, 1 Ship-Small
and 0 Ship-Medium, when it should be 0 FWA-Large, 20 Ship-Small and 1 Ship-Medium.
 */


import groupw.Logistics.LogSprtNW.SupportNode;
import groupw.Logistics.LogDistNW.LogDistNode;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_C2_DISPATCHER_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_C2_NODE_HEADERS;
import static groupw.Logistics.Parsers.CsvFileSpec.LOG_DIST_NODE_NAME_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.NAME_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.SUPPORTNODE_FILE;

import groupw.Logistics.Unit;
import groupw.Network.NWUtils.Tuple2;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;
import java.util.Arrays;

/**
 *
 * @author DavidHa
 */
public class SupportNodeParser implements RowMapParser<String, SupportNode> {
    
    @Override
    public Map<String, SupportNode> parse(CSVParser Parser) throws DuplicateItemException {
        Map<String, SupportNode> logC2NodeCache = new HashMap<>();
        Map<String, Integer> csvHeaderMap = Parser.getHeaderMap();
        CsvProcessor.validateHeaders(SUPPORTNODE_FILE, csvHeaderMap, Arrays.asList(LOG_C2_NODE_HEADERS)); 
        int numNonVehicleFields = 3; // avoid 'magic numbers' that will get out of sync
        List<CSVRecord> records = Parser.getRecords();
        for (CSVRecord record : records) {
            String unitName = record.get(csvHeaderMap.get(NAME_HEADER));
            SupportNode supportNode = null;
            if (logC2NodeCache.get(unitName) == null) {
                supportNode = new SupportNode();
                logC2NodeCache.put(unitName, supportNode);
            } else {
                //Fail here as we don't allow duplicates.
                throw new DuplicateItemException("Found duplicate key in logsupportnode.csv for " + unitName);
            }

            supportNode.unitName =unitName;

            String logDistNodeName = record.get(csvHeaderMap.get(LOG_DIST_NODE_NAME_HEADER));
            supportNode.setLogDistNodeName(logDistNodeName);
           
            String dispatcherName = record.get(csvHeaderMap.get(LOG_C2_DISPATCHER_HEADER));
            supportNode.setDispatcherName(dispatcherName);
            
            Map<String, Integer> vehicleSet = new HashMap<>();
            List<String> recordSubList = record.toList().subList(numNonVehicleFields, record.toList().size());
            if (recordSubList.size() % 2 != 0) {
                //fail
            }
            for (int i = 0; i < recordSubList.size(); i++) {
                String vehicleName = Parser.getHeaderNames().get(i + numNonVehicleFields);
                String vehicleQtyRecord = recordSubList.get(i);
                double vehicleQty = vehicleQtyRecord.isEmpty() ? 0.0 : Double.parseDouble(vehicleQtyRecord);
                vehicleSet.put(vehicleName, (int)vehicleQty);
            }
            supportNode.vehicles = vehicleSet;
        }
        return logC2NodeCache;
    }

    @Override
    public List<String> verifyData() {
        List<String> errorLogs = new ArrayList<>();
        Map<String, SupportNode> logC2Nodes = (Map<String, SupportNode>) CsvProcessor.csvDataCache.get(CsvFileSpec.SUPPORTNODE);
        Map<String, LogDistNode> logDistNodes = (Map<String, LogDistNode>) CsvProcessor.csvDataCache.get(CsvFileSpec.LOGDISTNODE);

        for (SupportNode supportNode : logC2Nodes.values()) {
            String logDistNodeRecord = supportNode.getLogDistNodeName();
            if (!logDistNodeRecord.isEmpty() && !logDistNodes.containsKey(logDistNodeRecord)) {
                String errorLog = String.format("Error: Unable to find LogDistNode : %s for LogC2Node: %s", logDistNodeRecord, supportNode.unitName);
                errorLogs.add(errorLog);
            } else {
                supportNode.setLogDistNode(logDistNodes.get(logDistNodeRecord));
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
