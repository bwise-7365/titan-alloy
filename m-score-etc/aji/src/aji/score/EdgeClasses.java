package aji.score;

import aji.model.Dep;

import java.util.List;

/**
 * Every dependency falls into exactly one class.
 *
 * @param internal both files are in the same module
 * @param downward a file in a lower layer depends on a file in a higher layer (the normal case)
 * @param upward   a file in a higher layer depends on a file in a lower layer: a layering
 *                 violation, which the M-score's model assumes does not occur
 */
public record EdgeClasses(List<Dep> internal, List<Dep> downward, List<Dep> upward) {

    public EdgeClasses {
        internal = List.copyOf(internal);
        downward = List.copyOf(downward);
        upward = List.copyOf(upward);
    }
}
