/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics.Parsers;

import static groupw.Logistics.Parsers.Utilities.chooseCsvFile;
import static groupw.Logistics.Parsers.Utilities.readCsv;
import java.io.File;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.JDialog;
import javax.swing.JFrame;
import org.apache.commons.csv.CSVParser;

/**
 *
 * @author DavidHa
 */
public class CsvProcessor {

    protected static final Map<CsvFileSpec, String> pathMap = new HashMap<>();
    protected static final Map<CsvFileSpec, Object> csvDataCache = new HashMap<>();

    /**
     * Returns a pre-existing instance of CsvProcessor if any exists.
     * Otherwise, there is an option to either return NULL
     * or to create a new one, save a reference, and return it.
     * This allows developers to use 'getInstance' and thus
     * treat it like the singleton it really is.
     * The developer can force a new one to be created using 'CsvProcessor()',
     * but it is also possible for 'getInstance' to retrieve a pre-existing instance, if any, without
     * being required to rebuild it from scratch.
     * @param forceInit should a new instance be created if none currently exists?
     * @return the singleton instance, possibly NULL if not initialized
     */
    public static CsvProcessor getInstance(boolean forceInit) {
        if (forceInit && (null == CsvProcessor.theInstance)) {
            theInstance = new CsvProcessor();
        }
        return theInstance;
    }

    private static CsvProcessor theInstance = null;


    public interface CsvParserWrapper<T> {

        T parse(CSVParser csvParser) throws DuplicateItemException;

        List<String> verifyData();
    }

    /**
     * Interface for classes to implement their own CSV parsing.
     *
     * @param <T>
     */
    public interface RowParser<T> extends CsvParserWrapper<T> {
    }

    /**
     * Interface for classes to implement their own CSV parsing.
     *
     * @author DavidHa
     * @param <T>
     */
    public interface RowListParser<T> extends CsvParserWrapper<List<T>> {
    }

    /**
     * Interface for classes to implement their own CSV parsing.
     *
     * @author DavidHa
     * @param <K>
     * @param <V>
     */
    public interface RowMapParser<K, V> extends CsvParserWrapper<Map<K, V>> {
    }

    //Ideally this would be somewhere in the app directory...
    public static final String LOG_CONFIRUTATION_FILE_END = File.separator + "groupw" + File.separator + "Logistics" + File.separator + "Data2" + File.separator + "configuration.csv"; // TODO: fix hard-coded path
    public static final String LOG_CONFIGURATION_FILE_PATH = System.getProperty("user.dir") + File.separator + "src" + LOG_CONFIRUTATION_FILE_END;
    public static final String LOG_CONFIGURATION_TEST_FILE_PATH = System.getProperty("user.dir") + File.separator + "test" + LOG_CONFIRUTATION_FILE_END;
// TODO: test file path needs 'groupw' ?

    /**
     * Create a new, uninitialized processor and store a reference in the singleton.
     */
    public CsvProcessor() {
        pathMap.clear();
        csvDataCache.clear();
        theInstance = this;
    }

