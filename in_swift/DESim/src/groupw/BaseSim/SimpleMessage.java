/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.BaseSim;

import java.util.HashMap;
import java.util.Map;

/**
 * The basic contents of a message is a set of (key, value) strings.
 * The expected contents, and their interpretation, depends on the
 * 'format' string.
 * @author BenWise
 */
public class SimpleMessage
    extends CountedItem
{

    public SimpleMessage(long srcID, long dstID, String format, Map<String, String> contents) {
        super(); // set ID and register
        this.srcID = srcID;
        this.dstID = dstID;
        this.format = format;
        this.contents = contents;
    }

    /**
     * The radar detection message format exists mainly to support testing
     * in the SimpleIADS test.
     * @param srcID
     * @param dstID
     * @return
     */
    static public SimpleMessage makeRadarDetection(long srcID, long dstID) {
        Map<String, String> c = new HashMap<>(); // TODO: add something more useful
        SimpleMessage sm = new SimpleMessage(srcID, dstID, RadarDetectionFormat, c);
        return sm;
    }

    static public SimpleMessage makeRelayMessage(long origID, long srcID, long dstID, long finalID) {
        int timeToGo = 5; // 5 hops max
        Map<String, String> c = new HashMap<String, String>();
        c.put("OriginatorID", Long.toString(origID));
        c.put("FinalID", Long.toString(finalID));
        c.put("TimeToGo", Integer.toString(timeToGo));
        SimpleMessage sm = new SimpleMessage(srcID, dstID, RelayFormat, c);

        return sm;
    }

    final public long srcID; // immediate sender (not necessarily the originator)
    final public long dstID; // immediate destination (not necessarily the final)
    final public String format;
    final public Map<String, String> contents;

    static public String RadarDetectionFormat = "RadarDetection";

    /**
     * The Relay format is anticipated to be generally applicable for message-passing networks
     */
    static public String RelayFormat = "Relay";


}
// =============================================================================
