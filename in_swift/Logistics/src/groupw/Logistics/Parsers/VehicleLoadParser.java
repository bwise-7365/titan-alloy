/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import groupw.Logistics.LoadSupplyDefs;
import static groupw.Logistics.Parsers.CsvFileSpec.NAME_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.VEHICLELOAD_FILE;
import static groupw.Logistics.Parsers.CsvFileSpec.VEHICLE_LOAD_HEADERS;
import static groupw.Logistics.Parsers.CsvFileSpec.VEHICLE_TYPE_HEADER;
import groupw.Logistics.VehicleLoad;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;
import java.util.Arrays;

/**
 *
 * @author DavidHa
 */
public class VehicleLoadParser implements RowMapParser<String, VehicleLoad> {

    @Override
    public Map<String, VehicleLoad> parse(CSVParser csvParser) {

        Map<String, VehicleLoad> vehicleLoadCache = new HashMap<>();
        Map<String, Integer> headerMap = csvParser.getHeaderMap();
   
        CsvProcessor.validateHeaders(VEHICLELOAD_FILE, headerMap, Arrays.asList(VEHICLE_LOAD_HEADERS));

        List<CSVRecord> records = csvParser.getRecords();
        for (CSVRecord record : records) {
            String vicName = record.get(headerMap.get(NAME_HEADER));
            String vicType = record.get(headerMap.get(VEHICLE_TYPE_HEADER));
            List<String> recordSubList = record.toList().subList(2, record.toList().size());

            List<String> supplyTypes = new ArrayList<>();
            List<Double> supplyQuantity = new ArrayList<>();
            for (int i = 0; i < recordSubList.size(); i++) {
                String supplyTypeHeader = csvParser.getHeaderNames().get(i + 2);
                supplyTypes.add(supplyTypeHeader);
                double supplyTypeQTY = recordSubList.get(i).isEmpty() ? 0.0 : Double.parseDouble(recordSubList.get(i));
                supplyQuantity.add(supplyTypeQTY);
            }

            VehicleLoad vicLoad;
            if (vehicleLoadCache.get(vicType) == null) {
                vicLoad = new VehicleLoad();
                vicLoad.name = vicName;
                vicLoad.vehicleType = vicType;
                vicLoad.sTypes = supplyTypes;
                vicLoad.load = supplyQuantity;
                vehicleLoadCache.put(vicName, vicLoad);
            }
        }
        return vehicleLoadCache;
    }

    @Override
    public List<String> verifyData() {
        List<String> errorLogs = new ArrayList<>();
        LoadSupplyDefs lsd = (LoadSupplyDefs) CsvProcessor.csvDataCache.get(CsvFileSpec.SUPPLYTYPE);
        Map<String, VehicleLoad> vehicleLoadMap = (Map<String, VehicleLoad>) CsvProcessor.csvDataCache.get(CsvFileSpec.VEHICLELOAD);
        for (VehicleLoad vl : vehicleLoadMap.values()) {
            for (String supplyType : vl.sTypes) { // keep case
                if (!lsd.supplies.keySet().contains(supplyType)) {
                    errorLogs.add(String.format("Error: VehicleLoad for VehicleType |%s| contains the supply type |%s| not defined in the supply type csv file.", vl.vehicleType, supplyType));
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
