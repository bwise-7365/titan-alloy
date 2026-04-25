/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.Logistics.Parsers;

import groupw.Logistics.Parsers.CsvProcessor.CsvParserWrapper;
import java.awt.Component;
import java.io.File;
import java.io.FileInputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.Reader;
import java.nio.charset.StandardCharsets;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JFileChooser;
import javax.swing.filechooser.FileNameExtensionFilter;
import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVParser;
import org.apache.commons.csv.CSVRecord;
import org.apache.commons.io.input.BOMInputStream;


/**
 *
 * @author DavidHa
 */
public class Utilities {

    /*
    * Reads a csv file given the file path and parses with an implemented parser    
    * Uses a Byte Order Mark (BOMInputStream) because editors tend to apply a BOM    
    * which causes problems parsing using the apache commons csv libary. This 
    * ignores the BOM.
     */
    public static <T> T readCsv(String filePath, CsvParserWrapper<T> parser) throws DuplicateItemException {
        if (filePath == null) {
            Logger.getLogger(Utilities.class.getName()).log(Level.WARNING, String.format("Error: unable to find file path: %s. Please check the logconfiguration csv", parser.toString()));
            return null;
        }

        Logger.getLogger(Utilities.class.getName()).log(Level.INFO, String.format("Attempting to read ->%s<- \n", filePath));
        System.out.flush();
        CSVFormat format = CSVFormat.DEFAULT.builder()
                .setHeader()
                .setSkipHeaderRecord(true)
                .setTrim(true)
                .setIgnoreEmptyLines(true)
                .setIgnoreSurroundingSpaces(true) // This is key!
                .build();

        try (InputStream in = BOMInputStream.builder()
                .setInputStream(new FileInputStream(filePath))
                .setInclude(false)
                .get();
                Reader reader = new InputStreamReader(in, StandardCharsets.UTF_8)) {

            try (CSVParser csvParser = CSVParser.parse(reader, format)) {
                // Let Apache Commons CSV handle the parsing entirely
                Map<String, Integer> headers = csvParser.getHeaderMap();
                int expectedCols = headers.size();
                for (CSVRecord record : csvParser) {
                    if (record.size() != expectedCols) {
                        Logger.getLogger(Utilities.class.getName()).warning(
                                String.format("Line %d: column count %d != header count %d",
                                        record.getRecordNumber() + 1, record.size(), expectedCols));
                    }
                }

                // Reset and parse again for actual processing
                try (InputStream in2 = BOMInputStream.builder()
                        .setInputStream(new FileInputStream(filePath))
                        .setInclude(false)
                        .get();
                        Reader reader2 = new InputStreamReader(in2, StandardCharsets.UTF_8);
                        CSVParser finalParser = CSVParser.parse(reader2, format)) {
                    return parser.parse(finalParser);
                } 
            } catch (IllegalArgumentException ex) {
                    Logger.getLogger(Utilities.class.getName()).log(Level.SEVERE, "Error: %s Verify that there are no trailing header commas. Please check your csv file.", ex.getMessage());
                }
        } catch (IOException ex) {
            Logger.getLogger(Utilities.class.getName()).log(Level.SEVERE, "Error: IOException %s", ex.getMessage());
        }

        return null;
    }

    /**
     * Call to this method opens a file chooser, which might be in a dialog
     * window or in a frame.
     *
     * @param parent graphical component in which to setup the file chooser
     * @return the file chosen by the user (if any)
     */
    public static File chooseCsvFile(Component parent, String title) {
        JFileChooser fileChooser = new JFileChooser();
        fileChooser.setCurrentDirectory(new File(System.getProperty("user.dir"))); // start somewhere close to test data
        fileChooser.setDialogTitle(title);

        FileNameExtensionFilter csvFilter = new FileNameExtensionFilter("CSV Files", "csv");
        fileChooser.setFileFilter(csvFilter);

        int result = fileChooser.showOpenDialog(parent);

        if (result == JFileChooser.APPROVE_OPTION) {
            File selectedFile = fileChooser.getSelectedFile();
            return selectedFile;
        }

        return null;
    }
}

/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
