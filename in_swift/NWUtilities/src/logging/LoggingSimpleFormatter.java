/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package logging;

import java.util.logging.Formatter;
import java.util.logging.LogRecord;

/**
 * Removes timestamps and other unnecessary information when using java logger.
 * @author DavidHa
 */
public class LoggingSimpleFormatter extends Formatter {

    @Override
    public String format(LogRecord record) {
        return record.getMessage() + System.lineSeparator();
    }

}
// =============================================================================
