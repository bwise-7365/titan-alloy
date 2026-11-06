package aji.selftest;

import aji.deps.CppDeps;
import aji.deps.JavaDeps;
import aji.deps.SourceUnit;
import aji.diagnose.Diagnosis;
import aji.graph.Graphs;
import aji.io.XmlFiles;
import aji.lex.Lexer;
import aji.lex.Token;
import aji.model.Dep;
import aji.model.FileNode;
import aji.model.Params;
import aji.model.SystemModel;
import aji.score.MScore;
import aji.score.Result;
import aji.testing.Checks;

import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;

/**
 * Checks aji against the worked examples of the M-score paper and against small hand-made
 * inputs. Needs no test library. Run it from the aji folder, or pass the fixtures folder:
 *
 * <pre>  java -cp out aji.selftest.SelfTest [fixtures]</pre>
 *
 * Published values are rounded to two or three decimals, so a check passes when the computed
 * value is within 0.005 percentage points of the published one.
 */
public final class SelfTest {

    private static final double TOLERANCE = 0.005;
    private static final Checks CHECKS = new Checks();

    private SelfTest() {
    }

    public static void main(String[] args) throws Exception {
        Path fixtures = Path.of(args.length > 0 ? args[0] : "fixtures");
        if (!Files.isDirectory(fixtures)) {
            System.err.println("fixtures folder not found: " + fixtures.toAbsolutePath()
                    + " (run from the aji folder or pass the path)");
            System.exit(1);
        }
        Params all = Params.defaults();
        Params adjacent = all.with(Params.LowerScope.ADJACENT);

        System.out.println("Worked examples of the paper (figures reconstructed from the text)");
        SystemModel f2a = XmlFiles.readSystem(fixtures.resolve("fig2a.xml"));
        SystemModel f2b = XmlFiles.readSystem(fixtures.resolve("fig2b.xml"));
        SystemModel f4a = XmlFiles.readSystem(fixtures.resolve("fig4a-n10.xml"));
        SystemModel f4b = XmlFiles.readSystem(fixtures.resolve("fig4b-n10.xml"));
        same("fixture fig2a.xml equals the generated Fig. 2A, N = 6", f2a, fig2(6, false));
        same("fixture fig2b.xml equals the generated Fig. 2B, N = 7", f2b, fig2(6, true));
        same("fixture fig4a-n10.xml equals the generated Fig. 4A, N = 10", f4a, fig4(10, true));
        same("fixture fig4b-n10.xml equals the generated Fig. 4B, N = 10", f4b, fig4(10, false));

        published("Fig. 2A, N = 6", score(f2a, all), 66.67);
        published("Fig. 2B, N = 7, adjacent lower layer", score(f2b, adjacent), 71.43);
        known("Fig. 2B, N = 7, all lower layers (expected to differ: ambiguity 3)",
                score(f2b, all), 71.43, 73.809524);
        published("Fig. 2A, N = 10,000", score(fig2(10_000, false), all), 17.47);
        published("Fig. 2B, N = 10,001, adjacent lower layer", score(fig2(10_000, true), adjacent), 17.48);
        published("Fig. 4A, N = 10", score(f4a, all), 36.62);
        published("Fig. 4B, N = 10", score(f4b, all), 51.27);
        published("Fig. 4A, N = 10,000", score(fig4(10_000, true), all), 17.466);
        published("Fig. 4B, N = 10,000", score(fig4(10_000, false), all), 17.469);

        System.out.println();
        System.out.println("Fig. 3 (N = 8, two modules of 4 files; published 75.00 % and 87.50 %)");
        for (int pairs : new int[] {8, 6, 4, 2}) {
            SystemModel m = fig3(pairs);
            System.out.printf(Locale.ROOT, "  INFO %d cross pairs: PAIRS %.3f %%, FILES %.3f %%%n",
                    pairs, score(m, all), score(m, all.with(Params.CrossCount.FILES)));
        }
        System.out.println("  INFO Only the PAIRS reading can reach 75 % (8 of 16 pairs); FILES"
                + " cannot go below 87.5 %. The 6 and 2 dependencies named in the text give neither.");

        System.out.println();
        System.out.println("Dependency extraction, layering and propagation cost");
        lexerIgnoresCommentsAndLiterals();
        javaDependencies();
        cppDependencies();
        derivedLayering();
        propagationCost();
        upwardDependencies();

        System.out.println();
        System.out.println("Why a module loses points");
        joiningNodes();
        whatJoinsAModule();
        cyclesAndCrossings();

        System.exit(CHECKS.finish());
    }

