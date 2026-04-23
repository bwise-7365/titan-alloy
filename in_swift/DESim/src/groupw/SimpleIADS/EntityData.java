/*
 * ---------------------------------------------------
 *  Copyright Group W. All Rights Reserved.
 * ---------------------------------------------------
 */

package groupw.SimpleIADS;

import static groupw.SimpleIADS.DamageServer.DetonationSSPK;
import static groupw.SimpleIADS.DamageServer.DetonationScanRange;
import static groupw.SimpleIADS.SimpleRadar.RadarCrossSectionMap;
import java.util.HashMap;
import java.util.Map;

/**
 *
 * @author bwise
 */
public class EntityData {
    // TODO: this data should be read from files

    final static String a2aMissileBlue01 = "Blue A2A Missile MediumRange"; // Mach 4, 60 nm, nothing more specific for a demo
    final static String a2aMissileBlue02 = "Blue A2A Missile ShortRange";  // Mach 2, 20 nm, nothing more specific for a demo
    final static String a2aMissileRed01 = "Red A2A Missile MediumRange";  // Mach 4, 60 nm, nothing more specific for a demo
    final static String a2aMissileRed02 = "Red A2A Missile ShortRange";   // Mach 2, 20 nm, nothing more specific for a demo

    final static String fwaBlue01 = "Blue Fighter 01"; // older
    final static String fwaBlue02 = "Blue Fighter 02"; // newer
    final static String fwaRed01 = "Red Fighter 01"; // older
    final static String fwaRed02 = "Red Fighter 02";  // newer
    
    
    /**
     * parameterizeRCSMap provides RCS data for various Hull
     * As of 2024-12, https://en.wikipedia.org/wiki/Radar_cross_section
     * gives some nominal values: 
     * Typical values for a centimeter wave radar are:[8][9] 
     * Insect: 0.00001 m2 
     * Bird: 0.01 m2 
     * Stealth aircraft: less than 0.1 m2 (e.g. F-117A: 0.001 m2) 
     * Surface-to-air-missile: ≈0.1 m2 
     * Human: 1 m2
     * small combat aircraft: 2–3 m2 
     * large combat aircraft: 5–6 m2 
     * Cargo aircraft: up to 100 m2 
     * Coastal trading vessel (55 m length): 300–4000 m2 
     * Corner reflector with 1.5 m edge length: ≈20,000 m2[10][11] 
     * Frigate (103 m length): 5000–100,000 m2 
     * Container ship (212 m length): 10,000–80,000 m2
     *
     * So it might not be unreasonable to treat a bomber, tanker, or cargo plane
     * as 100m^2, fighters as 2-6 m^2, missiles as 0.1 m^2, and similar.
     * 
     */
    static public void parameterizeRCSMap() {
      // TODO: this data about Hulls, not Radars, should be read from files
        RadarCrossSectionMap = new HashMap<String, Double>();
        
        // Use wikipedia values for S2A (!) missiles, with medium range > short range
        RadarCrossSectionMap.put(EntityData.a2aMissileBlue01, 0.5);
        RadarCrossSectionMap.put(EntityData.a2aMissileBlue02, 0.1);
        RadarCrossSectionMap.put(EntityData.a2aMissileRed01, 0.5);
        RadarCrossSectionMap.put(EntityData.a2aMissileRed02, 0.1);
        
        // use wikipedia values for 'smal' and 'large' non-stealthy combat fwa
        RadarCrossSectionMap.put(EntityData.fwaBlue01, 5.5); // older
        RadarCrossSectionMap.put(EntityData.fwaBlue02, 2.5); // newer
        RadarCrossSectionMap.put(EntityData.fwaRed01, 5.5); // older
        RadarCrossSectionMap.put(EntityData.fwaRed02, 2.5); // newer
        
    }
        
    static public void parameterizeDamageServer() {
        // TODO: this data should be read from files
        System.out.println("Building damage SSPK and scan-range tables");
        assert (null != DamageServer.theDS);
        DetonationSSPK = new HashMap<String, Map<String, Double>>();
        DetonationScanRange = new HashMap<String, Double>();

        Map<String, Double> sspk1 = new HashMap<String, Double>();
        sspk1.put(EntityData.fwaBlue01, 10.0); // 10 meter for 50% kill blast radius
        sspk1.put(EntityData.fwaBlue02, 10.0);
        sspk1.put(EntityData.fwaRed01, 10.0);
        sspk1.put(EntityData.fwaRed02, 10.0);
        DetonationSSPK.put(EntityData.a2aMissileBlue01, sspk1);
        DetonationScanRange.put(EntityData.a2aMissileBlue01, 50.0); // 50 meter max

        DetonationSSPK.put(EntityData.a2aMissileRed01, sspk1);
        DetonationScanRange.put(EntityData.a2aMissileRed01, 50.0); // 50 meter max

        Map<String, Double> sspk2 = new HashMap<String, Double>();
        sspk2.put(EntityData.fwaBlue01, 20.0); // 20 meter for 50% kill blast radius
        sspk2.put(EntityData.fwaBlue02, 20.0);
        sspk2.put(EntityData.fwaRed01, 20.0);
        sspk2.put(EntityData.fwaRed02, 20.0);
        DetonationSSPK.put(EntityData.a2aMissileBlue02, sspk2);
        DetonationScanRange.put(EntityData.a2aMissileBlue02, 100.0); // 100 meter max

        DetonationSSPK.put(EntityData.a2aMissileRed02, sspk2);
        DetonationScanRange.put(EntityData.a2aMissileRed02, 100.0); // 100 meter max
    }

}
// =============================================================================
