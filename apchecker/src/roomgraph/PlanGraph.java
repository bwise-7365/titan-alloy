package roomgraph;

import java.util.ArrayList;
import java.util.Collections;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The plan as an immutable undirected graph: rooms are vertices, connections are
 * edges. Nothing here knows about the design rules; the rules live in the
 * classes under {@code roomgraph.checks}, and the pure graph algorithms live
 * in {@link Graphs}.
 */
public final class PlanGraph {

    private final String name;
    private final Map<String, Room> rooms;
    private final List<Connection> connections;
    private final Map<String, List<String>> adjacency;
    private final List<String> exteriorId;

    public PlanGraph(String name, List<Room> roomList, List<Connection> doorList) {
        if (name == null || name.isBlank()) {
            throw new IllegalArgumentException("The plan must have a name.");
        }
        if (roomList == null || doorList == null) {
            throw new IllegalArgumentException("The plan must have room and connection lists.");
        }

        Map<String, Room> byId = new LinkedHashMap<>();
        for (Room room : roomList) {
            Room previous = byId.put(room.id(), room);
            if (previous != null) {
                throw new IllegalArgumentException("Duplicate room id: '" + room.id() + "'.");
            }
        }

        // ANY NUMBER of exterior vertices, including none.
        //
        // None, because the empty design has placed nothing, not even the
        // outdoors, and is still a legitimate node of a search - the root of it.
        //
        // Several, because the outdoors is not one thing. The street, the
        // driveway and the back garden are different places, and a bedroom door
        // onto the garden is not the privacy fault that a bedroom door onto the
        // street would be.
        //
        // Note what this does NOT fix: two doors onto the same STREET still share
        // a vertex, so walking out the front and in at the garage is still a walk
        // of length two. The avoid= and exclude= arguments in rules.xml are what
        // suppress that, and they are still needed.
        List<String> exteriors = new ArrayList<>();
        for (Room room : byId.values()) {
            if (room.kind() == RoomKind.EXTERIOR) {
                exteriors.add(room.id());
            }
        }
        this.exteriorId = List.copyOf(exteriors);

        Map<String, List<String>> adj = new LinkedHashMap<>();
        for (String id : byId.keySet()) {
            adj.put(id, new ArrayList<>());
        }
        Set<String> seenDoors = new LinkedHashSet<>();
        List<Connection> uniqueDoors = new ArrayList<>();
        for (Connection connection : doorList) {
            if (!byId.containsKey(connection.a())) {
                throw new IllegalArgumentException("Connection refers to unknown room '" + connection.a() + "'.");
            }
            if (!byId.containsKey(connection.b())) {
                throw new IllegalArgumentException("Connection refers to unknown room '" + connection.b() + "'.");
            }
            if (!seenDoors.add(connection.key())) {
                throw new IllegalArgumentException("Duplicate connection between '" + connection.a() + "' and '" + connection.b() + "'.");
            }
            uniqueDoors.add(connection);
            adj.get(connection.a()).add(connection.b());
            adj.get(connection.b()).add(connection.a());
        }

        Map<String, List<String>> frozen = new LinkedHashMap<>();
        for (Map.Entry<String, List<String>> entry : adj.entrySet()) {
            frozen.put(entry.getKey(), List.copyOf(entry.getValue()));
        }

        this.name = name;
        this.rooms = Collections.unmodifiableMap(byId);
        this.connections = List.copyOf(uniqueDoors);
        this.adjacency = Collections.unmodifiableMap(frozen);
    }

    public String name() {
        return this.name;
    }

    /** All rooms, in the order they were declared. */
    public List<Room> rooms() {
        return List.copyOf(this.rooms.values());
    }

    /**
     * The given rooms, in the order the plan declares them.
     *
     * Every set the rule engine produces is unordered, but a report must be
     * canonical: two conforming implementations of the rule language have to
     * emit the same findings in the same order, or their outputs cannot be
     * compared. Declaration order is the one order the plan file itself
     * supplies, so it is the one the specification fixes.
     */
    public List<String> ordered(java.util.Collection<String> someRooms) {
        List<String> canonical = new java.util.ArrayList<>();
        for (String id : roomIds()) {
            if (someRooms.contains(id)) {
                canonical.add(id);
            }
        }
        return List.copyOf(canonical);
    }

    public List<String> roomIds() {
        return List.copyOf(this.rooms.keySet());
    }

    public List<Connection> connections() {
        return this.connections;
    }

    /**
     * Every outdoor vertex, in declaration order; empty for the empty design.
     * Callers must cope with both: a traversal from nowhere reaches nothing, and
     * a traversal from several places is multi-source.
     */
    public Set<String> exteriorIds() {
        return new LinkedHashSet<>(this.exteriorId);
    }

    /** True when the design has placed nothing at all. */
    public boolean isEmpty() {
        return this.rooms.isEmpty();
    }

    /** Look up a room. Throws rather than returning null for an unknown id. */
    public Room room(String id) {
        Room room = this.rooms.get(id);
        if (room == null) {
            throw new IllegalArgumentException("No such room: '" + id + "'.");
        }
        return room;
    }

    public List<String> neighbors(String id) {
        List<String> list = this.adjacency.get(id);
        if (list == null) {
            throw new IllegalArgumentException("No such room: '" + id + "'.");
        }
        return list;
    }

    public int degree(String id) {
        return neighbors(id).size();
    }

    /** Every room of the given kind, in declaration order. */
    public List<Room> roomsOfKind(RoomKind kind) {
        List<Room> found = new ArrayList<>();
        for (Room room : this.rooms.values()) {
            if (room.kind() == kind) {
                found.add(room);
            }
        }
        return found;
    }

    /** Every room whose privacy rank equals the given level. */
    public List<Room> roomsAtLevel(PrivacyLevel level) {
        List<Room> found = new ArrayList<>();
        for (Room room : this.rooms.values()) {
            if (room.privacy() == level) {
                found.add(room);
            }
        }
        return found;
    }

    /** Every room except the outdoors. */
    public Set<String> interiorIds() {
        Set<String> ids = new LinkedHashSet<>(this.rooms.keySet());
        ids.removeAll(this.exteriorId);
        return ids;
    }
}
