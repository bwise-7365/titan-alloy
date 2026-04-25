/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import groupw.Logistics.Parsers.CsvProcessor.RowMapParser;

/**
 * Parses the log configuration file which contains the paths to the log network
 * csv files.
 * @author DavidHa
 */
public class LogConfigParser implements RowMapParser<CsvFileSpec, String> {

    @Override
    public Map<CsvFileSpec, String> parse(CSVParser Parser) {
        Map<CsvFileSpec, String> fileSpecMap = new HashMap<>();
        List<CSVRecord> records = Parser.getRecords();
        for (CSVRecord record : records) {
            String fileType = record.get(0);
            String filePath = record.get(1);
            for (Map.Entry<CsvFileSpec, String> fileSpecEntry : CsvFileSpec.csvFileMap.entrySet()) {
                if (fileType.equals(fileSpecEntry.getValue())) {
                    fileSpecMap.put(fileSpecEntry.getKey(), filePath);
                }
            }
        }
        return fileSpecMap;
    }

    @Override
    public List<String> verifyData() {
        return new ArrayList<>();
    }
}

/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