    /**
     * Initializes the log configuration files which contains the paths to the
     * log network csv files.
     *
     * @throws DuplicateItemException
     */
    public void init() throws DuplicateItemException {
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, LOG_CONFIGURATION_FILE_PATH);
        initPaths();
    }

    public void initTest() throws DuplicateItemException {
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, LOG_CONFIGURATION_TEST_FILE_PATH);
        initTestPaths();
    }

    public void initFromPath(String file) throws DuplicateItemException {
        getPathMap().clear();
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, file);
        initPaths();
    }
    
    /**
     * Initializes the log configuration file by allowing the user to select the
     * configuration file that contains the paths to the log network csv files.
     *
     * @throws DuplicateItemException
     */
    public void initWithFileChooser() throws DuplicateItemException {
        getPathMap().clear();
        // NOTE: this 'title' parameter is not used or visible
        JDialog dialog = new JDialog(new JFrame(), "Choose Logistics Path Configuration File", true);

        // attempt to prevent the generic dialog from hiding behind main window
        dialog.toFront(); // this alone is not enough
        dialog.requestFocus();

        // NOTE: this 'title' parameter is used and visible
        File file = chooseCsvFile(dialog, "Select Data or Data2 \\ configuration.csv");
        getPathMap().put(CsvFileSpec.LOGCONFIGURATION, file.getAbsolutePath());
        initPaths();
    }

    /**
     * Initializes the paths to the log network csv files.
     *
     * @throws DuplicateItemException
     */
    public void initPaths() throws DuplicateItemException {
        Map<CsvFileSpec, String> logCsvFilesMap = (Map<CsvFileSpec, String>) getData(CsvFileSpec.LOGCONFIGURATION);
        getPathMap().putAll(logCsvFilesMap);
        for (Map.Entry<CsvFileSpec, String> filePathEntry : logCsvFilesMap.entrySet()) {
            if (filePathEntry.getKey().equals(CsvFileSpec.LOGCONFIGURATION)) {
                continue;
            }
            Logger.getLogger(CsvProcessor.class.getName()).log(Level.INFO, String.format("Found key %s and value (filename): %s",
                    filePathEntry.getKey().toString(), filePathEntry.getValue()));
        }
    }

    /**
     * Need to fix this to not duplicate code.
     *
     * @throws DuplicateItemException
     */
    public void initTestPaths() throws DuplicateItemException {
        Map<CsvFileSpec, String> logCsvFilesMap = (Map<CsvFileSpec, String>) getData(CsvFileSpec.LOGCONFIGURATION);
        for (Map.Entry<CsvFileSpec, String> filePathEntry : logCsvFilesMap.entrySet()) {
            if (filePathEntry.getKey().equals(CsvFileSpec.LOGCONFIGURATION)) {
                continue;
            }
            String userDir = System.getProperty("user.dir") + File.separator + "test" + File.separator+ "groupw" + File.separator; // TODO: fix hard-coded path
            Path testPath = Paths.get(userDir, filePathEntry.getValue());
            logCsvFilesMap.put(filePathEntry.getKey(), testPath.toString());
            Logger.getLogger(CsvProcessor.class.getName()).log(Level.INFO, String.format("Found key %s and value (filename): %s",
                    filePathEntry.getKey().toString(), filePathEntry.getValue()));
        }
        getPathMap().putAll(logCsvFilesMap);
    }

    public static String getPath(CsvFileSpec csvFileSpec) {
        if (getPathMap().get(csvFileSpec) != null) {
            return getPathMap().get(csvFileSpec);
        } else {
            return null;
        }
    }



    public static Object getData(CsvFileSpec spec) throws DuplicateItemException {
        if (!csvDataCache.containsKey(spec)) {
            CsvParserWrapper parser = spec.getParser();
            Object parsed;

            if (parser instanceof RowMapParser<?, ?>) {
                parsed = readCsv(getPath(spec), (RowMapParser<?, ?>) parser);
            } else if (parser instanceof RowListParser<?>) {
                parsed = readCsv(getPath(spec), (RowListParser<?>) parser);
            } else if (parser instanceof RowParser<?>) {
                parsed = readCsv(getPath(spec), (RowParser<?>) parser);
            } else {
                throw new IllegalStateException("Unknown parser type: " + parser.getClass().getName());
            }

            csvDataCache.put(spec, parsed);
        }

        Object thing = csvDataCache.get(spec);
        return thing;
    }

    public static <T> List<T> getList(CsvFileSpec spec) {
        if (!csvDataCache.containsKey(spec)) {
            CsvParserWrapper parser = spec.getParser();
            if (parser instanceof RowListParser<?>) {
                return (List<T>) csvDataCache.get(spec);
            } else {
                throw new IllegalStateException("");
            }
        }
        return null;
    }

    public static <K, V> Map<K, V> getMap(CsvFileSpec spec) {
        if (!csvDataCache.containsKey(spec)) {
            CsvParserWrapper parser = spec.getParser();
            if (parser instanceof RowMapParser<?, ?>) {
                return (Map<K, V>) csvDataCache.get(spec);
            }
        } else {
            throw new IllegalStateException("");
        }
        return null;
    }

    /**
     * @return the pathMap
     */
    public static Map<CsvFileSpec, String> getPathMap() {
        return pathMap;
    }

    /**
     * @return the csvDataCache
     */
    public static Map<CsvFileSpec, Object> getCsvDataCache() {
        return csvDataCache;
    }

    /**
     * Validates csv headers using the parser header map with pre-defined
     * required headers defined in csvFileSpec for respective parser class.
     *
     * @param csvParserName
     * @param csvHeaderMap
     * @param requiredHeaders
     * @return false if a missing header is found.
     */
    public static boolean validateHeaders(String csvParserName, Map<String, Integer> csvHeaderMap, List<String> requiredHeaders) {
        boolean missingHeader = false;
        Logger.getLogger(CsvProcessor.class.getName()).log(
                Level.INFO,
                String.format("\n"+"Validating headers for %s.csv", csvParserName));
        for (String header : requiredHeaders) {
            if (!csvHeaderMap.containsKey(header)) {
                Logger.getLogger(CsvProcessor.class.getName()).log(
                        Level.SEVERE,
                        String.format("Missing header: %s", header));
                missingHeader = true;
            }
        }
        return missingHeader;
    }
}
/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
