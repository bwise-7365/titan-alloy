/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import static groupw.Logistics.Parsers.CsvFileSpec.NAME_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.UNIT_FILE;
import static groupw.Logistics.Parsers.CsvFileSpec.UNIT_HEADERS;
import static groupw.Logistics.Parsers.CsvFileSpec.UNIT_TYPE_HEADER;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;
import groupw.Logistics.Unit;
import groupw.Logistics.UnitType;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;

/**
 *
 * @author DavidHa
 */
public class UnitParser implements RowMapParser<String, Unit>{

    @Override
    public Map<String, Unit> parse(CSVParser csvParser) throws DuplicateItemException {
        Map<String, UnitType> unitTypeMap = (Map<String, UnitType>) CsvProcessor.getCsvDataCache().get(CsvFileSpec.UNITTYPE);
        Map<String, Unit> unitCache = new HashMap<>();
        Map<String, Integer> csvHeaderMap = csvParser.getHeaderMap();

        CsvProcessor.validateHeaders(UNIT_FILE, csvHeaderMap, Arrays.asList(UNIT_HEADERS));
        
        for(CSVRecord record : csvParser.getRecords()){
            String unitName = record.get(csvHeaderMap.get(NAME_HEADER));
            String unitTypeName = record.get(csvHeaderMap.get(UNIT_TYPE_HEADER));
            if(unitCache.get(unitName) != null){
                throw new DuplicateItemException(String.format("Error: Found a duplicate unit name %s. Please check the unit file.", unitName));
            }
            // unitCache.put(unit, new Unit(unit, new UnitType(unitTypeName)));
            // This ^^ would create an incomplete UnitType object with nothing but the name.
            // However, we can get the complete object from the unitTypeMap when it is not NULL
            UnitType ut = unitTypeMap.get(unitTypeName);
            Unit u = new Unit(unitName, ut); // so we can inspect it in the debugger
            // note that at this point u has no postureName, reorderLevel, etc.
            unitCache.put(unitName, u);
        }

        return unitCache;
        
    }

    @Override
    public List<String> verifyData() {
        List<String> errorLogs = new ArrayList<>();
        Map<String, UnitType> unitTypeMap = (Map<String, UnitType>) CsvProcessor.csvDataCache.get(CsvFileSpec.UNITTYPE);
        Map<String, Unit> unitMap = (Map<String, Unit>) CsvProcessor.csvDataCache.get(CsvFileSpec.UNIT);
        for(Unit unit : unitMap.values()){
            UnitType unitType = unit.getType();
            String unitTypeName = unitType.getTypeName();
            if(unitTypeMap.get(unitTypeName) == null){
                errorLogs.add(String.format("Error: Unable to find unit type: %s for %s. Please check the unit and unit type definition files.", unitTypeName, unit.getName()));
            }
            unit.setType(unitType);
        }
        return errorLogs;
    }
    
}

/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
