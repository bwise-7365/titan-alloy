/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import groupw.Logistics.Parsers.CsvProcessor.CsvParserWrapper;
import groupw.Logistics.Parsers.CsvProcessor.RowListParser;
import java.util.HashMap;
import java.util.Map;
import java.util.function.Function;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;

public enum CsvFileSpec {

    LOGCONFIGURATION(new LogConfigParser()),
    DOMAINS(new DomainParser()),
    LOGDISTARC(new LogDistArcParser()),
    LOGDISTNODE(new LogDistNodeParser()),
    SUPPORTARC(new SupportArcParser()),
    SUPPORTNODE(new SupportNodeParser()),
    VEHICLETYPE(new VehicleTypeParser()),
    VEHICLELOAD(new VehicleLoadParser()),
    SUPPLYTYPE(new SupplyTypeParser()),
    UNITTYPE(new UnitTypeParser()),
    UNIT(new UnitParser()),
    POSTURE(new PostureParser());

    private final CsvParserWrapper parser;

    //FileNames
    public static final String DOMAINS_FILE = "domains";
    public static final String LOGDISTARC_FILE = "logdistarc";
    public static final String LOGDISTNODE_FILE = "logdistnode";
    public static final String SUPPORTARC_FILE = "logsupportarc";
    public static final String SUPPORTNODE_FILE = "logsupportnode";
    public static final String VEHICLELOAD_FILE = "vehicleload";
    public static final String VEHICLETYPE_FILE = "vehicletype";
    public static final String SUPPLYTYPE_FILE = "supplytype";
    public static final String UNITTYPE_FILE = "unittype";
    public static final String UNIT_FILE = "unit";

    // Mapping of the csv file spec to the default file names
    public static final Map<CsvFileSpec, String> csvFileMap = new HashMap<>();

    //Header Constants
    public static final String NAME_HEADER = "Name";
    public static final String OPEN_HEADER = "Open";
    public static final String COORDINATES_HEADER = "Coordinates";
    public static final String INTERMEDIATE_COORDINATES_HEADER = "IntermediateCoordinates";
    public static final String SUPPLY_TYPE_HEADER = "SupplyType";
    public static final String LOG_NODE_SOURCE_HEADER = "LogNodeSource";
    public static final String LOG_NODE_TARGET_HEADER = "LogNodeTarget";
    public static final String MAX_SPEED_HEADER = "MaxSpeed";
    
    //domain.csv
    //posture.csv
    public static final String DOMAINS_HEADER = "Domains";
    public static final String POSTURE_HEADER = "Posture";
    public static final String[] DOMAINS_HEADERS = {
        DOMAINS_HEADER
    };
    public static final String[] POSTURE_HEADERS = {
        POSTURE_HEADER   
    };

    //logdistarc.csv
    public static final String TRUE_LENGTH_HEADER = "TrueLength";
    public static final String MAX_THROUGHPUT_HEADER = "MaxThroughput";
    public static final String MAX_LOAD_HEADER = "MaxLoad";
    public static final String[] LOG_DIST_ARC_HEADERS = {
        NAME_HEADER,
        OPEN_HEADER,
        INTERMEDIATE_COORDINATES_HEADER,
        DOMAINS_HEADER,
        MAX_SPEED_HEADER,
        TRUE_LENGTH_HEADER,
        MAX_THROUGHPUT_HEADER,
        MAX_LOAD_HEADER,
        LOG_NODE_SOURCE_HEADER,
        LOG_NODE_TARGET_HEADER
    };

    //logdistnode.csv
    //public static final String MAX_STORAGE_CAPACITY_SUPPLY_HEADER = "MaximumStorageCapacitySupplyType";
    //public static final String MAX_THROUGHPUT_RATE_SUPPLY_HEADER = "MaximumThroughputRateSupplyType";
    public static final String MAX_STORAGE_CAPACITY_SUPPLY_HEADER = "StorageCapacity";
    public static final String MAX_THROUGHPUT_RATE_SUPPLY_HEADER = "Throughput";

    // The following two have not yet been used
    //public static final String MAX_STORAGE_CAPACITY_SUPPLY_QTY_HEADER = "MaximumStorageCapacitySupplyQuantity";
    //public static final String MAX_THROUGHTPUT_RATE_SUPPLY_QTY_HEADER = "MaximumThroughputRateSupplyQuantity";

