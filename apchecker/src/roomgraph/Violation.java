package roomgraph;

import java.util.List;

/**
 * One finding against the plan.
 *
 * @param ruleId    short identifier of the rule, e.g. "APL-127" or "IRC-R302.5.1"
 * @param severity  see {@link Severity}
 * @param message   what is wrong, in plain words
 * @param rooms     the rooms implicated, drawn with a red outline
 * @param connections     the connections implicated, drawn as red dashed edges
 */
public record Violation(
        String ruleId,
        Severity severity,
        String message,
        List<String> rooms,
        List<Connection> connections) {

    public Violation {
        if (ruleId == null || ruleId.isBlank()) {
            throw new IllegalArgumentException("A violation must name its rule.");
        }
        if (severity == null) {
            throw new IllegalArgumentException("A violation must have a severity.");
        }
        if (message == null || message.isBlank()) {
            throw new IllegalArgumentException("A violation must carry a message.");
        }
        if (rooms == null || connections == null) {
            throw new IllegalArgumentException("Use empty lists, not null, for the implicated rooms and connections.");
        }
        rooms = List.copyOf(rooms);
        connections = List.copyOf(connections);
    }

    /** Convenience constructor for a finding about rooms only. */
    public static Violation ofRooms(String ruleId, Severity severity, String message, List<String> rooms) {
        return new Violation(ruleId, severity, message, rooms, List.of());
    }

    /** Convenience constructor for a finding about a single connection. */
    public static Violation ofConnection(String ruleId, Severity severity, String message, Connection connection) {
        return new Violation(ruleId, severity, message, List.of(connection.a(), connection.b()), List.of(connection));
    }

    @Override
    public String toString() {
        return "[" + this.severity + "] " + this.ruleId + ": " + this.message;
    }
}
