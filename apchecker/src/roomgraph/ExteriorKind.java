package roomgraph;

/**
 * What sort of outdoors an EXTERIOR vertex stands for.
 *
 * The outdoors is not one thing. A bedroom giving onto a private garden and a
 * bedroom giving onto the sidewalk are the same graph and utterly different
 * houses, and a single exterior vertex could not tell them apart.
 *
 * Note what several exterior vertices do NOT fix: two doors onto the same STREET
 * still share a vertex, so walking out the front door and in at the garage door
 * remains a walk of length two. That artifact is suppressed where it matters by
 * the avoid= and exclude= arguments in rules.xml, not by this enum.
 */
public enum ExteriorKind {

    /** The public way: where one arrives, and where egress must discharge. */
    STREET,

    /** Vehicle approach; often the only thing a garage door opens onto. */
    DRIVEWAY,

    /** Private outdoor ground. A bedroom door onto it is not a privacy fault. */
    GARDEN,

    /** Paved private outdoor space adjoining the house. */
    TERRACE,

    /** Enclosed outdoor space with rooms on several sides. */
    COURT,

    /** Anything else outdoors that a rule need not distinguish. */
    OTHER
}
