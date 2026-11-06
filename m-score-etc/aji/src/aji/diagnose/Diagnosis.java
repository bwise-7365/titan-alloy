package aji.diagnose;

import aji.graph.Graphs;
import aji.model.Dep;
import aji.model.FileNode;
import aji.model.Module;
import aji.model.Params;
import aji.model.SystemModel;
import aji.score.MScore;
import aji.score.ModuleScore;
import aji.score.Result;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;

/**
 * Why one module loses points, part by part of the formula.
 *
 * <p><em>Size.</em> A large module loses most of its share of the score to the size penalty,
 * so the question to ask of it is which folders and files hold it together, and what the
 * M-score would be with a part of the module as a layer of its own. A <em>piece</em> is a
 * group of the module's files that stay linked to one another, directly or indirectly, when a
 * folder or a file is taken out of the module.
 *
 * <p><em>Cycles.</em> The files of a dependency cycle cannot be put in different layers
 * without a layering violation, so the cycles are what no layering inside the module can
 * divide.
 *
 * <p><em>Crossings.</em> The dependencies from lower layers are counted by the file depended
 * upon and by the file that depends.
 *
 * <p>Every function here is pure.
 *
 * @param module       the module and its terms in the score
 * @param folders      the folders directly below the module's directory, those that leave the
 *                     most pieces first; empty when all the files are in one folder
 * @param links        the dependencies between those folders, the most numerous first
 * @param joiningFiles the files without which the rest of the module falls into two or more
 *                     pieces, those that cut off the most files first
 * @param whatIfs      the changes tried that raise the M-score, the largest gain first
 * @param cycles       the dependency cycles inside the module, the largest first
 * @param finest       the score with the module layered as finely as its cycles allow, or
 *                     null for a module of fewer than {@link #MIN_FILES} files
 * @param crossings    the dependencies on the module from lower layers
 */