    // ---- paper figures -------------------------------------------------------------------

    /**
     * Fig. 2: file f1 on top; files f2..fn below it in a chain (n - 2 internal dependencies),
     * each depending on f1. With {@code isolated}, file i1 forms a third layer of its own.
     */
    static SystemModel fig2(int n, boolean isolated) {
        List<FileNode> files = new ArrayList<>();
        Set<Dep> deps = new HashSet<>();
        files.add(new FileNode("f1", 1));
        for (int k = 2; k <= n; k++) {
            files.add(new FileNode("f" + k, 2));
            deps.add(new Dep("f" + k, "f1"));
            if (k < n) {
                deps.add(new Dep("f" + k, "f" + (k + 1)));
            }
        }
        if (isolated) {
            files.add(new FileNode("i1", 3));
        }
        return new SystemModel("fig2", files, deps);
    }

    /**
     * Fig. 4: file f1 on top; files f2..fn below it, each depending on f1. File f2 is a hub
     * linked to every other lower file, and f3..fn form a ring. In A both links are two-way
     * (4(n - 2) internal dependencies); in B they are one-way (2(n - 2)).
     */
    static SystemModel fig4(int n, boolean twoWay) {
        List<FileNode> files = new ArrayList<>();
        Set<Dep> deps = new HashSet<>();
        files.add(new FileNode("f1", 1));
        for (int k = 2; k <= n; k++) {
            files.add(new FileNode("f" + k, 2));
            deps.add(new Dep("f" + k, "f1"));
        }
        for (int k = 3; k <= n; k++) {
            int next = k == n ? 3 : k + 1;
            deps.add(new Dep("f2", "f" + k));
            deps.add(new Dep("f" + k, "f" + next));
            if (twoWay) {
                deps.add(new Dep("f" + k, "f2"));
                deps.add(new Dep("f" + next, "f" + k));
            }
        }
        return new SystemModel("fig4", files, deps);
    }

    /** Fig. 3 as far as the text describes it: odd files on top, even files below. */
    static SystemModel fig3(int crossPairs) {
        List<FileNode> files = new ArrayList<>();
        Set<Dep> deps = new HashSet<>();
        for (int k = 1; k <= 8; k++) {
            files.add(new FileNode("f" + k, k % 2 == 1 ? 1 : 2));
        }
        for (int k = 1; k <= 5; k += 2) {
            deps.add(new Dep("f" + k, "f" + (k + 2)));
            deps.add(new Dep("f" + (k + 1), "f" + (k + 3)));
        }
        int made = 0;
        for (int low = 2; low <= 8 && made < crossPairs; low += 2) {
            for (int up = 1; up <= 7 && made < crossPairs; up += 2) {
                deps.add(new Dep("f" + low, "f" + up));
                made++;
            }
        }
        return new SystemModel("fig3", files, deps);
    }

    // ---- unit checks ---------------------------------------------------------------------

    static void lexerIgnoresCommentsAndLiterals() {
        String src = "/* Foo */ String s = \"Bar\"; // Baz\nchar c = 'x'; String t = \"\"\"\n Qux\n\"\"\"; Real r;";
        List<String> ids = Lexer.tokens(src, Lexer.Dialect.JAVA).stream()
                .filter(Token::isIdent).map(Token::text).toList();
        check("lexer drops comments, strings and text blocks",
                ids.equals(List.of("String", "s", "char", "c", "String", "t", "Real", "r")),
                ids.toString());
    }

