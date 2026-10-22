package roomgraph;

/**
 * What a room is for. The checks key off this, so a plan that mislabels a
 * kitchen as OTHER will simply not be checked for the kitchen rules; there is
 * deliberately no attempt to guess a kind from the room's label.
 */
public enum RoomKind {

    /** The outdoors. Exactly one per plan; it is the root of the justified graph. */
    EXTERIOR,

    /** Mud room, vestibule, foyer. Pattern 130, Entrance Room. */
    ENTRY_ROOM,

    /** The common area: family room, living room. Pattern 129. */
    COMMON,

    KITCHEN,

    DINING,

    /** A room whose only purpose is movement: hall, corridor. Pattern 132. */
    PASSAGE,

    /** A stair, treated as a room in its own right. Patterns 133 and 158. */
    STAIR,

    /** Any room containing a water closet or a tub. */
    BATH,

    BEDROOM,

    GARAGE,

    /** Library, study, workshop, laundry: rooms with no rule of their own here. */
    OTHER;

    /** A passage must declare its length so that pattern 132 can be checked. */
    public boolean requiresLength() {
        return this == PASSAGE;
    }
}
