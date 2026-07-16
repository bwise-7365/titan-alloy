// =============================================
// Copyright Ben Paul Wise. All Rights Reserved.
// =============================================

// Subscript types (CapNode, DmndNode, AType, TType) and the containers
// they index (DoubleVec, DoubleGrid, IntGrid, Vec, Grid) live in Indexing.java.

import static java.lang.Math.sqrt;

//TIP To <b>Run</b> code, press <shortcut actionId="Run"/> or
// click the <icon src="AllIcons.Actions.Execute"/> icon in the gutter.
public class Main {

    public static void main(String[] args) {
        //TIP Press <shortcut actionId="ShowIntentionActions"/> with your caret at the highlighted text
        // to see how IntelliJ IDEA suggests fixing it.
        System.out.printf("Hello and welcome!\n");


        var prng = new java.util.Random(); // different every time
        int numTransTypes = 3; // number of transport types
        int numAssetTypes = 4; // number of asset types

        // By raising either plannedDist or plannedDays or both,
        // you can raise the final number of transports.
        // if plannedDist/plannedDays rises, so will the number of transports.
        double plannedDist = 2500; // miles
        double plannedDays = 30;

        int numCapNodes = 3;
        int numDmndNodes = 8;
        double plannedHours = plannedDays * 24; // hours

        // setup random asset types
        Vec<Asset, AType> assetTypes = Vec.of(numAssetTypes, AType::new);
        for (AType a : assetTypes.keys()) {
            var at = new Asset();
            double qa = prng.nextDouble();

            // random cargo area
            at.aRqrt = 10 + qa * qa * 250; // area


            // random cargo density
            double qw = prng.nextDouble(10.0, 50.0);
            at.wRqrt = qw * at.aRqrt;
            System.out.printf("Asset type %2d: area %6.1f  wght %7.1f\n", a.i(), at.aRqrt, at.wRqrt);
            assetTypes.set(a, at);
        }
        System.out.println();


        // build transport types as random combinations of asset types
        Vec<Transport, TType> transTypes = Vec.of(numTransTypes, TType::new);
        for (TType t : transTypes.keys()) {
            var tt = new Transport();

            AType ndx1 = new AType(prng.nextInt(assetTypes.size()));
            AType ndx2 = new AType(prng.nextInt(assetTypes.size()));

            double s1 = prng.nextDouble(2.0, 10.0);
            double s2 = prng.nextDouble(2.0, 10.0);

            tt.aCap = (s1 * assetTypes.get(ndx1).aRqrt + s2 * assetTypes.get(ndx2).aRqrt) / 2.0;
            tt.wCap = (s1 * assetTypes.get(ndx1).wRqrt + s2 * assetTypes.get(ndx2).wRqrt) / 2.0;

            double ps = prng.nextDouble(0.0, 1.0);
            tt.speed = 20 + ps * ps * 100;

            System.out.printf("Transport type %2d: area %6.1f  wght %7.1f  speed %5.1f\n",
                              t.i(), tt.aCap, tt.wCap, tt.speed);

            transTypes.set(t, tt);
        }
        System.out.println();


        // determine the number of each kind of asset per Demand node
        IntGrid<DmndNode, AType> demand =
                IntGrid.of(numDmndNodes, DmndNode::new, numAssetTypes, AType::new);
        for (DmndNode k : demand.rows()) {
            for (AType a : demand.cols()) {
                demand.set(k, a, prng.nextInt(500, 2000));
                System.out.printf("Node %2d, asset type %2d: %4d\n", k.i(), a.i(), demand.get(k, a));
            }
        }
        System.out.println();

        DoubleVec<AType> D = DoubleVec.of(numAssetTypes, AType::new); // D.get(a) is the total demand for asset type

        // Two measures of demand
        double WD = 0.0; // total weight, w/o weighting by mileage
        double AD = 0.0; // total area, w/o weighting by mileage

        for (AType a : D.keys()) {
            D.set(a, 0.0);
            for (DmndNode k : demand.rows()) {
                D.add(a, demand.get(k, a));
            }
            WD = WD + D.get(a) * assetTypes.get(a).wRqrt;
            AD = AD + D.get(a) * assetTypes.get(a).aRqrt;
        }

        System.out.printf("Total w and a required: %.4e  %.4e\n", WD, AD);
        System.out.println();

        System.out.println("Done building demand\n");

        // These are the total weight and area requirements
        // for the transport fleet, based on nominal travel distance
        double wd = plannedDist * WD; // total weight*mile demand
        double ad = plannedDist * AD; // total area*mile demand


        double wc = 0.0; // total weight*mile capacity
        double ac = 0.0; // total area*mile capacity

        // set initial number of transports per node
        // and update total weight and area capacities
        DoubleGrid<CapNode, TType> transport =
                DoubleGrid.of(numCapNodes, CapNode::new, numTransTypes, TType::new);
        for (CapNode k : transport.rows()) {
            for (TType t : transport.cols()) {
                double n = prng.nextInt(10, 20);
                transport.set(k, t, n);
                System.out.printf("Node %2d, trans type %2d: %4.1f\n", k.i(), t.i(), transport.get(k, t));

                Transport tt = transTypes.get(t);
                wc = wc + (n * tt.wCap * plannedHours * tt.speed);
                ac = ac + (n * tt.aCap * plannedHours * tt.speed);

            }
        }
        System.out.println();

        // in general, capacity and demand will not match.
        System.out.printf("Initial demand and capacity\n");
        System.out.printf("Total w*m and a*m required: %.4e  %.4e\n", wd, ad);
        System.out.printf("Total w*m and a*m capacity: %.4e  %.4e\n", wc, ac);
        System.out.println();

        // we seek an 'f' that minimizes the RMS percent error
        // ((f*wc-wd)/wd)^2 + ((f*ac-ad)/ad)^2
        // Differentiating and setting to zero
        // f((wc/wd)^2) + f((ac/ad)^2) = (wd*wc)/(wd^2) + (ad*ac)/(ad^2)
        double lhs = (wc * wc) / (wd * wd) + (ac * ac) / (ad * ad);
        double rhs = (wd * wc) / (wd * wd) + (ad * ac) / (ad * ad);

        double f = rhs / lhs;
        System.out.printf("Adjusting f to minimize RMS percent error: %.4e\n", f);
        wc = wc * f;
        ac = ac * f;
        System.out.printf("Adjusted capacity\n");
        System.out.printf("Total w*m and a*m required: %.4e  %.4e\n", wd, ad);
        System.out.printf("Total w*m and a*m capacity: %.4e  %.4e\n", wc, ac);
        System.out.println();

        // reset
        wc = 0.0;
        ac = 0.0;
        for (CapNode k : transport.rows()) {
            for (TType t : transport.cols()) {
                int n = (int) (0.5 + (f * transport.get(k, t)));
                transport.set(k, t, n);
                System.out.printf("Node %2d, trans type %2d: %3d\n", k.i(), t.i(), n);

                wc = wc + (n * transTypes.get(t).wCap * plannedHours * transTypes.get(t).speed);
                ac = ac + (n * transTypes.get(t).aCap * plannedHours * transTypes.get(t).speed);
            }
        }
        System.out.println();


        System.out.printf("Final capacity\n");
        System.out.printf("Total w*m and a*m required: %.4e  %.4e\n", wd, ad);
        System.out.printf("Total w*m and a*m capacity: %.4e  %.4e\n", wc, ac);
        System.out.println();

        System.out.println("Done building transports\n");


        // Now we have to set up the supplies.
        //
        // At each supply node, they should be roughly proportional
        // to the number of transport vehicles at that node.
        // Also, the total amount of supply of each asset type
        // should be a fixed multiple of the total demand for that asset type.
        //
        // We use something like the Gravity Model:
        // C[k][a] = x[k] * s * D[a]
        //
        // where 'sdRatio' is the overall ratio of supply to demand, and
        // 'x[k]' is node k's share of that supply: proportional to the
        // transport capacity parked at node k, and summing to 1 over nodes.

        double sdRatio = (1+sqrt(5.0))/2.0; // total supply is 'sdRatio' times the total demand

        // two measures of transport capacity, per node
        DoubleVec<CapNode> WT = DoubleVec.of(numCapNodes, CapNode::new);
        DoubleVec<CapNode> AT = DoubleVec.of(numCapNodes, CapNode::new);

        double sumWT = 0.0; // weight capacity, summed over all nodes
        double sumAT = 0.0; // area capacity, summed over all nodes
        for (CapNode k : WT.keys()) {
            WT.set(k, 0.0);
            AT.set(k, 0.0);
            for (TType t : transport.cols()) {
                WT.add(k, transport.get(k, t) * transTypes.get(t).wCap);
                AT.add(k, transport.get(k, t) * transTypes.get(t).aCap);
            }
            sumWT = sumWT + WT.get(k);
            sumAT = sumAT + AT.get(k);
        }

        // x.get(k) is node k's share of the total supply. Capacity has two
        // measures that generally disagree, so take node k's share of the
        // weight capacity, take its share of the area capacity, and average
        // the two. Each share already sums to 1 across the nodes, so their
        // average does too: sum(x) == 1 by construction, nothing to solve.
        // Twice the transports at a node means twice the supply.
        DoubleVec<CapNode> x = DoubleVec.of(numCapNodes, CapNode::new);
        double sumX = 0.0;
        for (CapNode k : x.keys()) {
            x.set(k, 0.5 * ((WT.get(k) / sumWT) + (AT.get(k) / sumAT)));
            sumX = sumX + x.get(k);
            System.out.printf("Node %2d supply share: %.6f\n", k.i(), x.get(k));
        }
        System.out.printf("Sum of x: %.4e\n", sumX);  // 1.0 by construction
        System.out.println();


        // Instantiate and verify the 'gravity model' C[k][a] = x[k] * s * D[a]
        DoubleGrid<CapNode, AType> C =
                DoubleGrid.of(numCapNodes, CapNode::new, numAssetTypes, AType::new); // C.get(k, a) is the supply of asset type a at node k

        for (CapNode k : C.rows()) {
            for (AType a : C.cols()) {
                C.set(k, a, x.get(k) * sdRatio * D.get(a));
            }
        }
        DoubleVec<AType> S = DoubleVec.of(numAssetTypes, AType::new);
        for (AType a : S.keys()) {
            S.set(a, 0.0);
            for (CapNode k : C.rows()) {
                S.add(a, C.get(k, a));
            }
            System.out.printf("Total supply of asset type %2d: %7.1f, with ratio %.6f\n", a.i(), S.get(a), S.get(a) / D.get(a) );
        }
        System.out.println("Done building supply\n");
    }


}
// =============================================
// Copyright Ben Paul Wise. All Rights Reserved.
// =============================================