    static void javaDependencies() {
        List<SourceUnit> units = List.of(
                new SourceUnit("a/A.java", "package p; public class A { B b; q.C c; }"),
                new SourceUnit("a/B.java", "package p; class B { static class Inner {} }"),
                new SourceUnit("c/C.java", "package q; public class C { }"),
                new SourceUnit("d/D.java",
                        "package r; import p.B.Inner; import q.*; class D { C c; Inner i; }"),
                new SourceUnit("e/E.java",
                        "package r; class E { String s = \"A B C\"; /* C */ Object o = x.C; }"));
        Set<Dep> found = JavaDeps.find(units).deps();
        Set<Dep> expected = Set.of(new Dep("a/A.java", "a/B.java"), new Dep("a/A.java", "c/C.java"),
                new Dep("d/D.java", "a/B.java"), new Dep("d/D.java", "c/C.java"));
        check("Java: same package, qualified name, nested import, on-demand import",
                found.equals(expected), found.toString());
    }

    static void cppDependencies() {
        List<SourceUnit> units = List.of(
                new SourceUnit("inc/lib/x.hpp", "#pragma once\n#include <vector>\n#include \"y.hpp\"\n"),
                new SourceUnit("inc/lib/y.hpp", "// #include \"x.hpp\"\n"),
                new SourceUnit("src/x.cpp", "#include <lib/x.hpp>\n/*\n#include \"zz.h\"\n*/\n"
                        + "const char* s = \"#include \\\"y.hpp\\\"\";\n"),
                new SourceUnit("app/y.hpp", ""),
                new SourceUnit("app/main.cpp", "  #  include \"y.hpp\"\n#include \"lib/y.hpp\"\n"));
        Set<Dep> found = CppDeps.find(units, List.of()).deps();
        Set<Dep> expected = Set.of(new Dep("inc/lib/x.hpp", "inc/lib/y.hpp"),
                new Dep("src/x.cpp", "inc/lib/x.hpp"), new Dep("app/main.cpp", "app/y.hpp"),
                new Dep("app/main.cpp", "inc/lib/y.hpp"));
        check("C++: relative, path-suffix and nearest-directory include resolution",
                found.equals(expected), found.toString());
    }

    static void derivedLayering() {
        List<FileNode> files = new ArrayList<>();
        for (String id : List.of("a", "b", "c", "d", "e")) {
            files.add(new FileNode(id, 0));
        }
        Set<Dep> deps = Set.of(new Dep("a", "b"), new Dep("b", "c"), new Dep("d", "a"),
                new Dep("d", "e"), new Dep("e", "d"));
        SystemModel m = new SystemModel("derived", files, deps);
        Map<String, Integer> layers = Graphs.derivedLayers(m);
        check("derived layers: top = most depended upon, a cycle shares a layer",
                layers.equals(Map.of("c", 1, "b", 2, "a", 3, "d", 4, "e", 4)), layers.toString());
        Result r = MScore.evaluate(m.withLayers(layers, Map.of()), Params.defaults());
        check("derived layers leave no upward dependencies", r.edges().upward().isEmpty(), "");
    }

    static void propagationCost() {
        SystemModel m = new SystemModel("chain", List.of(new FileNode("a", 1), new FileNode("b", 1),
                new FileNode("c", 1)), Set.of(new Dep("a", "b"), new Dep("b", "c")));
        double pc = Graphs.propagationCost(m);
        check("propagation cost of a three-file chain is 6/9", Math.abs(pc - 6.0 / 9) < 1e-12,
                String.valueOf(pc));
    }

