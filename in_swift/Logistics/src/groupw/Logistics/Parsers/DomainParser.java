/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import static groupw.Logistics.Parsers.CsvFileSpec.DOMAINS_FILE;
import static groupw.Logistics.Parsers.CsvFileSpec.DOMAINS_HEADER;
import static groupw.Logistics.Parsers.CsvFileSpec.DOMAINS_HEADERS;
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
public class DomainParser implements RowListParser<String>{
    public DomainParser(){}

    @Override
    public List<String> parse(CSVParser csvParser) throws DuplicateItemException {
        Set<String> domainsSet = new HashSet<>();
        Map<String, Integer> headerMap = csvParser.getHeaderMap();
        CsvProcessor.validateHeaders(DOMAINS_FILE, headerMap, Arrays.asList(DOMAINS_HEADERS));
        if(headerMap.get(DOMAINS_HEADER) != null){
            int col = headerMap.get(DOMAINS_HEADER);
            CSVRecord record = csvParser.getRecords().get(col);
            String[] domains = record.get(col).split(",");
            domainsSet.addAll(Arrays.asList(domains));    
        } 
        
        return new ArrayList<>(domainsSet);
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
