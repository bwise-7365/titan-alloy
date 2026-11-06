package aji.spec;

import java.util.List;

/**
 * A declared layer: every file whose relative path matches one of the globs belongs to it.
 * Rules are tried in the order written, and the first match wins.
 */
public record LayerRule(int layer, String name, List<Glob> paths) {

    public LayerRule {
        if (layer < 1) {
            throw new IllegalArgumentException("layer numbers start at 1, not " + layer);
        }
        paths = List.copyOf(paths);
        name = name == null ? "" : name;
    }

    public boolean matches(String relPath) {
        return paths.stream().anyMatch(g -> g.matches(relPath));
    }
}