    static void upwardDependencies() {
        SystemModel m = new SystemModel("up", List.of(new FileNode("top", 1), new FileNode("low", 2)),
                Set.of(new Dep("top", "low")));
        Result r = MScore.evaluate(m, Params.defaults());
        check("an upward dependency is listed and left out of the score",
                r.edges().upward().size() == 1 && Math.abs(r.mScore() - 1.0) < 1e-12,
                String.valueOf(r.mScore()));
        boolean threw = false;
        try {
            MScore.evaluate(m, Params.defaults().with(Params.UpwardPolicy.FAIL));
        } catch (MScore.UpwardDependencyException ex) {
            threw = true;
        }
        check("--upward FAIL refuses a system with an upward dependency", threw, "");
    }

    static void joiningNodes() {
        // A triangle 0-1-2 and a tail 2-3; node 4 and the pair 5-6 hang from node 3. One edge
        // is given in both directions.
        List<int[]> edges = List.of(new int[] {0, 1}, new int[] {1, 0}, new int[] {2, 1},
                new int[] {0, 2}, new int[] {3, 2}, new int[] {3, 4}, new int[] {5, 3},
                new int[] {6, 5});
        int[][] expected = {{6}, {6}, {4, 2}, {3, 2, 1}, {6}, {5, 1}, {6}};
        int[][] found = Graphs.piecesWithoutEach(7, edges);
        check("pieces left without each node of a triangle with a tail and two branches",
                Arrays.deepEquals(found, expected), Arrays.deepToString(found));
        boolean same = true;
        for (int k = 0; k < 7; k++) {
            boolean[] removed = new boolean[7];
            removed[k] = true;
            same &= Arrays.equals(found[k], Graphs.pieceSizes(7, edges, removed));
        }
        check("the search for joining nodes agrees with taking each node out in turn", same, "");
    }

    static void whatJoinsAModule() {
        // One layer of five files: a library used by two pairs of files that are otherwise
        // apart. The five files form one module, which scores 80 %.
        List<FileNode> files = new ArrayList<>();
        for (String id : List.of("a/x1", "a/x2", "b/y1", "b/y2", "s/lib")) {
            files.add(new FileNode(id, 1));
        }
        Set<Dep> deps = Set.of(new Dep("a/x1", "a/x2"), new Dep("b/y1", "b/y2"),
                new Dep("a/x1", "s/lib"), new Dep("b/y1", "s/lib"));
        Result r = MScore.evaluate(new SystemModel("joined", files, deps), Params.defaults());
        List<Diagnosis> explained = Diagnosis.of(r, 3);
        if (explained.size() != 1) {
            check("one module of five files is explained", false, explained.size() + " explained");
            return;
        }
        Diagnosis d = explained.get(0);
        List<String> joining = d.joiningFiles().stream().map(Diagnosis.Part::name).toList();
        List<String> folders = d.folders().stream().map(Diagnosis.Part::name).toList();
        check("joining files and folders of a library used by two pairs of files",
                joining.equals(List.of("s/lib", "a/x1", "b/y1"))
                        && folders.equals(List.of("s/", "a/", "b/"))
                        && d.folders().get(0).pieces().equals(List.of(2, 2)),
                joining + " " + folders);

        Diagnosis.WhatIf lib = d.whatIfs().stream()
                .filter(w -> w.part().name().equals("s/")).findFirst().orElse(null);
        check("with the library as a layer of its own above the rest, 80 % becomes 90 %",
                Math.abs(r.mScore() - 0.8) < 1e-12 && lib != null && lib.above()
                        && Math.abs(lib.mScore() - 0.9) < 1e-12 && lib.modules() == 3
                        && lib.largest() == 2,
                lib == null ? "no such change is offered" : String.valueOf(lib.mScore()));

        // Each pair can go below the rest for the same gain. A single file of a pair cannot:
        // as a layer of its own it would lower the score.
        List<String> offered = d.whatIfs().stream().map(w -> w.part().name()).sorted().toList();
        check("only the changes that raise the score are offered",
                offered.equals(List.of("a/", "b/", "s/")), offered.toString());
    }

