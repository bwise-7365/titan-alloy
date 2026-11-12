package aji.app;

import aji.diagnose.Diagnosis;
import aji.graph.Graphs;
import aji.io.MeasureWriter;
import aji.io.ReportWriter;
import aji.io.XmlFiles;
import aji.measure.Direction;
import aji.measure.Fit;
import aji.measure.Grouping;
import aji.measure.Move;
import aji.measure.Reach;
import aji.measure.TestReach;
import aji.model.Dep;
import aji.model.Params;
import aji.model.SystemModel;
import aji.moves.Trials;
import aji.score.EdgeClasses;
import aji.score.MScore;
import aji.score.ModuleScore;
import aji.score.Result;
import aji.testing.Checks;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.function.BiConsumer;
import java.util.function.Predicate;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.stream.Collectors;

/**
 * One run of the command line on a real project, with the checks that hold for every project
 * whatever its code looks like. The test of each project adds the rules of that project's own
 * architecture.
 *
 * <p>The run is the ordinary one: aji reads the project file, extracts the dependencies and
 * writes report.txt, modules.csv and model.xml. The checks then read those three files, so
 * they cover the whole path from the source code to the outputs.
 *
 * @param report    the text of report.txt
 * @param csv       the text of modules.csv
 * @param model     the files, layers and dependencies of model.xml
 * @param result    model.xml scored again with the default parameters
 * @param derived   the same files scored with layers derived from the dependencies
 * @param explained what joins the modules of that result that lose the most
 */
