package aji.extract;

import aji.model.FileNode;
import aji.model.SystemModel;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileVisitResult;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.SimpleFileVisitor;
import java.nio.file.attribute.BasicFileAttributes;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;
import java.util.TreeSet;

/**
 * Turns a {@link ProjectSpec} into a {@link SystemModel}: finds the source files, applies the
 * exclusions and the declared layers, and extracts the dependencies. When no layers are
 * declared (or the caller asks to ignore them), the files are left unassigned, and the caller
 * derives layers from the dependencies. Reading the source files is its only side effect.
 */
public final class Extractor {

    private Extractor() {
    }

    /**
     * The extracted model and how it was made.
     *
     * @param model       files, layers (or none) and dependencies
     * @param layering    "declared", or "none" when the files have no layers yet
     * @param unlayered   files dropped because no declared layer matched them
     * @param notes       remarks from dependency extraction
     */
    public record Extraction(SystemModel model, String layering, List<String> unlayered,
                             List<String> notes) {
    }

    /**
     * @param derive when true, leave the files without layers even if the project declares
     *               layers; the declared layers then serve only to select the files
     */
    public static Extraction run(ProjectSpec spec, boolean derive) throws IOException {
        List<String> paths = listSources(spec);
        Map<String, Integer> declared = new HashMap<>();
        List<String> unlayered = new ArrayList<>();
        List<String> kept = new ArrayList<>();
        for (String p : paths) {
            if (!spec.declaresLayers()) {
                kept.add(p);
                continue;
            }
            LayerRule rule = spec.layers().stream().filter(r -> r.matches(p)).findFirst().orElse(null);
            if (rule == null) {
                unlayered.add(p);
            } else {
                declared.put(p, rule.layer());
                kept.add(p);
            }
        }
        List<SourceUnit> units = new ArrayList<>();
        for (String p : kept) {
            units.add(new SourceUnit(p, read(spec.root().resolve(p))));
        }
        Found found = spec.language() == Language.JAVA
                ? JavaDeps.find(units)
                : CppDeps.find(units, spec.includeDirs());
        boolean useDeclared = spec.declaresLayers() && !derive;
        List<FileNode> nodes = new ArrayList<>();
        for (String p : kept) {
            nodes.add(new FileNode(p, useDeclared ? declared.get(p) : FileNode.UNASSIGNED));
        }
        SystemModel model = new SystemModel(spec.name(), nodes, found.deps(),
                useDeclared ? layerNames(spec) : Map.of());
        return new Extraction(model, useDeclared ? "declared" : "none", unlayered, found.notes());
    }

    /** Relative paths (forward slashes, sorted) of the source files that are not excluded. */
    static List<String> listSources(ProjectSpec spec) throws IOException {
        Path root = spec.root().toAbsolutePath().normalize();
        if (!Files.isDirectory(root)) {
            throw new IOException("project root is not a directory: " + root);
        }
        List<Glob> excludes = spec.allExcludes();
        List<Path> starts = new ArrayList<>();
        if (spec.sourceDirs().isEmpty()) {
            starts.add(root);
        } else {
            for (String d : spec.sourceDirs()) {
                starts.add(root.resolve(d).normalize());
            }
        }
        TreeSet<String> found = new TreeSet<>();
        for (Path start : starts) {
            if (!Files.isDirectory(start)) {
                throw new IOException("source directory not found: " + start);
            }
            Files.walkFileTree(start, new SimpleFileVisitor<>() {
                @Override
                public FileVisitResult preVisitDirectory(Path dir, BasicFileAttributes attrs) {
                    String rel = relative(root, dir);
                    if (!rel.isEmpty() && excluded(excludes, rel + "/")) {
                        return FileVisitResult.SKIP_SUBTREE;
                    }
                    return FileVisitResult.CONTINUE;
                }

                @Override
                public FileVisitResult visitFile(Path file, BasicFileAttributes attrs) {
                    String rel = relative(root, file);
                    if (attrs.isRegularFile() && spec.language().accepts(file.getFileName().toString())
                            && !excluded(excludes, rel)) {
                        found.add(rel);
                    }
                    return FileVisitResult.CONTINUE;
                }
            });
        }
        return new ArrayList<>(found);
    }

    private static boolean excluded(List<Glob> excludes, String rel) {
        return excludes.stream().anyMatch(g -> g.matches(rel));
    }

    private static String relative(Path root, Path p) {
        return root.relativize(p.toAbsolutePath().normalize()).toString().replace('\\', '/');
    }

    private static String read(Path p) throws IOException {
        String s = new String(Files.readAllBytes(p), StandardCharsets.UTF_8);
        return s.startsWith("﻿") ? s.substring(1) : s;
    }

    private static Map<Integer, String> layerNames(ProjectSpec spec) {
        Map<Integer, String> names = new TreeMap<>();
        for (LayerRule r : spec.layers()) {
            if (!r.name().isBlank()) {
                names.merge(r.layer(), r.name(), (a, b) -> a.equals(b) ? a : a + ", " + b);
            }
        }
        return names;
    }
}
