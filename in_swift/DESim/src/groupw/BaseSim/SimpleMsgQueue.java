/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.BaseSim;

import java.util.ArrayDeque;
import java.util.Deque;

/**
 * This is a fairly bare queue. It simply sends and receives,
 * so it should be embedded in a queue manager that schedules
 * events in a simulation or adjudications in a game.
 */
public class SimpleMsgQueue
    extends CountedItem
        implements SimpleMsgProc {

    public SimpleMsgQueue(int sm, double mpt) {
        super();
        if (0 < sm) {
            sizeMax = sm;
        } else {
            sizeMax = 0; // default is to be unlimited
        }
        if (0.0 < mpt) {
            meanProcTime = mpt;
        } else {
            meanProcTime = 1.0;
        }
        queue = new ArrayDeque<SimpleMessage>();
    }


    @Override
    public void send(SimpleMessage sm) {
        long d = sm.dstID;
        if (getID() == d) { // prevent endless loop of unphysical behavior
            System.out.printf("SimpleMsgProc %4d cannot send message to self - message %4d dropped and not sent.\n", d, sm.getID());

        } else {
            CountedItem ci = ItemRegistry.getItem(d);
            if (null != ci) {
                SimpleMsgProc smp = (SimpleMsgProc) ci;
                System.out.printf("SimpleMsgProc %4d will send message %4d to SimpleMsgProc %4d.\n",
                        d, sm.getID(), smp.getID());
                smp.receive(sm);
            } else {
                System.out.printf("SimpleMsgProc %4d receiver was not found - message %4d dropped and not sent to it.\n", d, sm.getID());
            }
        }
    }

    @Override
    public void handleMsgToSelf(SimpleMessage sm) {
        if (sm.dstID != getID()){
            throw new RuntimeException("SimpleMsgQueue "+getID()+" received message to self addressed to "+sm.dstID);
        }
        System.out.printf("SimpleMsgQueue %6d will handle message %6d from %6d to self.\n",
                getID(), sm.getID(), sm.srcID);
    }


    @Override
    public void receive(SimpleMessage sm) {
        if ((0 == sizeMax) || (queue.size() < sizeMax)) {
            queue.addLast(sm);
            System.out.printf("SimpleMsgQueue %4d received message %4d reached queue size %4d/%d , \n",
                    getID(), sm.getID(), queue.size(), sizeMax);
        } else {
            System.out.printf("SimpleMsgQueue %4d  dropped message %4d reached queue  max %4d/%d , \n",
                    getID(), sm.getID(), queue.size(), sizeMax);
            ;
        }
    }

    public boolean isEmpty() {
        return queue.isEmpty();
    }

    public int size() {
        return queue.size();
    }

    public SimpleMessage removeFirst() {
        return queue.removeFirst();
    }

    protected Deque<SimpleMessage> queue;
    public int sizeMax = 0; // non-positive means unlimited
    public double meanProcTime = 1.0; // seconds

}
// =============================================================================
