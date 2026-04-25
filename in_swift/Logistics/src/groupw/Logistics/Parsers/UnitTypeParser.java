/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import groupw.Logistics.LoadSupplyDefs;
import groupw.Logistics.Manifest;

import static groupw.Logistics.Parsers.CsvFileSpec.POSTURE_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.UNITTYPE_FILE;
import static groupw.Logistics.Parsers.CsvFileSpec.UNIT_TYPE_HEADER;
import groupw.Logistics.UnitType;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;
import groupw.Network.NWUtils.Tuple;
import java.util.Arrays;

/**
 *
 * @author DavidHa
 */
public class UnitTypeParser implements RowMapParser<String, UnitType> {

    @Override
    public Map<String, UnitType> parse(CSVParser Parser) throws DuplicateItemException {
        Map<String, UnitType> unitTypeCache = new HashMap<>();
        Map<String, Integer> csvHeaderMap = Parser.getHeaderMap();
        List<CSVRecord> records = Parser.getRecords();

        CsvProcessor.validateHeaders(UNITTYPE_FILE, csvHeaderMap, Arrays.asList(UNIT_TYPE_HEADER));

        for (CSVRecord record : records) {
            UnitType unitType = null;
            String unitTypeName = record.get(csvHeaderMap.get(UNIT_TYPE_HEADER));
            if (unitTypeCache.get(unitTypeName) == null) {
                unitType = new UnitType();
                unitType.setTypeName(unitTypeName);
                unitTypeCache.put(unitTypeName, unitType);
            }
            unitType = unitTypeCache.get(unitTypeName);
            String unitPosture = record.get(csvHeaderMap.get(POSTURE_HEADER));
            if (unitType.getPostures().contains(unitPosture)) {
                throw new DuplicateItemException(String.format("Found duplicate posture type %s for unit type %s.", unitPosture, unitTypeName));
            }
            unitType.getPostures().add(unitPosture);
            List<String> recordSubList = record.toList().subList(2, record.toList().size());

            // unitType.dcRates might be empty, but it is never NULL (see class definition)
            if (null == unitType.dcRates.get(unitPosture)) {
                unitType.dcRates.put(unitPosture, new Manifest());
            }
            Manifest dcrs = unitType.dcRates.get(unitPosture);
            for (int i = 0; i < recordSubList.size(); i++) {
                String supplyTypeHeader = Parser.getHeaderNames().get(i + 2); // keep case
                String supplyTypeCRRecord = recordSubList.get(i);
                double supplyTypeCR = supplyTypeCRRecord.isEmpty() ? 0.0 : Double.parseDouble(supplyTypeCRRecord);
                if (supplyTypeCR > 0.0) {
                    dcrs.addInventory(supplyTypeHeader, supplyTypeCR);
                }
            }
        }
        return unitTypeCache;
    }

    @Override
    public List<String> verifyData() {
        List<String> errorLogs = new ArrayList<>();
        Map<String, UnitType> unitTypes = (Map<String, UnitType>) CsvProcessor.csvDataCache.get(CsvFileSpec.UNITTYPE);
        List<String> postures = (List<String>) CsvProcessor.csvDataCache.get(CsvFileSpec.POSTURE);
        LoadSupplyDefs lsd = (LoadSupplyDefs) CsvProcessor.csvDataCache.get(CsvFileSpec.SUPPLYTYPE);
        Map<String, Tuple> supplyTypes = lsd.supplies;

        for (UnitType unitType : unitTypes.values()) {
            for (String posture : unitType.getPostures()) {
                if (!postures.contains(posture)) {
                    String errorLog = String.format("Error UnitType Posture: %s was assigned but is not defined in the domains csv for postures.", posture);
                    errorLogs.add(errorLog);
                }
            }

            for (Map.Entry<String, Manifest> entry : unitType.dcRates.entrySet()) {
                String p = entry.getKey();
                if (!unitType.getPostures().contains(p)) {
                    String errorLog = String.format("Error UnitType Posture: %s was used in UnitType consumption rates but is not defined for that UnitTYpe.", p);
                    errorLogs.add(errorLog);
                }
                Manifest m = entry.getValue();
                Set<String> unitSupplyDefinition = m.getItemNames();
                for (String supply : unitSupplyDefinition) {
                    if (supplyTypes.get(supply) == null) {
                        String errorLog = String.format("Error UnitType: %s was assigned the supply type: %s which is not defined in the supply type csv.", unitType.getTypeName(), supply);
                        errorLogs.add(errorLog);
                    }
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
