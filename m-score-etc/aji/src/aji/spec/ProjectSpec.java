package aji.spec;

import java.nio.file.Path;
import java.util.List;

/**
 * What to measure: where the source is, which language it is in, what to leave out, and,
 * optionally, the declared layers.
 *
 * @param name         a short name used in reports and output folders
 * @param root         the project root; every relative path is relative to it
 * @param language     the source language
 * @param sourceDirs   directories (relative to the root) to search; empty means the root itself
 * @param includeDirs  C++ include directories (relative to the root), tried before the
 *                     file-name search; may be empty
 * @param excludes     files to leave out, in addition to the default exclusions when those are on
 * @param defaultExcludes whether to apply {@link #DEFAULT_EXCLUDES}
 * @param layers       declared layers; empty means the layers are derived from the dependencies
 */
public record ProjectSpec(String name, Path root, Language language, List<String> sourceDirs,
                          List<String> includeDirs, List<Glob> excludes, boolean defaultExcludes,
                          List<LayerRule> layers) {

    /** Build output, version control and IDE folders. */
    public static final List<String> DEFAULT_EXCLUDES = List.of(
            "**/.git/**", "**/.idea/**", "**/.vs/**", "**/.vscode/**",
            "**/build/**", "**/cmake-build-*/**", "**/out/**", "**/target/**",
            "**/node_modules/**");

    public ProjectSpec {
        sourceDirs = List.copyOf(sourceDirs);
        includeDirs = List.copyOf(includeDirs);
        excludes = List.copyOf(excludes);
        layers = List.copyOf(layers);
    }

    public boolean declaresLayers() {
        return !layers.isEmpty();
    }

    /** The rules actually applied: the defaults (when on) followed by the project's own. */
    public List<Glob> allExcludes() {
        if (!defaultExcludes) {
            return excludes;
        }
        List<Glob> all = new java.util.ArrayList<>();
        DEFAULT_EXCLUDES.forEach(p -> all.add(Glob.of(p)));
        all.addAll(excludes);
        return all;
    }
}
