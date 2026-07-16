package roomgraph;

import java.util.Optional;
import java.util.OptionalDouble;

/**
 * One vertex of the plan graph.
 *
 * @param id         unique key used by doors and by the checks
 * @param label      human-readable name, used only for the drawing
 * @param kind       what the room is for
 * @param privacy    where the room sits on the intimacy gradient
 * @param lengthFeet travel length of the room; required for PASSAGE, forbidden
 *                   otherwise, because the only rule that reads it is the
 *                   50-foot limit of pattern 132
 */
public record Room(
        String id,
        String label,
        RoomKind kind,
        PrivacyLevel privacy,
        OptionalDouble lengthFeet,
        Optional<ExteriorKind> outdoor) {

    public Room {
        if (id == null || id.isBlank()) {
            throw new IllegalArgumentException("Room id must be a non-blank string.");
        }
        if (label == null || label.isBlank()) {
            throw new IllegalArgumentException("Room '" + id + "' has no label.");
        }
        if (kind == null) {
            throw new IllegalArgumentException("Room '" + id + "' has no kind.");
        }
        if (privacy == null) {
            throw new IllegalArgumentException("Room '" + id + "' has no privacy level.");
        }
        if (lengthFeet == null) {
            throw new IllegalArgumentException("Room '" + id + "' has a null length; use OptionalDouble.empty().");
        }
        // Fail loudly rather than substituting a default length: a corridor of
        // unknown length cannot be checked against pattern 132, and silently
        // passing it would hide the very error we are looking for.
        if (kind.requiresLength() && lengthFeet.isEmpty()) {
            throw new IllegalArgumentException("Passage '" + id + "' must declare lengthFeet.");
        }
        if (!kind.requiresLength() && lengthFeet.isPresent()) {
            throw new IllegalArgumentException("Room '" + id + "' is not a PASSAGE, so it must not declare lengthFeet.");
        }
        if (lengthFeet.isPresent() && lengthFeet.getAsDouble() <= 0.0) {
            throw new IllegalArgumentException("Passage '" + id + "' has a non-positive length.");
        }
        if (kind == RoomKind.EXTERIOR && privacy != PrivacyLevel.EXTERIOR) {
            throw new IllegalArgumentException("Room '" + id + "': kind EXTERIOR requires privacy EXTERIOR.");
        }
        if (privacy == PrivacyLevel.EXTERIOR && kind != RoomKind.EXTERIOR) {
            throw new IllegalArgumentException("Room '" + id + "': privacy EXTERIOR requires kind EXTERIOR.");
        }
        if (outdoor == null) {
            throw new IllegalArgumentException("Room '" + id + "': use Optional.empty(), not null, for outdoor.");
        }
        // An exterior vertex must say WHICH outdoors it is. A plan with a garden
        // door and a street door has two vertices, and a rule that cares about
        // the difference - the front door must not open into a bedroom, but a
        // garden door may - has to be able to tell them apart.
        if (kind == RoomKind.EXTERIOR && outdoor.isEmpty()) {
            throw new IllegalArgumentException("Exterior '" + id + "' must declare which outdoors it is.");
        }
        if (kind != RoomKind.EXTERIOR && outdoor.isPresent()) {
            throw new IllegalArgumentException("Room '" + id + "' is not EXTERIOR, so it must not declare an outdoor kind.");
        }
    }
}
