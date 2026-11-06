package aji.score;

import aji.graph.Graphs;
import aji.model.Dep;
import aji.model.FileNode;
import aji.model.Module;
import aji.model.Params;
import aji.model.SystemModel;

import java.util.ArrayList;
import java.util.Collection;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.TreeSet;

/**
 * The M-score of Pisch, Cai, Kazman, Lefever and Fang (ESEM 2024), computed on layers that are
 * supplied rather than clustered:
 *
 * <pre>
 *   M-score = sum over layers L, modules M of L:  SF(M) * (1 - CLDDP(M)) * (1 - IMCP(M))
 *   SF      = |M| / N * SP,        SP = 1 / log5 |M| when |M| >= 5, else 1
 *   CLDDP   = lower-layer dependencies / (lower-layer files * |M|), and 0 in the bottom layer
 *   IMCP    = internal dependencies / (|M| (|M| - 1)) when |M| >= 5, else 0
 * </pre>
 *
 * <p>Every function here is pure.
 */
public final class MScore {

    private static final double LN5 = Math.log(5);

    private MScore() {
    }

    /** Thrown when upward dependencies exist and the parameters say to fail. */
    public static final class UpwardDependencyException extends RuntimeException {
        private static final long serialVersionUID = 1L;
        private final transient List<Dep> upward;

        public UpwardDependencyException(List<Dep> upward) {
            super(upward.size() + " upward (layer-violating) dependencies; first: "
                    + upward.get(0).from() + " -> " + upward.get(0).to());
            this.upward = List.copyOf(upward);
        }

        public List<Dep> upward() {
            return upward;
        }
    }

    public static Result evaluate(SystemModel m, Params p) {
        return evaluate(m, p, Graphs.propagationCost(m));
    }

    /**
     * As {@link #evaluate(SystemModel, Params)}, for a caller that already knows the
     * propagation cost. That cost depends on the dependencies alone, so it is the same for
     * every layering of one set of files.
     */
    public static Result evaluate(SystemModel m, Params p, double propagationCost) {
        if (m.files().isEmpty()) {
            throw new IllegalArgumentException("the system has no files; the M-score is undefined");
        }
        if (!m.layered()) {
            throw new IllegalArgumentException("some files have no layer");
        }
        List<Integer> declared = new ArrayList<>(new TreeSet<>(m.layerOf().values()));
        Map<Integer, Integer> rankOfLayer = new HashMap<>();
        for (int r = 0; r < declared.size(); r++) {
            rankOfLayer.put(declared.get(r), r + 1);
        }
        int n = declared.size();
        int bigN = m.files().size();
        Map<String, Integer> rank = new HashMap<>();
        Map<Integer, List<String>> filesOfRank = new TreeMap<>();
        for (FileNode f : m.files()) {
            int r = rankOfLayer.get(f.layer());
            rank.put(f.id(), r);
            filesOfRank.computeIfAbsent(r, k -> new ArrayList<>()).add(f.id());
        }
        int[] filesPerRank = new int[n + 2];
        filesOfRank.forEach((r, fs) -> filesPerRank[r] = fs.size());

        List<Module> modules = new ArrayList<>();
        Map<String, Module> moduleOf = new HashMap<>();
        for (Map.Entry<Integer, List<String>> e : filesOfRank.entrySet()) {
            for (Module mod : modulesOfLayer(e.getKey(), e.getValue(), m.deps())) {
                modules.add(mod);
                mod.files().forEach(f -> moduleOf.put(f, mod));
            }
        }
        EdgeClasses edges = classify(m.deps(), rank, moduleOf);
        if (!edges.upward().isEmpty() && p.upward() == Params.UpwardPolicy.FAIL) {
            throw new UpwardDependencyException(edges.upward());
        }
        Map<Module, List<Dep>> into = new HashMap<>();
        for (Dep d : edges.downward()) {
            into.computeIfAbsent(moduleOf.get(d.to()), k -> new ArrayList<>()).add(d);
        }
        Map<Module, Integer> internal = new HashMap<>();
        for (Dep d : edges.internal()) {
            internal.merge(moduleOf.get(d.from()), 1, Integer::sum);
        }
        List<ModuleScore> scores = new ArrayList<>();
        for (Module mod : modules) {
            scores.add(score(mod, into.getOrDefault(mod, List.of()), internal.getOrDefault(mod, 0),
                    p, n, bigN, filesPerRank, rank));
        }
        return new Result(m, p, declared, scores, edges, mScore(scores), propagationCost);
    }

