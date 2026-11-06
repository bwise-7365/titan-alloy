package aji.extract;

import aji.model.Dep;

import java.util.List;
import java.util.Set;

/** Dependencies found in a set of source files, with notes about anything doubtful. */
public record Found(Set<Dep> deps, List<String> notes) {

    public Found {
        deps = Set.copyOf(deps);
        notes = List.copyOf(notes);
    }
}
