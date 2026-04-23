/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */

package groupw.Network;

import org.apache.commons.math4.legacy.linear.RealMatrix;
import org.apache.commons.math4.legacy.linear.RealVector;


/**
 * Generic interface of vector-to-vector functions.
 * More focused and efficient than Apache Commons'.
 * In particular, it maps RealVector -> RealVector, not double[][] -> RealVector.
 * thus F.value(G.value(x)) is valid, without any RealVector -> double[][] conversion in the middle
 */
public interface VectorFunction {

    public RealVector value(RealVector p0);
    public RealMatrix jacobian(RealVector p0);

    public int dimInput = 0;
    public int dimOutput = 0;

    /**
     * Return sum of squared errors in AX = Y, assuming all three are compatible matrices (not vector)
     * @param A
     * @param X
     * @param Y
     * @return
     */
    public static double matrixCost (RealMatrix A, RealMatrix X, RealMatrix Y) {
        int n = Y.getRowDimension();
        int m = A.getColumnDimension();
        int k = X.getColumnDimension();
        if ((n != A.getRowDimension())
                || (m != X.getRowDimension())
                || (k != Y.getColumnDimension())) {
            throw new RuntimeException("matrixCost: Mismatched array dimensions");
        }
        double errCost = 0.0;
        RealMatrix err = (A.multiply(X)).subtract(Y);
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < k; j++) {
                double e = err.getEntry(i, j);
                errCost = errCost + (e * e);
            }
        }
        return errCost;
    }

    /**
     * Calculate and return the 'gradient' of matrixCost w.r.t A at (X,Y)
     * which is  2(AX-Y)*X^T
     * Hence, LLSE of A from AX ~= Y has the gradient equal zero, i.e.
     * A X X^T = Y X^T
     * A = (Y X^T) (X X^T)^-1
     * Also derivable from classic LLSE with three matrices (not vectors)
     * in Bw ~= z, w = (B^T B)^-1 (B^T z) where B = X^T, w=A^T and z = Y^T
     * @param A
     * @param X
     * @param Y
     * @return
     */
    public static RealMatrix costGradient(RealMatrix A, RealMatrix X, RealMatrix Y) {
        int n = Y.getRowDimension();
        int m = A.getColumnDimension();
        int k = X.getColumnDimension();
        if ((n != A.getRowDimension())
                || (m != X.getRowDimension())
                || (k != Y.getColumnDimension())) {
            throw new RuntimeException("matrixCost: Mismatched array dimensions");
        }
        RealMatrix err = (A.multiply(X)).subtract(Y);
        RealMatrix xt = X.transpose();
        RealMatrix g = err.multiply(xt).scalarMultiply(2.0);
        return g;
    }

}

// =============================================================================
