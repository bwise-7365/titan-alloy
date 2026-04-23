/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics;

/**
 *
 * @author DavidHa
 */
public class Vehicle {
   private final String name;
   private VehicleType vehicleType = null;
   private VehicleLoad vehicleLoad = null;

   public Vehicle(String name){
    this.name = name; 
   }

    /**
     * @return the name
     */
    public String getName() {
        return name;
    }

    /**
     * @return the vehicleType
     */
    public VehicleType getVehicleType() {
        return vehicleType;
    }

    /**
     * @param vehicleType the vehicleType to set
     */
    public void setVehicleType(VehicleType vehicleType) {
        this.vehicleType = vehicleType;
    }

    /**
     * @return the vehicleLoad
     */
    public VehicleLoad getVehicleLoad() {
        return vehicleLoad;
    }
}
// =============================================================================
