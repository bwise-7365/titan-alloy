/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics;

import java.util.Set;

/**
 *
 * @author DavidHa
 */
public class VehicleType {
    public String vehicleTypeName; // different names for different things of different types
    public String fuelType;
    public double availability;
    public double fuelEfficiency;
    public double maxRange;
    public double maxSpeed;
    public Set<String> domains;
    
    public VehicleType() {
    }

    public VehicleType(
        String vehicleTypeName,
        String fuelType,
        double availability,
        double fuelEfficiency,
        double maxRange,
        double maxSpeed,
        Set<String> domains) {
        this.vehicleTypeName = vehicleTypeName;
        this.fuelType = fuelType;
        this.availability = availability;
        this.fuelEfficiency = fuelEfficiency;
        this.maxRange = maxRange;
        this.maxSpeed = maxSpeed;
        this.domains = domains;
    }

}
// =============================================================================
