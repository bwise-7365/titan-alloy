/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import static groupw.Logistics.Parsers.CsvFileSpec.DOMAINS_FILE;
import static groupw.Logistics.Parsers.CsvFileSpec.POSTURE_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.POSTURE_HEADERS;
import groupw.Logistics.Parsers.CsvProcessor.RowListParser;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;

/**
 *
 * @author DavidHa
 */
public class PostureParser implements RowListParser<String>{
    public PostureParser(){}

    @Override
    public List<String> parse(CSVParser csvParser) throws DuplicateItemException {
        Set<String> postureSet = new HashSet<>();
        Map<String, Integer> headerMap = csvParser.getHeaderMap();
        CsvProcessor.validateHeaders(DOMAINS_FILE, headerMap, Arrays.asList(POSTURE_HEADERS));
        if(headerMap.get(POSTURE_HEADER) != null){
            int col = headerMap.get(POSTURE_HEADER);
            CSVRecord record = csvParser.getRecords().get(0);
            String[] postures = record.get(col).split(",");
            postureSet.addAll(Arrays.asList(postures));    
        } 
        
        return new ArrayList<>(postureSet);
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