    public static final String[] LOG_DIST_NODE_HEADERS = {
        NAME_HEADER,
        OPEN_HEADER,
        COORDINATES_HEADER,
    };

    //logsupportarc.csv
    public static final String[] LOG_C2_ARC_HEADERS = {
        NAME_HEADER,
        LOG_NODE_SOURCE_HEADER,
        LOG_NODE_TARGET_HEADER
    };
    //logc2node.csv
    public static final String LOG_DIST_NODE_NAME_HEADER = "LogDistNodeName";
    public static final String LOG_C2_DISPATCHER_HEADER = "Dispatcher";
    public static final String[] LOG_C2_NODE_HEADERS = {
        NAME_HEADER,
        LOG_DIST_NODE_NAME_HEADER,
        LOG_C2_DISPATCHER_HEADER
    };

    //vehicletype.csv
    public static final String VEHICLE_TYPE_HEADER = "VehicleType";
    public static final String VEHICLE_AVAILABILITY_HEADER = "Availability";
    public static final String FUEL_TYPE_HEADER = "FuelType";
    public static final String FUEL_EFFICIENCY_HEADER = "FuelEfficiency";
    public static final String MAX_RANGE_HEADER = "MaxRange";

    public static final String[] VEHICLE_TYPE_HEADERS = {
        VEHICLE_TYPE_HEADER,
        VEHICLE_AVAILABILITY_HEADER,
        FUEL_TYPE_HEADER,
        FUEL_EFFICIENCY_HEADER,
        MAX_RANGE_HEADER,
        DOMAINS_HEADER,
        MAX_SPEED_HEADER
    };

    //vehicleload.csv
    public static final String[] VEHICLE_LOAD_HEADERS = {
        NAME_HEADER,
        VEHICLE_TYPE_HEADER
    };

    //supplytype.csv
    public static final String WEIGHT_HEADER = "Weight";

    public static final String[] SUPPLY_TYPE_HEADERS = {
        SUPPLY_TYPE_HEADER,
        WEIGHT_HEADER
    };

    //unittype.csv
    public static final String UNIT_TYPE_HEADER = "UnitType";
    public static final String[] UNIT_TYPE_HEADERS = {
        UNIT_TYPE_HEADER,
        POSTURE_HEADER,
        SUPPLY_TYPE_HEADER
    };

    //unit.csv
    public static final String[] UNIT_HEADERS = {
        NAME_HEADER,
        UNIT_TYPE_HEADER
    };
    
    public static final Map<String, Function<String, ?>> CSV_CLASS_MAPPING = new HashMap<>();

    //Data type mappings
    // This is an anonymous block of code, run once at program startup,
    // that builds the static csvFileMap object
    static {
        csvFileMap.put(CsvFileSpec.DOMAINS, CsvFileSpec.DOMAINS_FILE);
        csvFileMap.put(CsvFileSpec.LOGDISTARC, CsvFileSpec.LOGDISTARC_FILE);
        csvFileMap.put(CsvFileSpec.LOGDISTNODE, CsvFileSpec.LOGDISTNODE_FILE);
        csvFileMap.put(CsvFileSpec.SUPPORTARC, CsvFileSpec.SUPPORTARC_FILE);
        csvFileMap.put(CsvFileSpec.VEHICLELOAD, CsvFileSpec.VEHICLELOAD_FILE);
        csvFileMap.put(CsvFileSpec.SUPPLYTYPE, CsvFileSpec.SUPPLYTYPE_FILE);
        csvFileMap.put(CsvFileSpec.SUPPORTNODE, CsvFileSpec.SUPPORTNODE_FILE);
        csvFileMap.put(CsvFileSpec.UNITTYPE, CsvFileSpec.UNITTYPE_FILE);
        csvFileMap.put(CsvFileSpec.UNIT, CsvFileSpec.UNIT_FILE);
        csvFileMap.put(CsvFileSpec.VEHICLETYPE, CsvFileSpec.VEHICLETYPE_FILE);
        csvFileMap.put(CsvFileSpec.POSTURE, CsvFileSpec.DOMAINS_FILE);
    }

    CsvFileSpec(CsvParserWrapper parser) {
        this.parser = parser;
    }

    CsvFileSpec(RowListParser<?> parser) {
        this.parser = parser;
    }

    <K, V> CsvFileSpec(RowMapParser<K, V> parser) {
        this.parser = parser;
    }

    public CsvParserWrapper getParser() {
        return parser;
    }

}

/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
