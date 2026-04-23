/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.BaseSim;


/**
 * Interface to send and receive messages
 * @author bwise
 */
public interface SimpleMsgProc
{
    public void receive(SimpleMessage sm); // then do whatever makes sense
    public void send(SimpleMessage sm);

    /**
     * This needs to be overridden by an implementation-specific action.
     * @param sm
     */
    public void handleMsgToSelf(SimpleMessage sm);
    public long getID(); // from the CountedItem
}


// =============================================================================
