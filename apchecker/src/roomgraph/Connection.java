package roomgraph;

/**
 * One undirected edge of the plan graph: a connection, or a cased opening with no leaf
 * in it. Direction is not meaningful, so equality of the pair is defined on the
 * unordered pair.
 *
 * This type used to be called Door, back when the model assumed every adjacency
 * was one. It is not: a room that simply gives onto a hall is joined to it, and
 * calling that a connection in the code would be a lie that some later rule would trip
 * over.
 *
 * The KIND is not part of the identity of the edge. Two rooms are joined once,
 * and asking whether they are joined "twice, differently" is not a question the
 * plan may pose: the reader rejects a duplicate pair whatever its kind.
 */
public record Connection(String a, String b, ConnectionKind kind) {

    public Connection {
        if (a == null || a.isBlank() || b == null || b.isBlank()) {
            throw new IllegalArgumentException("A connection must name two rooms.");
        }
        if (a.equals(b)) {
            throw new IllegalArgumentException("A connection cannot join room '" + a + "' to itself.");
        }
        if (kind == null) {
            throw new IllegalArgumentException("Connection " + a + "-" + b + " must say whether it is a DOOR or an OPENING.");
        }
    }

    /** True when this connection joins exactly the two given rooms, in either order. */
    public boolean joins(String x, String y) {
        return (this.a.equals(x) && this.b.equals(y)) || (this.a.equals(y) && this.b.equals(x));
    }

    /** True when this connection touches the given room. */
    public boolean touches(String x) {
        return this.a.equals(x) || this.b.equals(x);
    }

    /** Canonical unordered key, so that (a,b) and (b,a) are one connection. */
    public String key() {
        return (this.a.compareTo(this.b) <= 0) ? (this.a + "|" + this.b) : (this.b + "|" + this.a);
    }

    /** The same connection with its ends swapped; used to orient a witness. */
    public Connection reversed() {
        return new Connection(this.b, this.a, this.kind);
    }
}
