/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */
package groupw.SimpleIADS;

import java.util.HashMap;
import java.util.Map;
import org.junit.Test;
import static org.junit.Assert.*;

/**
 *
 * @author BenWise
 */
public class rpslsTest {

    public rpslsTest() {
    }

    @Test
    public void testSymmetricDefeats() {
        for (rpsls.Move mBlue : rpsls.Move.values()) {
            for (rpsls.Move mRed : rpsls.Move.values()) {
                int dBR = rpsls.defeats(mBlue, mRed);
                int dRB = rpsls.defeats(mRed, mBlue);
                //System.out.printf("%7s:%7s  gives %2d %2d \n", mBlue, mRed, dBR, dRB);
                assertEquals(0, dBR + dRB);
            }
        }
    }

    /**
     * Run Brown's Algorithm (see "Approximations" chapter of The Compleat Strategyst)
     */
    @Test
    public void testStrategiesDefeats() {
        Map<rpsls.Move, Integer> bCounts = new HashMap<>(5);
        Map<rpsls.Move, Integer> rCounts = new HashMap<>(5);
        for (rpsls.Move m : rpsls.Move.values()) {
            bCounts.put(m, 0);
            rCounts.put(m, 0);
        }
        double bestVal = Double.NEGATIVE_INFINITY;
        int numRounds = 20 *  1000; // need 20 million to consistently get 4 correct decimals
        rpsls.Move initialMove = rpsls.Move.Spock; // their usual choice
        int crlfFreq = numRounds / 80;
        for (int iter = 1; iter <= numRounds; iter++) {
            if (0 == (iter % crlfFreq)) {
                System.out.print(".");
                System.out.flush();
            }
            // pick a Blue move
            rpsls.Move bestMoveBlue = initialMove;
            bestVal = Double.NEGATIVE_INFINITY;
            for (rpsls.Move mb : rpsls.Move.values()) {
                // calculate expected value to Blue of this move given Red's past behavior
                double num = 0.0;
                double dnm = 0.0;
                for (rpsls.Move mr : rpsls.Move.values()) {
                    int d = rpsls.defeats(mb, mr);
                    double w = 1.0 + rCounts.get(mr); // Dirichlet Bayesian estimate
                    num = num + (w * d);
                    dnm = dnm + w;
                }
                double eVal = num / dnm;
                if (eVal > bestVal) {
                    bestMoveBlue = mb;
                    bestVal = eVal;
                }
            }
            bCounts.put(bestMoveBlue, 1 + bCounts.get(bestMoveBlue));
            
            

            // pick a Red move
            rpsls.Move bestMoveRed = initialMove;
            bestVal = Double.NEGATIVE_INFINITY;
            for (rpsls.Move mr : rpsls.Move.values()) {
                // calculate expected value to Red of this move given Blue's past behavior
                double num = 0.0;
                double dnm = 0.0;
                for (rpsls.Move mb : rpsls.Move.values()) {
                    int d = rpsls.defeats(mr, mb);
                    double w = 1.0 + bCounts.get(mb); // Dirichlet Bayesian estimate
                    num = num + (w * d);
                    dnm = dnm + w;
                }
                double eVal = num / dnm;
                if (eVal > bestVal) {
                    bestMoveRed = mr;
                    bestVal = eVal;
                }
            }
            rCounts.put(bestMoveRed, 1 + rCounts.get(bestMoveRed));
            
            
            
            int bSum = 0;
            int rSum = 0;
            for (rpsls.Move m : rpsls.Move.values()){
                bSum = bSum + bCounts.get(m);
                rSum = rSum + rCounts.get(m);
            }
            assertEquals(iter, bSum);
            assertEquals(iter, rSum);
            
        } // end of loop over iter
        System.out.println();
        
        
            for (rpsls.Move m : rpsls.Move.values()){
                System.out.printf("Blue %7s: %.4f\n", m, bCounts.get(m) / ((double) numRounds));
            }
            System.out.println("");
            
            for (rpsls.Move m : rpsls.Move.values()){
                System.out.printf("Red  %7s: %.4f\n", m, rCounts.get(m) / ((double) numRounds));
            }
            System.out.println("");
    }
}
// =============================================================================
