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
public class Unit {

    private final String name;
    private UnitType unitType;

    public Unit(String name, UnitType unitType) {
        this.name = name;
        this.unitType = unitType;
        this.sStatus = new SupplyStatus(); // all zeros
    }

    /**
     * @return the name
     */
    public String getName() {
        return name;
    }

    /**
     * @return the unitType
     */
    public UnitType getType() {
        return unitType;
    }

    /**
     * @param unitType the unitType to set
     */
    public void setType(UnitType unitType) {
        this.unitType = unitType;
    }

    /**
     * Reset the current posture and maintains data consistency be
     * resetting the daily consumption rate as implied by the posture.
     * @param p new posture to be used
     */
    public void setPosture(String p){
        postureName = p;
        // TODO: throw error if dcRates map is null or empty.
        Manifest m = unitType.dcRates.get(p);
        // TODO: throw error if 'p' is not a posture of this unit, i.e. the manifest is null or empty.
        sStatus.consumptionRate = m;
    }

    // NOTE WELL: (unitType, posture) determines consumptionRate
    public String postureName="";
    public SupplyStatus sStatus;

}
// =============================================================================
