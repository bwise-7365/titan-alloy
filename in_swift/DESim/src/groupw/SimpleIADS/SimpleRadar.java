/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */

package groupw.SimpleIADS;

import groupw.BaseSim.Entity;
import groupw.BaseSim.Scheduler;

import java.util.Map;

/**
 *
 * @author BenWise
 */
public class SimpleRadar
        extends Entity {

    public SimpleRadar(String tn, Scheduler s) {
        super(s);
        typeName = tn;
        
    }

    public double probDetect(double rcs, double range) {
        // the power returned from 'rcs' at 'range' is
        // pwr = c * rcs / (r0+range)^4, for a constant 'c' that depends on the radar
        // the calculation is pwr^2 / (pwr50^2 + pwr^2) = signal/(signal + noise),
        // rearranged so that 'c' cancels
        // Intuitive check:
        // (A) if 0 < rcs and 0 = range, then 0 < num and 0 = dnm, so p = 1 if r0 = 0
        //  because range50 is about 10^8, p ~~ rcs*10^8/ (rcs*10^8 + 1) with nominal parameters
        // (B) if 0 = rcs and 0 < range, then 0 = num and 0 < dnm, so p = 0
        double r50 = rangeZero + range50;
        double r = rangeZero + range;
        double num = rcs * (r50 * r50 * r50 * r50);
        double dnm = rcs50 * (r * r * r * r);
        double p = num / (num + dnm);
        return p;
    }

    public boolean detect(double rcs, double range) {
        double p = probDetect(rcs, range);
        boolean d = false;
        if (stochasticP) {
            d = mySim.prob(p);
        } else {
            if (0.50 <= p) {
                d = true;
            } else {
                d = false;
            }
        }
        return d;
    }
    

    @Override
    public void process() {
        throw new UnsupportedOperationException("Not supported yet.");
    }

    
    /**
     * RadarCrossSectionMap specifies the RCS, in square meters of various Hull 
     */
    public static Map<String, Double> RadarCrossSectionMap = null;


    // TODO: have a RealVector center, or use the Hull.
    // TODO: have four rays to define field of view

    // The basic performance parameters are rcs50 and range50.
    // They are linked because range50 is the range at which a target with
    // RCS of rcs50 has 50% chance of detection in one scan.
    // The Wikipedia data imply that if a radar can see at 2.5 m^2 target
    // at 100Km with 50% prob, then it could detect a 0.001 m^2 at 14Km and
    // a 100 m^2 at 251Km, each with 50% probability per scan
    public double rcs50 = 2.5; // meters squared
    public double range50 = 100000.0; // meters
    public double rangeZero = 1.0; // meters, to prevent division by zero
    public String typeName = "genericRadar";
    public double period = 10.0; // seconds between scans
    public boolean stochasticP = false;
}


// =============================================================================
