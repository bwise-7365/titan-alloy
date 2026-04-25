/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import groupw.Logistics.LoadSupplyDefs;
import static groupw.Logistics.Parsers.CsvFileSpec.SUPPLYTYPE_FILE;
import static groupw.Logistics.Parsers.CsvFileSpec.SUPPLY_TYPE_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.SUPPLY_TYPE_HEADERS;
import static groupw.Logistics.Parsers.CsvFileSpec.WEIGHT_HEADER;
import groupw.Network.NWUtils.Tuple;
import groupw.Network.NWUtils.Tuple2;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import groupw.Logistics.Parsers.CsvProcessor.RowParser;
import java.util.Arrays;
import java.util.logging.Level;
import java.util.logging.Logger;

/**
 *
 * @author DavidHa
 */
public class SupplyTypeParser implements RowParser<LoadSupplyDefs> {

    @Override
    public LoadSupplyDefs parse(CSVParser Parser) throws DuplicateItemException{
        Map<String, Tuple> supplyTypeCache = new HashMap<>();
        Map<String, Integer> headerMap = Parser.getHeaderMap();
        CsvProcessor.validateHeaders(SUPPLYTYPE_FILE, headerMap, Arrays.asList(SUPPLY_TYPE_HEADERS));

        for (CSVRecord record : Parser.getRecords()) {
            String supplyTypeName = record.get(headerMap.get(SUPPLY_TYPE_HEADER)); // keep case
            Tuple supplyType = null;
            double weight = Double.parseDouble(record.get(headerMap.get(WEIGHT_HEADER)));
            if (supplyTypeCache.get(supplyTypeName) == null) {
                supplyType = new Tuple2<>(supplyTypeName, weight);
                supplyTypeCache.put(supplyTypeName, supplyType);
            } else {
                throw new DuplicateItemException("Found duplicate supply type: " + supplyTypeName);
            }
        }
        return new LoadSupplyDefs(supplyTypeCache);
    }

    @Override
    public List<String> verifyData() {
        List<String> errorLogs = new ArrayList<>();
        try {
            LoadSupplyDefs lsd = (LoadSupplyDefs) CsvProcessor.getData(CsvFileSpec.SUPPLYTYPE);
            if(lsd == null || lsd.supplies.isEmpty()){
                errorLogs.add("Loaded supply definition is empty. Please check the supply def csv.");
            }
        } catch (DuplicateItemException ex) {
            Logger.getLogger(SupplyTypeParser.class.getName()).log(Level.SEVERE, null, ex);
        }

        return errorLogs;
    }
}

/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
