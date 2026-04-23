/*
 *  ---------------------------------------------------
 *        Copyright Group W. All Rights Reserved.
 *  ---------------------------------------------------
 */
package groupw.Logistics;

import java.util.HashMap;
import java.util.HashSet;
import java.util.Map;
import java.util.Set;

/**
 *
 * @author DavidHa
 */
public class UnitType {
    private String typeName;
    private Set<String> postures = new HashSet<>(2); // probably at least 2 postures

    /**
     * Daily consumption rates of supplies, for each possible posture
     * of this type of unit.
     * NOTE: it is pointless to make this private, then provide
     * getter and setter that enable direct access.
     */
    public Map<String, Manifest> dcRates = new HashMap<>(2); // probably at least 2 postures

    public UnitType() {
    }

    public UnitType(String type) {
        this.typeName = type;
    }

    /**
     * Get the daily consumption rates for this type of unit in the given posture.
     * Returns NULL if not manifest was provided with the given posture.
     * @param posture
     * @return Manifest of daily consumption rates
     */
    public Manifest getDailyConsRate(String posture) {
        Manifest m = dcRates.get(posture);
        return m;
    }

    public void setDailyConsRate(String p, Manifest dcr) {
        dcRates.put(p, dcr);
    }

    /**
     * @return the type
     */
    public String getTypeName() {
        return typeName;
    }

    /**
     * @param typeName the type to set
     */
    public void setTypeName(String typeName) {
        this.typeName = typeName;
    }

    /**
     * @return the postures
     */
    public Set<String> getPostures() {
        return postures;
    }

    /**
     * @param postures the postures to set
     */
    public void setPostures(Set<String> postures) {
        this.postures = postures;
    }
}
// =============================================================================
