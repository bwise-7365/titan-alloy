package roomgraph;

/**
 * The privacy rank used by Alexander's Intimacy Gradient (pattern 127).
 *
 * The rank is an ordinal scale, not a measurement: the only thing the checks
 * rely on is the ordering EXTERIOR &lt; ENTRY &lt; PUBLIC &lt; SEMI_PRIVATE &lt; PRIVATE.
 * Every room in the plan must be assigned one explicitly; there is no default.
 */
public enum PrivacyLevel {

    /** Outside the building. Exactly one room in a plan carries this rank. */
    EXTERIOR(0),

    /** Mud room, vestibule, entrance room: the buffer between outside and inside. */
    ENTRY(1),

    /** Living room, dining room, kitchen, hall: seen by any visitor. */
    PUBLIC(2),

    /** Family room, study, shared bath: seen by friends but not by strangers. */
    SEMI_PRIVATE(3),

    /** Bedrooms, ensuite baths: seen only by the occupants. */
    PRIVATE(4);

    private final int rank;

    PrivacyLevel(int rank) {
        this.rank = rank;
    }

    /** Position on the gradient. Higher means more private. */
    public int rank() {
        return this.rank;
    }

    /** True when this level is at least as private as the other. */
    public boolean atLeastAsPrivateAs(PrivacyLevel other) {
        return this.rank >= other.rank;
    }
}
