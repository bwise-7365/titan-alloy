/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.Logistics.Parsers;

/**
 *
 * @author DavidHa
 */
public class DuplicateItemException extends Exception {
    public DuplicateItemException(String message){
        super(message);
    }

    // Just in case some SWIFT code does serialize this
    private static final long serialVersionUID = 1000;
}

// =============================================================================
