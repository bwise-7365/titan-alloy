package aji.model;

import java.util.ArrayList;
import java.util.Collections;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

/**
 * The input to the M-score: files with their layers, the dependencies among them, and
 * optional names for the layers. A model may also carry the project's test files and the
 * dependencies from tests to files; those take no part in the score and serve the test-reach
 * measure.
 *
 * <p>The constructor checks that file identifiers are unique and that every dependency joins
 * two known files. The collections are copied and cannot be changed afterwards.
 */
public record SystemModel(String name, List<FileNode> files, Set<Dep> deps,
                          Map<Integer, String> layerNames, List<String> tests, Set<Dep> testDeps) {

    public SystemModel {
        name = (name == null || name.isBlank()) ? "system" : name;
        files = List.copyOf(files);
        deps = Collections.unmodifiableSet(new LinkedHashSet<>(deps));
        layerNames = Collections.unmodifiableMap(new TreeMap<>(layerNames));
        tests = List.copyOf(new LinkedHashSet<>(tests));
        testDeps = Collections.unmodifiableSet(new LinkedHashSet<>(testDeps));
        Set<String> ids = new LinkedHashSet<>();
        for (FileNode f : files) {
            if (!ids.add(f.id())) {
                throw new IllegalArgumentException("file listed twice: " + f.id());
            }
        }
        for (Dep d : deps) {
            if (!ids.contains(d.from()) || !ids.contains(d.to())) {
                throw new IllegalArgumentException("dependency names an unknown file: "
                        + d.from() + " -> " + d.to());
            }
        }
        for (String t : tests) {
            if (ids.contains(t)) {
                throw new IllegalArgumentException("a file is listed as a test as well: " + t);
            }
        }
        Set<String> testIds = new LinkedHashSet<>(tests);
        for (Dep d : testDeps) {
            if (!testIds.contains(d.from()) || !ids.contains(d.to())) {
                throw new IllegalArgumentException("test dependency must join a test to a file: "
                        + d.from() + " -> " + d.to());
            }
        }
    }

    public SystemModel(String name, List<FileNode> files, Set<Dep> deps) {
        this(name, files, deps, Map.of(), List.of(), Set.of());
    }

    public SystemModel(String name, List<FileNode> files, Set<Dep> deps,
                       Map<Integer, String> layerNames) {
        this(name, files, deps, layerNames, List.of(), Set.of());
    }

    /** True when every file has a layer. */
    public boolean layered() {
        return !files.isEmpty() && files.stream().allMatch(f -> f.layer() != FileNode.UNASSIGNED);
    }

    public Map<String, Integer> layerOf() {
        Map<String, Integer> m = new HashMap<>();
        for (FileNode f : files) {
            m.put(f.id(), f.layer());
        }
        return m;
    }

    /** A copy with the layers replaced; files missing from the map become unassigned. */
    public SystemModel withLayers(Map<String, Integer> layers, Map<Integer, String> names) {
        List<FileNode> relabelled = new ArrayList<>();
        for (FileNode f : files) {
            relabelled.add(f.withLayer(layers.getOrDefault(f.id(), FileNode.UNASSIGNED)));
        }
        return new SystemModel(name, relabelled, deps, names, tests, testDeps);
    }

    /** A copy with one dependency taken out. */
    public SystemModel without(Dep d) {
        Set<Dep> kept = new LinkedHashSet<>(deps);
        kept.remove(d);
        return new SystemModel(name, files, kept, layerNames, tests, testDeps);
    }

    /** A copy with the tests and their dependencies. */
    public SystemModel withTests(List<String> newTests, Set<Dep> newTestDeps) {
        return new SystemModel(name, files, deps, layerNames, newTests, newTestDeps);
    }

    /** Files grouped by layer number, in increasing layer order. */
    public Map<Integer, List<String>> filesByLayer() {
        Map<Integer, List<String>> m = new TreeMap<>();
        for (FileNode f : files) {
            m.computeIfAbsent(f.layer(), k -> new ArrayList<>()).add(f.id());
        }
        return new LinkedHashMap<>(m);
    }
}