public record Diagnosis(ModuleScore module, List<Part> folders, List<Link> links,
                        List<Part> joiningFiles, List<WhatIf> whatIfs, List<Cycle> cycles,
                        Finest finest, Crossings crossings) {

    /**
     * A smaller module pays no penalty for its size or for the dependencies inside it, so
     * there is nothing to gain by dividing it. Its folders, joining files and what-ifs are
     * left empty.
     */
    public static final int MIN_FILES = 5;

    /** How many joining files are tried as layers of their own: those that cut off the most. */
    public static final int FILES_TRIED = 5;

    /** How many folders are tried as layers of their own: the largest. */
    public static final int FOLDERS_TRIED = 24;

    /** The name given to the files directly in the module's directory, as one "folder". */
    public static final String OWN_FILES = "(its own files)";

    private static final double EPS = 1e-12;

    public Diagnosis {
        folders = List.copyOf(folders);
        links = List.copyOf(links);
        joiningFiles = List.copyOf(joiningFiles);
        whatIfs = List.copyOf(whatIfs);
        cycles = List.copyOf(cycles);
    }

    /** True when the module is large enough to pay for its size. */
    public boolean sized() {
        return module.module().size() >= MIN_FILES;
    }

    /**
     * A folder or a single file of a module.
     *
     * @param name   its path below the module's directory; a folder's name ends with a slash
     * @param files  the files in it
     * @param uses   the dependencies of its files on the rest of the module
     * @param usedBy the dependencies of the rest of the module on its files
     * @param pieces the sizes, largest first, of the pieces of the rest of the module when it
     *               is taken out
     */
    public record Part(String name, List<String> files, int uses, int usedBy,
                       List<Integer> pieces) {

        public Part {
            files = List.copyOf(files);
            pieces = List.copyOf(pieces);
        }

        /** The files of the rest of the module that are outside its largest piece. */
        public int cutOff() {
            int rest = 0;
            for (int p : pieces) {
                rest += p;
            }
            return pieces.isEmpty() ? 0 : rest - pieces.get(0);
        }

        /**
         * True when it uses the rest of the module or is used by it, and not both. Only then
         * can it be a layer of its own without a layering violation.
         */
        public boolean oneWay() {
            return (uses == 0) != (usedBy == 0);
        }
    }

    /** The dependencies of the files of one folder on the files of another. */
    public record Link(String from, String to, int deps) {
    }

    /**
     * The score with one part of the module as a layer of its own, directly above or below
     * the module's layer, and everything else as it is.
     *
     * @param part    the folder or file moved
     * @param above   true when the new layer is above the module's layer (the rest of the
     *                module uses the part), false when it is below (the part uses the rest)
     * @param mScore  the M-score after the change, a fraction
     * @param gain    that M-score minus the present one
     * @param modules the number of modules that the present module's files then form
     * @param largest the number of files in the largest of those modules
     */
    public record WhatIf(Part part, boolean above, double mScore, double gain, int modules,
                         int largest) {
    }

    /**
     * A dependency cycle: two or more files of a module, each of which depends on every other,
     * directly or through the others.
     *
     * @param files   the files, by their paths below the module's directory, in order
     * @param deps    the dependencies among them
     * @param mutual  the pairs of them that depend on each other directly
     * @param example one of the shortest cycles among them: each file depends on the next, and
     *                the last on the first
     */
    public record Cycle(List<String> files, int deps, List<Mutual> mutual, List<String> example) {

        public Cycle {
            files = List.copyOf(files);
            mutual = List.copyOf(mutual);
            example = List.copyOf(example);
        }
    }

    /** Two files that depend on each other directly. */
    public record Mutual(String a, String b) {
    }

    /**
     * The score with the module's files in layers derived from the dependencies among them,
     * and everything else as it is. Each cycle is then a module, and every other file is a
     * module of its own: no layering inside the module can divide it further.
     *
     * @param layers  the number of layers the module's files then occupy
     * @param mScore  the M-score after the change, a fraction
     * @param gain    that M-score minus the present one
     * @param modules the number of modules that the present module's files then form
     * @param largest the number of files in the largest of those modules
     */
    public record Finest(int layers, double mScore, double gain, int modules, int largest) {
    }

    /**
     * The dependencies on a module from the files of lower layers that the score counts.
     *
     * @param counted whether the score counts dependency pairs or lower files
     * @param scope   whether all lower layers count or the next one only
     * @param pairs   the dependencies, as pairs of a lower file and a file of the module
     * @param targets the module's files depended upon, the most depended upon first
     * @param sources the lower files that depend, those that depend on the most files first
     */
    public record Crossings(Params.CrossCount counted, Params.LowerScope scope, int pairs,
                            List<Count> targets, List<Count> sources) {

        public Crossings {
            targets = List.copyOf(targets);
            sources = List.copyOf(sources);
        }
    }

    /** A file and a number of dependencies on it or from it. */
    public record Count(String name, int deps) {
    }

    /**
     * Explains the modules that lose the most.
     *
     * @param maxModules how many modules to explain at most
     */
    public static List<Diagnosis> of(Result r, int maxModules) {
        List<ModuleScore> worst = new ArrayList<>();
        Map<String, Integer> rank = new HashMap<>();
        for (ModuleScore ms : r.modules()) {
            if (ms.lost() > EPS) {
                worst.add(ms);
            }
            for (String f : ms.module().files()) {
                rank.put(f, ms.module().layer());
            }
        }
        worst.sort(ModuleScore.BY_LOSS);
        List<Diagnosis> out = new ArrayList<>();
        for (int k = 0; k < worst.size() && k < maxModules; k++) {
            out.add(explain(r, worst.get(k), rank));
        }
        return out;
    }

    private static Diagnosis explain(Result r, ModuleScore ms, Map<String, Integer> rank) {
        Module module = ms.module();
        List<String> files = module.files();
        int n = files.size();
        Map<String, Integer> index = new HashMap<>();
        for (int k = 0; k < n; k++) {
            index.put(files.get(k), k);
        }
        // The dependencies inside the module, and how many each file has in each direction.
        Set<Dep> inside = new LinkedHashSet<>();
        List<int[]> edges = new ArrayList<>();
        int[] uses = new int[n];
        int[] usedBy = new int[n];
        for (Dep d : r.model().deps()) {
            Integer a = index.get(d.from());
            Integer b = index.get(d.to());
            if (a != null && b != null) {
                inside.add(d);
                edges.add(new int[] {a, b});
                uses[a]++;
                usedBy[b]++;
            }
        }
        // Names are paths below the directory that the module's files share.
        List<String> names = new ArrayList<>();
        int prefix = module.directory().length();
        for (String f : files) {
            names.add(f.substring(prefix));
        }

        List<Part> joining = new ArrayList<>();
        List<Part> folders = new ArrayList<>();
        List<Link> links = new ArrayList<>();
        List<WhatIf> whatIfs = new ArrayList<>();
        Finest finest = null;
        if (n >= MIN_FILES) {
            int[][] without = Graphs.piecesWithoutEach(n, edges);
            for (int k = 0; k < n; k++) {
                if (without[k].length >= 2) {
                    joining.add(new Part(names.get(k), List.of(files.get(k)), uses[k],
                            usedBy[k], boxed(without[k])));
                }
            }
            joining.sort((x, y) -> x.cutOff() != y.cutOff()
                    ? Integer.compare(y.cutOff(), x.cutOff()) : x.name().compareTo(y.name()));
            Map<String, List<String>> groups = module.folders();
            if (groups.size() >= 2) {
                fillFolders(groups, index, edges, folders, links);
            }
            whatIfs = tryWhatIfs(r, module, folders, joining);
            finest = finestLayering(r, module, inside);
        }
        return new Diagnosis(ms, folders, links, joining, whatIfs, findCycles(names, edges),
                finest, findCrossings(r, module, rank, prefix));
    }

    /**
     * Fills in the folders of a module and the dependencies between them.
     *
     * @param groups the module's files by folder
     * @param index  each file's number in the module
     * @param edges  the dependencies inside the module, as pairs of file numbers
     */
    private static void fillFolders(Map<String, List<String>> groups,
                                    Map<String, Integer> index, List<int[]> edges,
                                    List<Part> folders, List<Link> links) {
        int n = index.size();
        List<String> names = new ArrayList<>(groups.keySet());
        int g = names.size();
        int[] groupOf = new int[n];
        for (int k = 0; k < g; k++) {
            for (String f : groups.get(names.get(k))) {
                groupOf[index.get(f)] = k;
            }
        }
        Map<Long, Integer> count = new HashMap<>();
        int[] uses = new int[g];
        int[] usedBy = new int[g];
        for (int[] e : edges) {
            int a = groupOf[e[0]];
            int b = groupOf[e[1]];
            if (a != b) {
                count.merge((long) a * g + b, 1, Integer::sum);
                uses[a]++;
                usedBy[b]++;
            }
        }
        for (Map.Entry<Long, Integer> e : count.entrySet()) {
            int a = (int) (e.getKey() / g);
            int b = (int) (e.getKey() % g);
            links.add(new Link(shown(names.get(a)), shown(names.get(b)), e.getValue()));
        }
        links.sort((x, y) -> x.deps() != y.deps() ? Integer.compare(y.deps(), x.deps())
                : !x.from().equals(y.from()) ? x.from().compareTo(y.from())
                : x.to().compareTo(y.to()));
        for (int k = 0; k < g; k++) {
            boolean[] removed = new boolean[n];
            for (int j = 0; j < n; j++) {
                removed[j] = groupOf[j] == k;
            }
            folders.add(new Part(shown(names.get(k)), groups.get(names.get(k)), uses[k],
                    usedBy[k], boxed(Graphs.pieceSizes(n, edges, removed))));
        }
        folders.sort((x, y) -> x.pieces().size() != y.pieces().size()
                ? Integer.compare(y.pieces().size(), x.pieces().size())
                : x.files().size() != y.files().size()
                ? Integer.compare(y.files().size(), x.files().size())
                : x.name().compareTo(y.name()));
    }

    /**
     * Tries folders and joining files as layers of their own: the largest folders first, then
     * the joining files that are not a folder alone. Keeps the changes that raise the score.
     */
    private static List<WhatIf> tryWhatIfs(Result r, Module module, List<Part> folders,
                                           List<Part> joining) {
        List<Part> largest = new ArrayList<>(folders);
        largest.sort((x, y) -> Integer.compare(y.files().size(), x.files().size()));
        List<WhatIf> whatIfs = new ArrayList<>();
        Set<String> alone = new HashSet<>();
        for (Part p : largest.subList(0, Math.min(FOLDERS_TRIED, largest.size()))) {
            if (p.files().size() == 1) {
                alone.add(p.files().get(0));
            }
            if (p.oneWay()) {
                whatIfs.add(whatIf(r, module, p));
            }
        }
        if (folders.size() == 2 && whatIfs.size() == 2) {
            // Two folders are the two halves of one division: keep the better placement.
            whatIfs.remove(whatIfs.get(0).gain() >= whatIfs.get(1).gain() ? 1 : 0);
        }
        for (Part p : joining.subList(0, Math.min(FILES_TRIED, joining.size()))) {
            if (p.oneWay() && !alone.contains(p.files().get(0))) {
                whatIfs.add(whatIf(r, module, p));
            }
        }
        whatIfs.removeIf(w -> w.gain() <= EPS);
        whatIfs.sort((x, y) -> x.gain() != y.gain()
                ? Double.compare(y.gain(), x.gain()) : x.part().name().compareTo(y.part().name()));
        return whatIfs;
    }

    /**
     * Scores the system with one part of a module as a layer of its own. The layer numbers
     * are doubled to make room for the new layer next to the module's layer.
     */
    private static WhatIf whatIf(Result r, Module module, Part part) {
        boolean above = part.uses() == 0;
        int layer = r.declaredLayers().get(module.layer() - 1);
        Set<String> moved = new HashSet<>(part.files());
        Map<String, Integer> layers = new HashMap<>();
        for (FileNode f : r.model().files()) {
            layers.put(f.id(), moved.contains(f.id()) ? 2 * layer + (above ? -1 : 1)
                    : 2 * f.layer());
        }
        Result w = rescored(r, layers);
        int[] formed = formed(w, module);
        return new WhatIf(part, above, w.mScore(), w.mScore() - r.mScore(), formed[0], formed[1]);
    }

    /**
     * Scores the system with a module's files in layers derived from the dependencies among
     * them. The other layer numbers are multiplied to make room for those layers.
     */
    private static Finest finestLayering(Result r, Module module, Set<Dep> inside) {
        List<FileNode> nodes = new ArrayList<>();
        for (String f : module.files()) {
            nodes.add(new FileNode(f, FileNode.UNASSIGNED));
        }
        Map<String, Integer> own = Graphs.derivedLayers(new SystemModel("module", nodes, inside));
        int depth = 1;
        for (int l : own.values()) {
            depth = Math.max(depth, l);
        }
        int layer = r.declaredLayers().get(module.layer() - 1);
        Map<String, Integer> layers = new HashMap<>();
        for (FileNode f : r.model().files()) {
            Integer within = own.get(f.id());
            layers.put(f.id(), within == null ? f.layer() * depth : layer * depth + within - 1);
        }
        Result w = rescored(r, layers);
        int[] formed = formed(w, module);
        return new Finest(depth, w.mScore(), w.mScore() - r.mScore(), formed[0], formed[1]);
    }

    /** The same files and dependencies scored with other layers. */
    private static Result rescored(Result r, Map<String, Integer> layers) {
        return MScore.evaluate(r.model().withLayers(layers, Map.of()),
                r.params().with(Params.UpwardPolicy.EXCLUDE), r.propagationCost());
    }

    /** How many modules a module's files form in another result, and the largest one's size. */
    private static int[] formed(Result w, Module module) {
        Set<String> inModule = new HashSet<>(module.files());
        int modules = 0;
        int largest = 0;
        for (ModuleScore ms : w.modules()) {
            if (inModule.contains(ms.module().files().get(0))) {
                modules++;
                largest = Math.max(largest, ms.module().size());
            }
        }
        return new int[] {modules, largest};
    }

    /**
     * The dependency cycles of a module: its strongly connected components of two or more
     * files.
     *
     * @param names the module's files, in order
     * @param edges the dependencies inside the module, as pairs of file numbers
     */
    private static List<Cycle> findCycles(List<String> names, List<int[]> edges) {
        int n = names.size();
        int[] degree = new int[n];
        for (int[] e : edges) {
            degree[e[0]]++;
        }
        int[][] out = new int[n][];
        for (int k = 0; k < n; k++) {
            out[k] = new int[degree[k]];
        }
        int[] used = new int[n];
        Set<Long> have = new HashSet<>();
        for (int[] e : edges) {
            out[e[0]][used[e[0]]++] = e[1];
            have.add((long) e[0] * n + e[1]);
        }
        for (int[] row : out) {
            Arrays.sort(row);   // so that the example does not depend on the order of the edges
        }
        int[] comp = Graphs.strongComponents(out);
        Map<Integer, List<Integer>> members = new TreeMap<>();
        for (int k = 0; k < n; k++) {
            members.computeIfAbsent(comp[k], c -> new ArrayList<>()).add(k);
        }
        Map<Integer, Integer> deps = new HashMap<>();
        Map<Integer, List<Mutual>> mutual = new HashMap<>();
        for (int[] e : edges) {
            if (comp[e[0]] == comp[e[1]]) {
                deps.merge(comp[e[0]], 1, Integer::sum);
                if (e[0] < e[1] && have.contains((long) e[1] * n + e[0])) {
                    mutual.computeIfAbsent(comp[e[0]], c -> new ArrayList<>())
                            .add(new Mutual(names.get(e[0]), names.get(e[1])));
                }
            }
        }
        List<Cycle> cycles = new ArrayList<>();
        for (Map.Entry<Integer, List<Integer>> e : members.entrySet()) {
            if (e.getValue().size() < 2) {
                continue;
            }
            int[] nodes = e.getValue().stream().mapToInt(Integer::intValue).toArray();
            List<Mutual> pairs = mutual.getOrDefault(e.getKey(), new ArrayList<>());
            pairs.sort((x, y) -> !x.a().equals(y.a()) ? x.a().compareTo(y.a())
                    : x.b().compareTo(y.b()));
            cycles.add(new Cycle(named(names, nodes), deps.get(e.getKey()), pairs,
                    named(names, Graphs.shortestCycle(out, nodes))));
        }
        cycles.sort((x, y) -> x.files().size() != y.files().size()
                ? Integer.compare(y.files().size(), x.files().size())
                : x.files().get(0).compareTo(y.files().get(0)));
        return cycles;
    }

    /**
     * The dependencies on a module from lower layers that the score counts, by file.
     *
     * @param rank   each file's layer rank
     * @param prefix the length of the module's directory, which is left off its files' names
     */
    private static Crossings findCrossings(Result r, Module module, Map<String, Integer> rank,
                                           int prefix) {
        Set<String> inModule = new HashSet<>(module.files());
        boolean all = r.params().lower() == Params.LowerScope.ALL;
        Map<String, Integer> targets = new HashMap<>();
        Map<String, Integer> sources = new HashMap<>();
        int pairs = 0;
        for (Dep d : r.edges().downward()) {
            if (inModule.contains(d.to()) && (all || rank.get(d.from()) == module.layer() + 1)) {
                pairs++;
                targets.merge(d.to().substring(prefix), 1, Integer::sum);
                sources.merge(d.from(), 1, Integer::sum);
            }
        }
        return new Crossings(r.params().cross(), r.params().lower(), pairs, counts(targets),
                counts(sources));
    }

    /** The entries of a map as counts, the largest first; equal counts in the order of names. */
    private static List<Count> counts(Map<String, Integer> byName) {
        List<Count> out = new ArrayList<>();
        byName.forEach((name, deps) -> out.add(new Count(name, deps)));
        out.sort((x, y) -> x.deps() != y.deps() ? Integer.compare(y.deps(), x.deps())
                : x.name().compareTo(y.name()));
        return out;
    }

    private static List<String> named(List<String> names, int[] nodes) {
        List<String> out = new ArrayList<>();
        for (int k : nodes) {
            out.add(names.get(k));
        }
        return out;
    }

    private static String shown(String folder) {
        return folder.isEmpty() ? OWN_FILES : folder;
    }

    private static List<Integer> boxed(int[] a) {
        return Arrays.stream(a).boxed().toList();
    }
}
