package roomgraph;

/**
 * How two rooms are joined.
 *
 * Not every adjacency is a door. A living room usually gives onto its hall
 * through a cased opening with no leaf in it, and that is a different fact about
 * the house from a door: it cannot be closed, it gives no acoustic or visual
 * separation, and a code that requires a garage to be "equipped with" a door is
 * not satisfied by an archway.
 *
 * The rule language can therefore quantify over connections of one kind: see the
 * via= attribute of noEdge in rules.xsd.
 */
public enum ConnectionKind {

    /** A door: a closable leaf in the opening. */
    DOOR,

    /** A cased opening, arch, or open boundary. No leaf; cannot be closed. */
    OPENING
}
