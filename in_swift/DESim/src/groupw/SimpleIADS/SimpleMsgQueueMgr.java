/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.SimpleIADS;

import groupw.BaseSim.*;
import groupw.Network.NWUtils.ReportingLevel;
import static groupw.Network.NWUtils.ReportingLevel.Silent;

/**
 * The message queue manager sends and receives message.
 * If they are addressed to this manager, it handles them internally.
 * Otherwise, it queues them for later retransmission.
 * @author BenWise
 */
public class SimpleMsgQueueMgr
        extends Entity
        implements SimpleMsgProc {

    public SimpleMsgQueueMgr(int sm, double mpt, Scheduler s ) {
        super(s);
         myQueue = new SimpleMsgQueue(sm, mpt);
    }

    protected void scheduleNext() {
        double dt = mySim.negExp(myQueue.meanProcTime);
        double tNow = mySim.getCurrTime();
        double t2 = tNow + dt;
        System.out.printf("%s SimpleMsgQueueMgr.scheduleNext %4d after %.2f : %.2f \n", mySim.timeStamp(), getID(), dt, t2);
        EntEvent e = new EntEvent(this, mySim, t2);
        mySim.addEvent(e);
    }

    @Override
    public void receive(SimpleMessage sm) {
        assert (null != mySim);
        boolean wasEmpty = myQueue.isEmpty();
        if (sm.dstID == getID()) {
            System.out.printf("SimpleMsgQueueMgr %6d received message %6d for self.\n", getID(), sm.getID());

            // TODO: This should occur as a separate event, after message processing time.
            handleMsgToSelf(sm);
        }
        else  {
            myQueue.receive(sm);
        }

        // If it was empty, then we schedule the next send event.
        // But if it was not empty, then another send event was already scheduled,
        // so we need only push it onto the queue
        System.out.printf("%s SimpleMsgQueueMgr %6d has %5d messages in queue \n", mySim.timeStamp(), getID(), myQueue.size());
        if (wasEmpty) {
            scheduleNext();
        }
    }


    @Override
    public void send(SimpleMessage sm) {
        long d = sm.dstID;
        if (getID() == sm.dstID) { // prevent endless loop of unphysical behavior
            System.out.printf("SimpleMsgQueueMgr %6d cannot send message to self - message %6d dropped and not sent.\n",
                    getID(), sm.getID());

        } else {
            myQueue.send(sm);
        }
    }

    @Override
    public void handleMsgToSelf(SimpleMessage sm) {
        if (sm.dstID != getID()){
            throw new RuntimeException("SimpleMsgQueueMgr "+getID()+" received message to self addressed to "+sm.dstID);
        }
        System.out.printf("SimpleMsgQueueMgr %6d will handle message %6d from %6d to self.\n",
                getID(), sm.getID(), sm.srcID);
    }

    @Override
    public void process() {
        if (!myQueue.isEmpty()) {
            int n1 = myQueue.size();
            SimpleMessage sm = myQueue.removeFirst();
            int n2 = myQueue.size();
            //System.out.printf("SimpleMsgQueueMgr %d had %d before and %d after removeFirst \n", getID(), n1, n2);
            assert (n2 + 1 == n1);
            send(sm);
            System.out.printf("%s SimpleMsgQueueMgr.process %4d has %5d messages in queue \n", mySim.timeStamp(), getID(), myQueue.size());
            if (!myQueue.isEmpty()) {
                scheduleNext();
            }
        }
    }

    // TODO: a FIFO queue with stochastic waiting times
    // TODO: input a message, output a message
    SimpleMsgQueue myQueue;
    public ReportingLevel rLevel = Silent;

}


// =============================================================================
