package groupw.Network;

import java.util.List;
import groupw.Network.NWUtils.PointCoords;

/**
 *
 * This class uses the furthest-insertion heuristic.
 * Start with a trivial one-node tour.
 * For any tour, find the node furthest from the tour and splice
 * it into whichever segment gives the lowest-cost.
 *
 * @author BenWise
 *
 */
public class ApproxTSPFI extends ApproxTSP {
    public ApproxTSPFI() {
        super();
    }

    public ApproxTSPFI(ApproxTSP a) {
        super(a);
    }

    static public void extendETour(ApproxTSPFI et0, int nPointsTotal, PointCoords pcs) {
        assert(et0.eTourP());
        assert (et0.points.size() == et0.numPoints);
        assert (et0.numPoints < nPointsTotal); // length of those in the turn must be less than the total available

        int k = furthestUnused(et0.points, nPointsTotal, pcs);
        double minExtensionCost = Double.POSITIVE_INFINITY;
        int insertionNdx = -1;
        for (int ndx = 1; ndx < et0.numPoints; ndx++) {
            double c = costOfExtension(
                    et0.points.get(ndx-1),  // point 'i'
                    et0.points.get(ndx),    // adjacent point 'j'
                    k,
                    pcs);
            if (c < minExtensionCost) {
                minExtensionCost = c;
                insertionNdx = ndx;
            }
        }
        et0.points.add(insertionNdx, k);
        et0.numPoints = et0.points.size();
    }

    /**
     * Calculate the added cost of changing a [i,j] edge to [i,k] and [k,j] edges
     * @param i
     * @param j
     * @param k
     * @param dMat
     * @return
     */
    static protected double costOfExtension (int i, int j, int k, PointCoords pcs) {
        double dij = pcs.cost(i, j);
        double dik = pcs.cost(i, k);
        double dkj = pcs.cost(k, j);
        return (dik + dkj - dij);
    }

    static protected int furthestUnused(List<Integer> tour, int nPoints, PointCoords pcs) {
        int furthestK = -1;
        double furthestDistance = Double.NEGATIVE_INFINITY;
        for (int k = 0; k < nPoints; k++) {
            if (!tour.contains(k)) {
                double dk = distFromTour(k, tour, nPoints, pcs);
                if (furthestDistance < dk) {
                    furthestDistance = dk;
                    furthestK = k;
                }
            }
        }
        return furthestK;
    }

    /**
     * The distance from a point to a tour is the shortest distance from it to any point in the tour
     * @param tour
     * @param nPoints
     * @param pcs
     * @return
     */
    static protected double distFromTour(int k, List<Integer> tour, int nPoints, PointCoords pcs) {
        double dMin = Double.POSITIVE_INFINITY;
        for (int i : tour){
            double dki = pcs.cost(k, i);
            dMin = Math.min(dki, dMin);
        }
        return dMin;
    }

}

// =============================================================================
