/*
 * Click nbfs://nbhost/SystemFileSystem/Templates/Licenses/license-default.txt to change this license
 * Click nbfs://nbhost/SystemFileSystem/Templates/UnitTests/JUnit4TestClass.java to edit this template
 */
package groupw.SimpleIADS;

import static groupw.Network.NWUtils.DefaultSeedPRNG;
import groupw.BaseSim.Entity;
import groupw.BaseSim.Scheduler;
import groupw.BaseSim.SimpleMessage;
import org.junit.Test;

import java.util.ArrayList;
import java.util.List;

import static org.junit.Assert.*;

/**
 *
 * @author bwise
 */
public class SimpleMsgQueueTest {

    public SimpleMsgQueueTest() {
    }

    /**
     * Put messages in a queue and pull them out, and
     * send them to one or the other receiver. This is the archetype
     * for the "one big queue" model of a network.
     */
    @Test
    public void smqTest01() {
        System.out.println("\n\nStarting smqTest00");

        int sd0 = DefaultSeedPRNG;
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.setPRNG(sd0);
        int numReceivers = 15;
        int msgsPerReceiver = 10;
        int queueMax = (msgsPerReceiver + 1) * numReceivers;
        SimpleMsgQueueMgr msgCloud = new SimpleMsgQueueMgr(queueMax, 5.0, sim);
        long idA = msgCloud.getID();
        System.out.printf("Created msgCloud as SimpleMsgQueueMgr %d \n", idA);

        List<SimpleMsgQueueMgr> receivers = new ArrayList<>(0);
        for (int i = 0; i<numReceivers; i++) {
            SimpleMsgQueueMgr smq = new SimpleMsgQueueMgr(queueMax, 5.0, sim);
            System.out.printf("Created SimpleMsgQueueMgr %d \n", smq.getID());
            receivers.add(smq);
        }

        // We check that it drops 5 because it is full
        for (int i = 0; i < 5 + queueMax; i++) {

            System.out.printf("There are %d events in queue\n", sim.queueSize());
            int id0 = 2000 + i;
            SimpleMessage sm0;
            long id = receivers.get(i % numReceivers).getID();
            sm0 = SimpleMessage.makeRadarDetection(id0, id); // from fake source to actual receiver
            System.out.printf("Created radar detection %d from %d to %d\n", sm0.getID(), sm0.srcID, sm0.dstID);
            msgCloud.receive(sm0);
            System.out.flush();
        }

        System.out.printf("===================\n");
        System.out.printf("Starting simple sim\n");
        while (0 < sim.queueSize()) {
            System.out.printf("%s There are %d events in queue\n", sim.timeStamp(), sim.queueSize());
            sim.step();
            System.out.printf("%s There are %d events in queue\n\n", sim.timeStamp(), sim.queueSize());
            System.out.flush();
        }
        return;
    }

    /**
     * Put messages in a Q1 then send them to Q2. Q2 receives them. No other
     * processing
     */
    @Test
    public void smqTest00() {
        System.out.println("\n\nStarting smqTest01");

        int sd0 = DefaultSeedPRNG;
        //sd0 = 0;
        Scheduler sim = new Scheduler();
        sim.setPRNG(sd0);
        int queueMax = 0; // non-positive means unlimited

        // We want the second queue to be only a little slower.
        // It will sometimes have several message queued, but sometimes it will empty
        SimpleMsgQueueMgr smq1 = new SimpleMsgQueueMgr(queueMax, 7.5, sim);
        SimpleMsgQueueMgr smq2 = new SimpleMsgQueueMgr(queueMax, 5.0, sim);

        long id1 = smq1.getID();
        long id2 = smq2.getID();

        // no output means they were found
        Entity obj1 = sim.getEnt(id1);
        assertNotNull(obj1);
        Entity obj2 = sim.getEnt(id2);
        assertNotNull(obj2);

        int numSM = 25;
        for (int i = 0; i < numSM; i++) {
            SimpleMessage sm0 = SimpleMessage.makeRadarDetection(id1, id2);
            smq1.receive(sm0);
        }

        boolean stepP = true;
        while (stepP) {
            System.out.printf("There are %d events in queue\n", sim.queueSize());
            sim.step();
            System.out.printf("There are %d events in queue\n\n", sim.queueSize());
            System.out.flush();

            stepP = ((0 < sim.queueSize()) && (sim.queueSize() < 10));
        }

        sim = null;
    }

}

// =============================================================================