    /** The modules of one layer: weakly connected components of the dependencies inside it. */
    public static List<Module> modulesOfLayer(int layerRank, List<String> files, Collection<Dep> deps) {
        List<String> sorted = new ArrayList<>(new TreeSet<>(files));
        Map<String, Integer> idx = new HashMap<>();
        for (int k = 0; k < sorted.size(); k++) {
            idx.put(sorted.get(k), k);
        }
        List<int[]> edges = new ArrayList<>();
        for (Dep d : deps) {
            Integer a = idx.get(d.from());
            Integer b = idx.get(d.to());
            if (a != null && b != null) {
                edges.add(new int[] {a, b});
            }
        }
        int[] comp = Graphs.weakComponents(sorted.size(), edges);
        Map<Integer, List<String>> groups = new TreeMap<>();
        for (int k = 0; k < sorted.size(); k++) {
            groups.computeIfAbsent(comp[k], c -> new ArrayList<>()).add(sorted.get(k));
        }
        List<Module> out = new ArrayList<>();
        groups.values().forEach(g -> out.add(new Module(layerRank, g)));
        return out;
    }

    /** Sorts each dependency into internal, downward or upward. */
    public static EdgeClasses classify(Collection<Dep> deps, Map<String, Integer> rank,
                                       Map<String, Module> moduleOf) {
        List<Dep> internal = new ArrayList<>();
        List<Dep> downward = new ArrayList<>();
        List<Dep> upward = new ArrayList<>();
        List<Dep> ordered = new ArrayList<>(deps);
        ordered.sort(Dep.ORDER);
        for (Dep d : ordered) {
            int rf = rank.get(d.from());
            int rt = rank.get(d.to());
            if (moduleOf.get(d.from()) == moduleOf.get(d.to())) {
                internal.add(d);
            } else if (rf > rt) {
                downward.add(d);
            } else if (rf < rt) {
                upward.add(d);
            } else {
                throw new IllegalStateException("two modules of one layer are linked: " + d);
            }
        }
        return new EdgeClasses(internal, downward, upward);
    }

    /**
     * One module's terms.
     *
     * @param into         downward dependencies whose target is in the module
     * @param internalDeps dependencies between the module's own files
     * @param n            the number of layers
     * @param bigN         the number of files in the system
     * @param filesPerRank file count of each layer rank (index 1..n)
     * @param rank         each file's layer rank
     */
    public static ModuleScore score(Module mod, List<Dep> into, int internalDeps, Params p,
                                    int n, int bigN, int[] filesPerRank, Map<String, Integer> rank) {
        int layer = mod.layer();
        int size = mod.size();
        int lowerFiles = 0;
        int lowerDeps = 0;
        if (layer != n) {
            if (p.lower() == Params.LowerScope.ALL) {
                for (int r = layer + 1; r <= n; r++) {
                    lowerFiles += filesPerRank[r];
                }
            } else {
                lowerFiles = filesPerRank[layer + 1];
            }
            Set<String> fromFiles = new HashSet<>();
            int pairs = 0;
            for (Dep d : into) {
                int r = rank.get(d.from());
                boolean inScope = p.lower() == Params.LowerScope.ALL || r == layer + 1;
                if (inScope) {
                    pairs++;
                    fromFiles.add(d.from());
                }
            }
            lowerDeps = p.cross() == Params.CrossCount.PAIRS ? pairs : fromFiles.size();
        }
        double clddp = (layer == n || lowerFiles == 0) ? 0.0
                : (double) lowerDeps / ((double) lowerFiles * size);
        double imcp = size >= 5 ? (double) internalDeps / ((double) size * (size - 1)) : 0.0;
        double sp = size >= 5 ? LN5 / Math.log(size) : 1.0;
        double contribution = (double) size / bigN * sp * (1 - clddp) * (1 - imcp);
        return new ModuleScore(mod, bigN, lowerDeps, lowerFiles, internalDeps, sp, clddp, imcp,
                contribution);
    }

    public static double mScore(List<ModuleScore> scores) {
        return scores.stream().mapToDouble(ModuleScore::contribution).sum();
    }
}
