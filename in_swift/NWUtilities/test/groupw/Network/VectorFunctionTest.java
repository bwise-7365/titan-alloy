/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.Network;

import junit.framework.TestCase;
import org.apache.commons.math4.legacy.linear.MatrixUtils;
import org.apache.commons.math4.legacy.linear.RealMatrix;

import java.util.Random;

import static groupw.Network.NWUtils.*;
import static groupw.Network.VectorFunction.costGradient;
import static groupw.Network.VectorFunction.matrixCost;

public class VectorFunctionTest extends TestCase {

    public void testMatrixCost() {
        int sd = 0; //DefaultSeedPRNG;
        boolean verbose = true;
        Tuple3<RealMatrix, RealMatrix, RealMatrix>  rslt = makeTest(sd, verbose);
        RealMatrix A = rslt.get0();
        RealMatrix X = rslt.get1();
        RealMatrix Y = rslt.get2();

        double cost = matrixCost(A, X, Y);
        System.out.printf("Matrix cost: %.4f \n", cost);

        int n = Y.getRowDimension();
        int m = A.getColumnDimension();
        int k = X.getColumnDimension();

    }

    public void testCostGradient() {
        int sd = 0; //DefaultSeedPRNG;
        boolean verbose = true;
        Tuple3<RealMatrix, RealMatrix, RealMatrix> rslt = makeTest(sd, verbose);
        RealMatrix A = rslt.get0();
        RealMatrix X = rslt.get1();
        RealMatrix Y = rslt.get2();

        double c0 = matrixCost(A, X, Y);
        System.out.printf("Matrix cost-0: %.4f \n", c0);

        int sd2 = (sd + 1) * (2 * sd + 3);
        Random prng = makePRNG(sd2, verbose);
        int n = Y.getRowDimension();
        int m = A.getColumnDimension();
        int k = X.getColumnDimension();
        RealMatrix A1 = MatrixUtils.createRealMatrix(n, m);
        RealMatrix da = MatrixUtils.createRealMatrix(n, m);
        RealMatrix A2 = MatrixUtils.createRealMatrix(n, m);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                double v0 = A.getEntry(i, j);
                double v1 = v0 + (prng.nextDouble() - 0.3);
                A1.setEntry(i, j, v1);
                double d = (prng.nextDouble() - 0.3) / 1000.0;
                da.setEntry(i, j, d);
                A2.setEntry(i, j, v1 + d);
            }
        }
        double c1 = matrixCost(A1, X, Y);
        System.out.printf("Matrix cost-1: %10.4f \n", c1);
        double c2 = matrixCost(A2, X, Y);
        System.out.printf("Matrix cost-2: %10.4f \n", c2);
        double actualDelta = c2 - c1;
        System.out.printf("Act change: %10.4f \n", actualDelta);

        RealMatrix grad = costGradient(A1, X, Y);
        double estDelta = 0.0;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                double d = grad.getEntry(i, j) * da.getEntry(i,j);
                estDelta = estDelta + d;
            }
        }
        System.out.printf("Est change: %10.4f \n", estDelta);
        double sfd =  symFracDiff(actualDelta, estDelta);
        System.out.printf("Symmetric fractional difference: %.4E \n", sfd);
        assertTrue(sfd < 1E-3);

    }

    private Tuple3<RealMatrix, RealMatrix, RealMatrix> makeTest(int sd, boolean verbose) {
        Random prng = makePRNG(sd, verbose);
        int n = (int) (5.5 + 10 * prng.nextDouble());
        int m = (int) (5.5 + 10 * prng.nextDouble());
        int k = (int) (5.5 + 10 * prng.nextDouble());

        RealMatrix A = MatrixUtils.createRealMatrix(n, m);
        RealMatrix X = MatrixUtils.createRealMatrix(m, k);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < m; j++) {
                double v = 10.0 * prng.nextDouble() - 3.0;
                A.setEntry(i, j, v);
            }
        }
        for (int i=0; i<m; i++) {
            for (int j=0; j<k; j++) {
                double v = 10.0 * prng.nextDouble() - 3.0;
                X.setEntry(i, j, v);
            }
        }
        RealMatrix Y = A.multiply(X);
        Tuple3<RealMatrix, RealMatrix, RealMatrix> rslt = new Tuple3<>(A, X, Y);
        return rslt;
    }
}

// =============================================================================