record ProjectRun(String report, String csv, SystemModel model, Result result, Result derived,
                  List<Diagnosis> explained) {

    private static final double EPS = 1e-9;

    /**
     * The whole test of one project: runs aji, prints the headline figures, makes the general
     * checks and then the project's own, and ends the program with status 0 when every check
     * passed, otherwise 1.
     *
     * @param args         the command-line arguments; the first, if any, is the project file
     * @param defaultFile  the project file used when no argument is given
     * @param architecture the checks of the project's own architecture
     */
    static void test(String[] args, String defaultFile,
                     BiConsumer<ProjectRun, Checks> architecture) {
        Path file = Path.of(args.length > 0 ? args[0] : defaultFile);
        ProjectRun run;
        try {
            run = of(file);
        } catch (IOException ex) {
            System.err.println("aji: " + ex.getMessage());
            System.exit(1);
            return;
        }
        Checks c = new Checks();
        System.out.println();
        System.out.println("aji on " + run.model().name() + " (" + file + ")");
        run.describe(c);
        if (!run.explained().isEmpty()) {
            System.out.println();
            System.out.print(ReportWriter.explanation(run.explained()));
        }
        System.out.println();
        System.out.println("Checks that hold for every project");
        run.checkOutputs(c);
        run.checkFormula(c);
        run.checkExplanation(c);
        run.checkMeasures(c);
        System.out.println();
        System.out.println("Rules of the project's architecture");
        architecture.accept(run, c);
        System.exit(c.finish());
    }

    /**
     * Runs aji on a project file with its default options. For a project file NAME.aji.xml the
     * outputs go to {@code aji-out/NAME}. For the project files in {@code projects/} that is
     * the folder the command line uses by default.
     */
    static ProjectRun of(Path file) throws IOException {
        if (!Files.isRegularFile(file)) {
            throw new IOException("project file not found: " + file.toAbsolutePath()
                    + " (run from the aji folder or pass the path)");
        }
        String name = file.getFileName().toString().replaceFirst("(\\.aji)?\\.xml$", "");
        Path out = Path.of("aji-out", name);
        int status = Main.run(Main.parse(
                new String[] {file.toString(), "--out", out.toString(), "--quiet"}));
        if (status != 0) {
            throw new IOException("aji ended with status " + status + " on " + file);
        }
        String report = Files.readString(out.resolve("report.txt"), StandardCharsets.UTF_8);
        String csv = Files.readString(out.resolve("modules.csv"), StandardCharsets.UTF_8);
        SystemModel model = XmlFiles.readSystem(out.resolve("model.xml"));
        Result result = MScore.evaluate(model, Params.defaults());
        Result derived = MScore.evaluate(
                model.withLayers(Graphs.derivedLayers(model), Map.of()), Params.defaults());
        return new ProjectRun(report, csv, model, result, derived,
                Diagnosis.of(result, Main.DEFAULT_EXPLAIN));
    }

    // ---- checks that hold for every project ----------------------------------------------

    private void describe(Checks c) {
        c.info(String.format(Locale.ROOT, "%d files, %d dependencies, %d layers, %d modules",
                model.files().size(), model.deps().size(), result.layerCount(),
                result.modules().size()));
        c.info(String.format(Locale.ROOT,
                "M-score %.6f %%, with derived layers %.3f %%, propagation cost %.6f %%",
                100 * result.mScore(), 100 * derived.mScore(), 100 * result.propagationCost()));
        c.info(String.format(Locale.ROOT, "points lost: %.3f to module size, %.3f to dependencies"
                        + " from lower layers, %.3f to dependencies inside modules",
                100 * result.sizeLoss(), 100 * result.crossLoss(), 100 * result.innerLoss()));
    }

    /** The three output files agree with one another. */
    private void checkOutputs(Checks c) {
        Matcher m = Pattern.compile("M-score\\s+=\\s+([0-9.]+) %").matcher(report);
        double reported = m.find() ? Double.parseDouble(m.group(1)) : Double.NaN;
        c.check("model.xml scored again gives the M-score of report.txt",
                Math.abs(reported - 100 * result.mScore()) < 1e-6,
                String.format(Locale.ROOT, "report.txt %.6f %%, model.xml %.6f %%", reported,
                        100 * result.mScore()));

        List<String> lines = csv.lines().toList();
        int column = lines.isEmpty() ? -1
                : List.of(lines.get(0).split(",")).indexOf("contribution");
        double sum = column < 0 ? Double.NaN : lines.stream().skip(1)
                .mapToDouble(row -> Double.parseDouble(row.split(",")[column])).sum();
        c.check("modules.csv has one row per module, and its contributions add up to the M-score",
                lines.size() == result.modules().size() + 1
                        && Math.abs(sum - result.mScore()) < 1e-6,
                String.format(Locale.ROOT, "%d lines for %d modules, sum %.9f, M-score %.9f",
                        lines.size(), result.modules().size(), sum, result.mScore()));

        c.check("report.txt explains the modules that lose the most as model.xml does when"
                        + " scored again",
                explained.isEmpty() || report.contains(ReportWriter.explanation(explained)), "");
    }

    /** The pieces named in the explanation account for every file of the module. */
    private void checkExplanation(Checks c) {
        boolean files = true;
        boolean folders = true;
        boolean gains = true;
        for (Diagnosis d : explained) {
            int size = d.module().module().size();
            for (Diagnosis.Part p : d.joiningFiles()) {
                files &= p.pieces().size() >= 2 && sum(p.pieces()) == size - 1;
            }
            for (Diagnosis.Part p : d.folders()) {
                folders &= sum(p.pieces()) == size - p.files().size();
            }
            for (Diagnosis.WhatIf w : d.whatIfs()) {
                gains &= w.gain() > 0 && w.part().oneWay() && w.modules() >= 2
                        && w.largest() < size;
            }
        }
        c.check("without a joining file, the rest of its module is in two or more pieces", files,
                "");
        c.check("without a folder, the pieces hold all the other files of its module", folders,
                "");
        c.check("every change offered raises the M-score and divides its module", gains, "");

        boolean cycles = true;
        boolean finest = true;
        boolean crossings = true;
        for (Diagnosis d : explained) {
            int size = d.module().module().size();
            Set<String> inCycles = new HashSet<>();
            int held = 0;
            int largest = 1;
            for (Diagnosis.Cycle cycle : d.cycles()) {
                cycles &= cycle.files().size() >= 2 && cycle.deps() >= cycle.files().size()
                        && cycle.example().size() >= 2;
                inCycles.addAll(cycle.files());
                held += cycle.files().size();
                largest = Math.max(largest, cycle.files().size());
            }
            cycles &= inCycles.size() == held && held <= size;
            if (d.finest() != null) {
                finest &= d.finest().modules() == size - held + d.cycles().size()
                        && d.finest().largest() == largest;
            }
            Diagnosis.Crossings x = d.crossings();
            crossings &= x.pairs() == d.module().lowerDeps()
                    && x.pairs() == x.targets().stream().mapToInt(Diagnosis.Count::deps).sum()
                    && x.pairs() == x.sources().stream().mapToInt(Diagnosis.Count::deps).sum();
        }
        c.check("the cycles of a module share no file, and each has a dependency for every file",
                cycles, "");
        c.check("layered as finely as its cycles allow, a module is its cycles and its other"
                + " files, one by one", finest, "");
        c.check("the dependencies from lower layers, counted by file, add up to the number in"
                + " the score", crossings, "");
    }

    private static int sum(List<Integer> sizes) {
        return sizes.stream().mapToInt(Integer::intValue).sum();
    }

    /** Properties of the formula that hold for any files, layers and dependencies. */
    private void checkFormula(Checks c) {
        int n = model.files().size();
        EdgeClasses e = result.edges();
        Set<Dep> classified = new HashSet<>(e.internal());
        classified.addAll(e.downward());
        classified.addAll(e.upward());
        int classes = e.internal().size() + e.downward().size() + e.upward().size();
        c.check("every dependency is internal, downward or upward, and only one of them",
                classes == model.deps().size() && classified.equals(model.deps()),
                classes + " classified, " + model.deps().size() + " dependencies");

        Set<String> inModules = new HashSet<>();
        int moduleFiles = 0;
        for (ModuleScore ms : result.modules()) {
            inModules.addAll(ms.module().files());
            moduleFiles += ms.module().size();
        }
        c.check("every file is in exactly one module",
                moduleFiles == n && inModules.size() == n,
                moduleFiles + " files in modules, " + n + " files");

        double lost = result.sizeLoss() + result.crossLoss() + result.innerLoss();
        c.check("the three losses add up to 100 % minus the M-score",
                Math.abs(lost - (1 - result.mScore())) < EPS,
                String.format(Locale.ROOT, "losses %.9f, M-score %.9f", lost, result.mScore()));

        boolean fractions = true;
        boolean filesNotBelowPairs = true;
        for (Params.LowerScope lower : Params.LowerScope.values()) {
            Params p = Params.defaults().with(lower);
            double pairs = MScore.evaluate(model, p.with(Params.CrossCount.PAIRS)).mScore();
            double files = MScore.evaluate(model, p.with(Params.CrossCount.FILES)).mScore();
            fractions &= fraction(pairs) && fraction(files);
            filesNotBelowPairs &= files >= pairs - EPS;
        }
        c.check("the M-score is between 0 and 1 under all four readings", fractions, "");
        c.check("counting files instead of pairs never lowers the M-score", filesNotBelowPairs, "");

        double pc = result.propagationCost();
        c.check("the propagation cost is between 1/N and 1", pc >= 1.0 / n - EPS && pc <= 1 + EPS,
                String.valueOf(pc));
        c.check("layers derived from the dependencies leave no upward dependency",
                derived.edges().upward().isEmpty(), listing(derived.edges().upward()));
    }

    /** Properties of the four measures and of the combined list that hold for any project. */
    void checkMeasures(Checks c) {
        boolean cpp = TestReach.looksLikeCpp(model);
        Grouping grouping = Grouping.defaultFor(cpp);
        MeasureWriter.Measures ms = Main.measures(model, Params.defaults(), grouping, cpp);
        Reach r = ms.reach();
        int n = model.files().size();
        c.info(String.format(Locale.ROOT, "NCCD %.6f (CCD %d, ACD %.3f, %d levels), direction"
                        + " %.3f %%, Q %.6f, hidden %.3f %%, tests %s", r.nccd(), r.ccd(), r.acd(),
                r.levels(), 100 * ms.direction().grade(), ms.fit().q(),
                100 * ms.fit().hiddenShare(), ms.tests() == null ? "not declared"
                        : String.format(Locale.ROOT, "%.3f %% of %d %s", 100 * ms.tests().grade(),
                        ms.tests().countable(), ms.tests().headersOnly() ? "headers" : "files")));

        c.check("reach: CCD lies between N and N squared, and equals N squared times the"
                        + " propagation cost",
                r.ccd() >= n && r.ccd() <= (long) n * n
                        && Math.abs(r.propagationCost() - result.propagationCost()) < EPS,
                "CCD " + r.ccd());
        boolean cuts = true;
        long last = -1;
        for (Reach.Cut cut : r.cuts()) {
            cuts &= cut.ccdAfter() < r.ccd() && cut.ccdAfter() >= last
                    && Reach.grade(model.without(cut.dep())).ccd() == cut.ccdAfter();
            last = cut.ccdAfter();
        }
        c.check("reach: every cut offered lowers CCD by the amount stated, largest fall first",
                cuts, "");

        Direction d = ms.direction();
        int breaking = d.groups().stream().mapToInt(Direction.Group::breaking).sum();
        int violating = d.violations().stream().mapToInt(Direction.Violation::deps).sum();
        c.check("direction: the dependencies that break the principle, by group and by pair,"
                        + " add up to those not obeying it",
                breaking == d.between() - d.obeying() && violating == d.between() - d.obeying(),
                breaking + " by group, " + violating + " by pair, " + (d.between() - d.obeying())
                        + " not obeying");
        boolean instabilities = d.groups().stream().allMatch(g -> g.ca() + g.ce() == 0
                || (g.instability() >= 0 && g.instability() <= 1));
        boolean examples = d.violations().stream().allMatch(v -> v.fromI() < v.toI()
                && !v.examples().isEmpty() && v.examples().size() <= Direction.EXAMPLES);
        c.check("direction: every instability lies in [0, 1], and every violation points to a"
                + " less stable group with examples", instabilities && examples, "");

        Fit f = ms.fit();
        int files = f.profiles().stream().mapToInt(Fit.Profile::files).sum();
        boolean partsAddUp = f.profiles().stream().allMatch(p -> p.hidden() + p.inbound()
                + p.outbound() + p.transit() == p.files());
        c.check("fit: Q lies in [-1/2, 1], and the four profile counts of every group add up to"
                        + " its files, and the groups to N",
                f.q() >= -0.5 - EPS && f.q() <= 1 + EPS && partsAddUp && files == n,
                String.format(Locale.ROOT, "Q %.6f, %d files in profiles", f.q(), files));
        Map<String, String> groupOf = grouping.of(model);
        boolean shifts = true;
        for (Fit.Shift s : f.shifts()) {
            Map<String, String> moved = new java.util.HashMap<>(groupOf);
            moved.put(s.file(), s.to());
            shifts &= s.qAfter() > f.q() && s.there() > s.here()
                    && Math.abs(Fit.grade(model, moved).q() - s.qAfter()) < EPS;
        }
        c.check("fit: every move offered raises Q by the amount stated and goes where the file"
                + " has more neighbours", shifts, "");

        TestReach t = ms.tests();
        if (t != null) {
            int byLayer = t.byLayer().stream().mapToInt(TestReach.LayerShare::reached).sum();
            int unreached = t.byFolder().stream().mapToInt(TestReach.FolderShare::unreached).sum();
            c.check("test reach: the reached files by layer and the unreached by folder add up"
                            + " to the countable files",
                    byLayer == t.reached() && unreached + t.reached() == t.countable()
                            && t.reached() <= t.countable(),
                    byLayer + " reached by layer, " + unreached + " unreached by folder, "
                            + t.countable() + " countable");
            Set<String> ids = new HashSet<>();
            model.files().forEach(x -> ids.add(x.id()));
            boolean testDeps = model.testDeps().stream()
                    .allMatch(x -> model.tests().contains(x.from()) && ids.contains(x.to()));
            c.check("test reach: model.xml holds the tests and dependencies from tests to files only",
                    !model.tests().isEmpty() && testDeps, "");
        }

        Trials tr = ms.trials();
        Set<Move> suggested = new java.util.TreeSet<>(Move.SAME);
        List<Move> all = new java.util.ArrayList<>(r.moves());
        all.addAll(d.moves());
        all.addAll(f.moves());
        if (t != null) {
            all.addAll(t.moves());
        }
        suggested.addAll(all);
        long triable = suggested.stream().filter(Move::triable).count();
        c.check("moves: every triable move suggested is tried once, and the rest is advice",
                tr.trials().size() == triable && tr.advice().size() == all.size() - all.stream()
                        .filter(Move::triable).count(),
                tr.trials().size() + " trials for " + triable + " triable moves");
        boolean counted = true;
        boolean ordered = true;
        Trials.Trial previous = null;
        for (Trials.Trial x : tr.trials()) {
            int up = 0;
            int down = 0;
            for (int k = 0; k < Trials.COLUMNS.size(); k++) {
                int s = Trials.sign(tr.base(), x.after(), k);
                up += s > 0 ? 1 : 0;
                down += s < 0 ? 1 : 0;
            }
            counted &= up == x.improved() && down == x.worsened();
            if (previous != null) {
                ordered &= previous.improved() > x.improved()
                        || (previous.improved() == x.improved() && previous.worsened() <= x.worsened());
            }
            previous = x;
        }
        c.check("moves: the counts of measures improved and worsened match the signs, and the"
                + " list is ranked by them", counted && ordered, "");
        c.check("report.txt holds the measures as model.xml gives them when measured again",
                report.contains(MeasureWriter.text(ms, 25)), "");
    }

    private static boolean fraction(double v) {
        return v >= 0 && v <= 1 + EPS;
    }

    // ---- helpers for the tests of the individual projects --------------------------------

    /** The dependencies that satisfy a condition, in a fixed order. */
    List<Dep> deps(Predicate<Dep> condition) {
        return model.deps().stream().filter(condition).sorted(Dep.ORDER).toList();
    }

    /** The identifiers of the files that satisfy a condition, in the order of the model. */
    List<String> files(Predicate<String> condition) {
        return model.files().stream().map(f -> f.id()).filter(condition).toList();
    }

    /** Up to five dependencies written out, for the detail of a failed check. */
    static String listing(List<Dep> deps) {
        String shown = deps.stream().limit(5).map(d -> d.from() + " -> " + d.to())
                .collect(Collectors.joining("; "));
        return deps.size() > 5 ? shown + "; ... (" + deps.size() + " in all)" : shown;
    }

    /** The k-th name of a path, counting from 0: for "apps/minppd/main.cpp" and 1, "minppd". */
    static String part(String path, int k) {
        String[] parts = path.split("/");
        return k < parts.length ? parts[k] : "";
    }
}
