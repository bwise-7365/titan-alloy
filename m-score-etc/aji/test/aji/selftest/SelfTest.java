package aji.selftest;

import aji.deps.CppDeps;
import aji.deps.JavaDeps;
import aji.deps.SourceUnit;
import aji.diagnose.Diagnosis;
import aji.graph.Graphs;
import aji.io.XmlFiles;
import aji.lex.Lexer;
import aji.lex.Token;
import aji.measure.Direction;
import aji.measure.Fit;
import aji.measure.Grouping;
import aji.measure.Move;
import aji.measure.Reach;
import aji.measure.TestReach;
import aji.model.Dep;
import aji.model.FileNode;
import aji.model.Params;
import aji.model.SystemModel;
import aji.moves.Trials;
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

        System.out.println();
        System.out.println("The four measures beside the M-score, worked by hand");
        reach();
        direction();
        fit();
        testReach();
        grouping();
        trials();

        System.exit(CHECKS.finish());
    }

    // ---- the four measures ------------------------------------------------------------------

    /** Files a, b, ... with one layer each (not needed by the measure under test). */
    private static SystemModel model(String name, String files, String... deps) {
        List<FileNode> nodes = new ArrayList<>();
        for (String f : files.split(" ")) {
            nodes.add(new FileNode(f, 1));
        }
        Set<Dep> set = new HashSet<>();
        for (String d : deps) {
            String[] ends = d.split(">");
            set.add(new Dep(ends[0], ends[1]));
        }
        return new SystemModel(name, nodes, set);
    }

    static void reach() {
        Reach chain = Reach.of(model("chain", "a b c d", "a>b", "b>c", "c>d"));
        check("reach: a chain of 4 has CCD 10 and NCCD 10 / (5 log2 5 - 4)",
                chain.ccd() == 10 && Math.abs(chain.nccd() - 10 / Reach.treeCcd(4)) < 1e-12
                        && Math.abs(Reach.treeCcd(4) - (5 * Math.log(5) / Math.log(2) - 4)) < 1e-12,
                "CCD " + chain.ccd());
        check("reach: in the chain, cutting b -> c lowers CCD the most, to 6",
                !chain.cuts().isEmpty() && chain.cuts().get(0).dep().equals(new Dep("b", "c"))
                        && chain.cuts().get(0).ccdAfter() == 6
                        && chain.cuts().size() == 3,
                chain.cuts().toString());
        Reach star = Reach.of(model("star", "r a b c", "r>a", "r>b", "r>c"));
        check("reach: a star of 1 + 3 has CCD 7", star.ccd() == 7, "CCD " + star.ccd());
        Reach tree = Reach.of(model("tree", "r m1 m2 l1 l2 l3 l4", "r>m1", "r>m2", "m1>l1",
                "m1>l2", "m2>l3", "m2>l4"));
        check("reach: a balanced binary tree of 7 has CCD 17 and NCCD 1",
                tree.ccd() == 17 && Math.abs(tree.nccd() - 1) < 1e-12, "CCD " + tree.ccd());
        Reach cycle = Reach.of(model("cycle", "a b c", "a>b", "b>c", "c>a"));
        check("reach: a cycle of 3 has CCD 9, and any one cut leaves a chain with CCD 6",
                cycle.ccd() == 9 && cycle.cuts().size() == 3
                        && cycle.cuts().stream().allMatch(x -> x.ccdAfter() == 6),
                cycle.cuts().toString());
        // With the chord a -> c, cutting a -> c changes nothing (a still reaches c through b);
        // cutting c -> a leaves a chain with the chord (6); cutting a -> b or b -> c leaves a
        // two-file cycle with a third file beside it (7). The guard against trying a cut that
        // another dependency covers must still try a -> b, since c reaches b only through a.
        Reach chord = Reach.of(model("chord", "a b c", "a>b", "b>c", "c>a", "a>c"));
        boolean found = chord.cuts().stream()
                .anyMatch(x -> x.dep().equals(new Dep("a", "b")) && x.ccdAfter() == 7);
        check("reach: with a chord a -> c across the cycle, three cuts help and a -> c does not",
                chord.ccd() == 9 && found && chord.cuts().size() == 3
                        && chord.cuts().get(0).dep().equals(new Dep("c", "a"))
                        && chord.cuts().get(0).ccdAfter() == 6,
                chord.cuts().toString());
        check("reach: propagation cost is CCD / N squared",
                Math.abs(chain.propagationCost() - 10.0 / 16) < 1e-12, "");
    }

    static void direction() {
        SystemModel m = model("line", "A/a1 A/a2 B/b1 B/b2 C/c1", "A/a1>B/b1", "A/a2>B/b1",
                "B/b1>C/c1", "B/b2>C/c1", "C/c1>A/a1");
        Direction d = Direction.of(m, Grouping.FOLDER.of(m));
        Map<String, Direction.Group> g = new java.util.HashMap<>();
        d.groups().forEach(x -> g.put(x.name(), x));
        check("direction: Ca, Ce and I of three groups in a line with one back-edge",
                g.get("A").ca() == 1 && g.get("A").ce() == 2
                        && g.get("B").ca() == 2 && g.get("B").ce() == 2
                        && g.get("C").ca() == 2 && g.get("C").ce() == 1
                        && Math.abs(g.get("A").instability() - 2.0 / 3) < 1e-12
                        && Math.abs(g.get("B").instability() - 0.5) < 1e-12
                        && Math.abs(g.get("C").instability() - 1.0 / 3) < 1e-12,
                d.groups().toString());
        check("direction: 4 of 5 dependencies between groups obey the principle; C -> A breaks it",
                d.between() == 5 && d.obeying() == 4 && Math.abs(d.grade() - 0.8) < 1e-12
                        && d.violations().size() == 1 && d.violations().get(0).from().equals("C")
                        && d.violations().get(0).to().equals("A") && d.violations().get(0).deps() == 1
                        && g.get("C").breaking() == 1,
                d.violations().toString());
        check("direction: no hub among files with fan-in and fan-out below 3", d.hubs().isEmpty(),
                d.hubs().toString());
        check("direction: the 90th percentile of 1..10 is 9",
                Direction.percentile90(new int[] {3, 1, 4, 1, 5, 9, 2, 6, 10, 7}) == 9, "");
    }

    static void fit() {
        SystemModel m = model("triangles", "X/x1 X/x2 X/x3 Y/y1 Y/y2 Y/y3", "X/x1>X/x2",
                "X/x2>X/x3", "X/x1>X/x3", "Y/y1>Y/y2", "Y/y2>Y/y3", "Y/y1>Y/y3", "X/x3>Y/y1");
        Fit f = Fit.of(m, Grouping.FOLDER.of(m));
        check("fit: two triangles joined by one edge have Q = 5/14",
                Math.abs(f.q() - 5.0 / 14) < 1e-12 && f.edges() == 7 && f.inside() == 6,
                String.valueOf(f.q()));
        check("fit: no single file has more neighbours in the other triangle", f.shifts().isEmpty(),
                f.shifts().toString());
        Map<String, Fit.Profile> p = new java.util.HashMap<>();
        f.profiles().forEach(x -> p.put(x.group(), x));
        check("fit: profiles: X has 2 hidden and 1 outbound, Y 2 hidden and 1 inbound",
                p.get("X").hidden() == 2 && p.get("X").outbound() == 1 && p.get("X").transit() == 0
                        && p.get("Y").hidden() == 2 && p.get("Y").inbound() == 1
                        && Math.abs(f.hiddenShare() - 4.0 / 6) < 1e-12 && f.transits().isEmpty(),
                f.profiles().toString());
        SystemModel one = model("one", "X/x1 X/x2 X/x3", "X/x1>X/x2", "X/x2>X/x3");
        check("fit: one group has Q = 0", Math.abs(Fit.of(one, Grouping.FOLDER.of(one)).q()) < 1e-12,
                "");
        SystemModel cliques = model("cliques", "X/x1 X/x2 X/x3 Y/y1 Y/y2 Y/y3", "X/x1>X/x2",
                "X/x2>X/x3", "X/x1>X/x3", "Y/y1>Y/y2", "Y/y2>Y/y3", "Y/y1>Y/y3");
        check("fit: two separate equal cliques have Q = 0.5",
                Math.abs(Fit.of(cliques, Grouping.FOLDER.of(cliques)).q() - 0.5) < 1e-12, "");
        // x1 has one neighbour in X and three in Y. Before the move Q = 10/16 - (49 + 81)/256
        // = 0.1172; with x1 in Y, Q = 14/16 - (9 + 169)/256 = 0.1797.
        SystemModel stray = model("stray", "X/x1 X/x2 X/x3 Y/y1 Y/y2 Y/y3", "X/x1>X/x2",
                "X/x2>X/x3", "Y/y1>Y/y2", "Y/y2>Y/y3", "Y/y1>Y/y3", "Y/y1>X/x1",
                "Y/y2>X/x1", "Y/y3>X/x1");
        Fit s = Fit.of(stray, Grouping.FOLDER.of(stray));
        check("fit: a file with one neighbour at home and three in the other group is offered"
                        + " a move there (Q 0.1172 to 0.1797)",
                !s.shifts().isEmpty() && s.shifts().get(0).file().equals("X/x1")
                        && s.shifts().get(0).to().equals("Y") && s.shifts().get(0).there() == 3
                        && s.shifts().get(0).here() == 1
                        && Math.abs(s.q() - (10.0 / 16 - 130.0 / 256)) < 1e-12
                        && Math.abs(s.shifts().get(0).qAfter() - (14.0 / 16 - 178.0 / 256)) < 1e-12,
                s.shifts().toString() + " Q " + s.q());
        // The same six files with x1 tied to both triangles equally: the degree correction
        // keeps x1 at home, since Y is already the larger group.
        SystemModel tied = model("tied", "X/x1 X/x2 X/x3 Y/y1 Y/y2 Y/y3", "X/x1>X/x2",
                "X/x2>X/x3", "X/x1>X/x3", "Y/y1>Y/y2", "Y/y2>Y/y3", "Y/y1>Y/y3", "Y/y1>X/x1",
                "Y/y2>X/x1", "Y/y3>X/x1");
        check("fit: a file with two neighbours at home and three away stays, as Q would fall",
                Fit.of(tied, Grouping.FOLDER.of(tied)).shifts().isEmpty(), "");
    }

    static void testReach() throws Exception {
        SystemModel m = model("tests", "a b c d", "a>b", "b>c", "c>d")
                .withTests(List.of("t"), Set.of(new Dep("t", "b")));
        TestReach r = TestReach.of(m, false);
        check("test reach: one test on the second file of a chain of 4 reaches 3 of 4",
                r.tests() == 1 && r.countable() == 4 && r.reached() == 3
                        && Math.abs(r.grade() - 0.75) < 1e-12 && r.wanted().size() == 1
                        && r.wanted().get(0).file().equals("a"),
                r.toString());
        SystemModel cpp = model("cpp", "lib/x.hpp lib/x.cpp lib/y.hpp", "lib/x.cpp>lib/x.hpp")
                .withTests(List.of("tests/t.cpp"), Set.of(new Dep("tests/t.cpp", "lib/x.hpp")));
        TestReach h = TestReach.of(cpp, true);
        check("test reach: in C++ the headers alone count; a folder with no reached header is"
                        + " advice only when all its headers are unreached",
                TestReach.looksLikeCpp(cpp) && h.countable() == 2 && h.reached() == 1
                        && h.moves().isEmpty() && h.byFolder().size() == 1
                        && h.byFolder().get(0).unreached() == 1,
                h.toString());
        Path tmp = Files.createTempFile("aji-selftest", ".xml");
        try {
            XmlFiles.writeSystem(cpp, tmp, "self-test");
            SystemModel back = XmlFiles.readSystem(tmp);
            check("test reach: model.xml keeps the tests and their dependencies",
                    back.tests().equals(cpp.tests()) && back.testDeps().equals(cpp.testDeps())
                            && back.deps().equals(cpp.deps()), back.toString());
        } finally {
            Files.deleteIfExists(tmp);
        }
    }

    static void grouping() {
        SystemModel m = model("paths", "apps/fleet/include/fleet/gui/w.hpp apps/fleet/gui/w.cpp"
                + " facilities/gui/src/v.cpp src/aji/model/X.java top.cpp");
        Map<String, String> c = Grouping.COMPONENT.of(m);
        Map<String, String> f = Grouping.FOLDER.of(m);
        check("grouping: components stop at include, src or tests, or failing those at gui",
                c.get("apps/fleet/include/fleet/gui/w.hpp").equals("apps/fleet")
                        && c.get("apps/fleet/gui/w.cpp").equals("apps/fleet")
                        && c.get("facilities/gui/src/v.cpp").equals("facilities/gui")
                        && c.get("src/aji/model/X.java").equals("(root)")
                        && c.get("top.cpp").equals("(root)"),
                c.toString());
        check("grouping: folders are the parent directories",
                f.get("src/aji/model/X.java").equals("src/aji/model") && f.get("top.cpp").equals("(root)"),
                f.toString());
    }

    static void trials() {
        // Two folders, P above Q in the layers; one cut and one move suggested.
        List<FileNode> files = List.of(new FileNode("P/p1", 1), new FileNode("P/p2", 1),
                new FileNode("Q/q1", 2), new FileNode("Q/q2", 2), new FileNode("Q/q3", 2));
        Set<Dep> deps = Set.of(new Dep("Q/q1", "P/p1"), new Dep("Q/q2", "P/p1"),
                new Dep("Q/q3", "P/p2"), new Dep("Q/q1", "Q/q2"), new Dep("P/p2", "P/p1"));
        SystemModel m = new SystemModel("trials", files, deps);
        List<Move> suggested = List.of(
                new Move(Move.Kind.CUT, "Q/q3", "P/p2", "reach", ""),
                new Move(Move.Kind.CUT, "Q/q3", "P/p2", "direction", ""),
                new Move(Move.Kind.MOVE, "P/p2", "Q", "fit", ""),
                new Move(Move.Kind.SPLIT, "P/p1", "", "fit", ""));
        Trials t = Trials.of(m, Params.defaults(), Grouping.FOLDER, false, suggested);
        check("trials: the same cut asked for twice is tried once, and advice is kept apart",
                t.trials().size() == 2 && t.advice().size() == 1
                        && t.trials().stream().anyMatch(x -> x.askedBy().equals(Set.of("reach", "direction"))),
                t.trials().toString());
        SystemModel cut = m.without(new Dep("Q/q3", "P/p2"));
        Trials.Trial tc = t.trials().stream().filter(x -> x.move().kind() == Move.Kind.CUT).findFirst()
                .orElseThrow();
        Trials.Grades direct = Trials.grades(cut, Params.defaults(), Grouping.FOLDER.of(cut), false);
        check("trials: the grades after a cut are those of the model without the dependency",
                tc.after().equals(direct) && Math.abs(tc.after().mScore()
                        - MScore.evaluate(cut, Params.defaults()).mScore()) < 1e-12,
                tc.after() + " vs " + direct);
        Trials.Trial tm = t.trials().stream().filter(x -> x.move().kind() == Move.Kind.MOVE).findFirst()
                .orElseThrow();
        Map<String, Integer> layers = m.layerOf();
        layers.put("P/p2", 2);
        SystemModel moved = m.withLayers(layers, Map.of());
        Map<String, String> groups = new java.util.HashMap<>(Grouping.FOLDER.of(m));
        groups.put("P/p2", "Q");
        check("trials: a move gives the file the destination's layer and group, and the moved"
                        + " dependency P/p2 -> P/p1 adds no violation",
                Math.abs(tm.after().mScore() - MScore.evaluate(moved, Params.defaults()).mScore()) < 1e-12
                        && Math.abs(tm.after().q() - Fit.grade(moved, groups).q()) < 1e-12
                        && !tm.addsViolation(),
                tm.toString());
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
