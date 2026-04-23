/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package logging;

import java.util.logging.Handler;
import java.util.logging.Logger;

/**
 * Call at application startup to use the Logging Simple Formatter.
 * @author DavidHa
 */
public class LoggerSetup {

    public static void configureLogger() {
        Logger rootLogger = Logger.getLogger("");
        Handler[] handlers = rootLogger.getHandlers();

        for (Handler handler : handlers) {
            handler.setFormatter(new LoggingSimpleFormatter());
        }
    }
}
// =============================================================================