    static void cyclesAndCrossings() {
        // An upper layer of five files: a ring a -> b -> c -> a, a pair d and e that depend on
        // each other, and c -> d between them. Two lower files depend on a and b.
        List<FileNode> files = new ArrayList<>();
        for (String id : List.of("a", "b", "c", "d", "e")) {
            files.add(new FileNode(id, 1));
        }
        files.add(new FileNode("u", 2));
        files.add(new FileNode("v", 2));
        Set<Dep> deps = Set.of(new Dep("a", "b"), new Dep("b", "c"), new Dep("c", "a"),
                new Dep("c", "d"), new Dep("d", "e"), new Dep("e", "d"), new Dep("u", "a"),
                new Dep("u", "b"), new Dep("v", "a"));
        Result r = MScore.evaluate(new SystemModel("cycles", files, deps), Params.defaults());
        List<Diagnosis> explained = Diagnosis.of(r, 3);
        if (explained.size() != 1 || explained.get(0).cycles().size() != 2) {
            check("one module with two cycles is explained", false, explained.size() + " explained");
            return;
        }
        Diagnosis d = explained.get(0);
        Diagnosis.Cycle ring = d.cycles().get(0);
        Diagnosis.Cycle pair = d.cycles().get(1);
        check("cycles: a ring of three files, and two files that depend on each other",
                ring.files().equals(List.of("a", "b", "c")) && ring.deps() == 3
                        && ring.mutual().isEmpty() && ring.example().equals(List.of("a", "b", "c"))
                        && pair.files().equals(List.of("d", "e")) && pair.deps() == 2
                        && pair.mutual().equals(List.of(new Diagnosis.Mutual("d", "e"))),
                ring + " " + pair);

        // With the pair as a layer above the ring, the score is 18/70 + 15/70 + 20/70.
        Diagnosis.Finest finest = d.finest();
        check("layered as finely as its cycles allow, the module is two modules and scores 53/70",
                finest != null && finest.layers() == 2 && finest.modules() == 2
                        && finest.largest() == 3 && Math.abs(finest.mScore() - 53.0 / 70) < 1e-12,
                String.valueOf(finest));

        Diagnosis.Crossings x = d.crossings();
        check("dependencies from the lower layer, by the file depended upon and by the file"
                        + " that depends",
                x.pairs() == 3
                        && x.targets().equals(List.of(new Diagnosis.Count("a", 2),
                                new Diagnosis.Count("b", 1)))
                        && x.sources().equals(List.of(new Diagnosis.Count("u", 2),
                                new Diagnosis.Count("v", 1))),
                x.toString());
    }

    // ---- helpers -------------------------------------------------------------------------

    private static double score(SystemModel m, Params p) {
        return 100 * MScore.evaluate(m, p).mScore();
    }

    private static void published(String name, double got, double published) {
        double d = Math.abs(got - published);
        check(String.format(Locale.ROOT, "%s: computed %.6f %% vs published %s %%", name, got,
                published), d < TOLERANCE, String.format(Locale.ROOT, "discrepancy %.6f", d));
    }

    private static void known(String name, double got, double published, double expected) {
        boolean asExpected = Math.abs(got - expected) < 1e-5;
        check(String.format(Locale.ROOT, "%s: computed %.6f %% vs published %s %%", name, got,
                published), asExpected, "expected the reconstruction to give " + expected);
    }

    private static void same(String name, SystemModel a, SystemModel b) {
        boolean ok = new HashSet<>(a.files()).equals(new HashSet<>(b.files()))
                && a.deps().equals(b.deps());
        check(name, ok, "");
    }

    private static void check(String name, boolean ok, String detail) {
        CHECKS.check(name, ok, detail);
    }
}
