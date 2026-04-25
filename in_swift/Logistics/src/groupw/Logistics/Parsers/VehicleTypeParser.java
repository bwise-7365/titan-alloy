/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import static groupw.Logistics.Parsers.CsvFileSpec.DOMAINS_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.FUEL_EFFICIENCY_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.FUEL_TYPE_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.MAX_RANGE_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.MAX_SPEED_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.VEHICLETYPE_FILE;
import static groupw.Logistics.Parsers.CsvFileSpec.VEHICLE_AVAILABILITY_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.VEHICLE_TYPE_HEADER;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;
import groupw.Logistics.VehicleType;

/**
 *
 * @author DavidHa
 */
public class VehicleTypeParser implements RowMapParser<String, VehicleType> {

    @Override
    public Map<String, VehicleType> parse(CSVParser csvParser) {
        Map<String, VehicleType> vehicleLoadCache = new HashMap<>();
        Map<String, Integer> headerMap = csvParser.getHeaderMap();
       
        //Validate Header before processing...
        CsvProcessor.validateHeaders(VEHICLETYPE_FILE, headerMap, Arrays.asList(CsvFileSpec.VEHICLE_TYPE_HEADERS));

        List<CSVRecord> records = csvParser.getRecords();
        for (CSVRecord record : records) {
            String vicType = record.get(headerMap.get(VEHICLE_TYPE_HEADER));
            String fuelType = record.get(headerMap.get(FUEL_TYPE_HEADER));
            double availability = Double.parseDouble(record.get(headerMap.get(VEHICLE_AVAILABILITY_HEADER)));
            double fuelEfficiency = Double.parseDouble(record.get(headerMap.get(FUEL_EFFICIENCY_HEADER)));
            double maxRange = Double.parseDouble(record.get(headerMap.get(MAX_RANGE_HEADER)));
            double maxSpeed = Double.parseDouble(record.get(headerMap.get(MAX_SPEED_HEADER)));
            String[] domains = record.get(headerMap.get(DOMAINS_HEADER)).split(",");

            VehicleType vehicleType = null;
            if (vehicleLoadCache.get(vicType) == null) {
                vehicleType = new VehicleType();
                vehicleType.vehicleTypeName = vicType;
                vehicleType.availability = availability;
                vehicleType.fuelEfficiency = fuelEfficiency;
                vehicleType.fuelType = fuelType;
                vehicleType.maxRange = maxRange;
                vehicleType.maxSpeed = maxSpeed;
                vehicleType.domains = new HashSet<>(Arrays.asList(domains));
                vehicleLoadCache.put(vicType, vehicleType);
            }
        }
        return vehicleLoadCache;
    }

    @Override
    public List<String> verifyData() {
        List<String> errorLogs = new ArrayList<>();
        List<String> domains = (List<String>) CsvProcessor.csvDataCache.get(CsvFileSpec.DOMAINS);
        Map<String, VehicleType> vehicleTypeMap = (Map<String, VehicleType>) CsvProcessor.csvDataCache.get(CsvFileSpec.VEHICLETYPE);
        List<VehicleType> vehicleTypes = new ArrayList<>(vehicleTypeMap.values());
        for (VehicleType vehicleType : vehicleTypes) {
            for (String domain : vehicleType.domains) {
                if (!domains.contains(domain)) {
                    errorLogs.add(String.format("Error: VehicleType: %s's domain: %s is not enumerated in the domain csv file.", vehicleType.vehicleTypeName, domain));
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
