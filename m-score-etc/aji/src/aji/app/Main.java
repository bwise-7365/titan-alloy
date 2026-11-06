package aji.app;

import aji.diagnose.Diagnosis;
import aji.extract.Extractor;
import aji.extract.Language;
import aji.extract.ProjectSpec;
import aji.graph.Graphs;
import aji.io.ReportWriter;
import aji.io.XmlFiles;
import aji.model.SystemModel;
import aji.score.MScore;
import aji.score.Params;
import aji.score.Result;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;

/**
 * aji: an approximate M-score evaluator for Java and C++ code.
 *
 * <pre>
 * aji INPUT.xml [--cross PAIRS|FILES] [--lower ALL|ADJACENT] [--upward EXCLUDE|FAIL]
 *               [--layers declared|derived] [--out DIR] [--top K] [--explain K] [--quiet]
 * </pre>
 *
 * INPUT is a project file ({@code <aji-project>}) or a dependency model ({@code <system>}).
 * Exit status: 0 on success, 1 for a usage or input error, 2 when {@code --upward FAIL} finds
 * an upward dependency.
 */
public final class Main {

    private static final String USAGE = """
            usage: aji INPUT.xml [options]
              INPUT.xml            an <aji-project> file (aji extracts the dependencies)
                                   or a <system> dependency model (files, layers, deps)
              --cross PAIRS|FILES  cross-layer count: dependency pairs (default) or files
              --lower ALL|ADJACENT lower layers that count: all (default) or the next one only
              --upward EXCLUDE|FAIL  upward dependencies: list them and leave them out
                                   of the score (default), or stop with exit status 2
              --layers declared|derived  for a project file with <layer> rules: use them
                                   (default) or derive layers from the dependencies
              --out DIR            output folder (default: aji-out/NAME)
              --top K              modules listed in the report (default 25)
              --explain K          modules for which the report explains the loss
                                   (default 3; 0 for none)
              --quiet              do not print the report on the console
            """;

    /** How many modules the report explains when {@code --explain} is not given. */
    static final int DEFAULT_EXPLAIN = 3;

    private Main() {
    }

    record Options(Path input, Params params, boolean derive, Path out, int top, int explain,
                   boolean quiet) {
    }

    public static void main(String[] args) {
        int status;
        try {
            status = run(parse(args));
        } catch (IllegalArgumentException ex) {
            System.err.println("aji: " + ex.getMessage());
            System.err.print(USAGE);
            status = 1;
        } catch (IOException ex) {
            System.err.println("aji: " + ex.getMessage());
            status = 1;
        } catch (MScore.UpwardDependencyException ex) {
            System.err.println("aji: " + ex.getMessage());
            ex.upward().forEach(d -> System.err.println("  " + d.from() + " -> " + d.to()));
            status = 2;
        }
        System.exit(status);
    }

    static Options parse(String[] args) {
        Path input = null;
        Params p = Params.defaults();
        boolean derive = false;
        Path out = null;
        int top = 25;
        int explain = DEFAULT_EXPLAIN;
        boolean quiet = false;
        for (int k = 0; k < args.length; k++) {
            String a = args[k];
            switch (a) {
                case "--cross" -> p = p.with(Params.parse(Params.CrossCount.class, value(args, ++k, a)));
                case "--lower" -> p = p.with(Params.parse(Params.LowerScope.class, value(args, ++k, a)));
                case "--upward" -> p = p.with(Params.parse(Params.UpwardPolicy.class, value(args, ++k, a)));
                case "--layers" -> {
                    String v = value(args, ++k, a);
                    if (!v.equals("declared") && !v.equals("derived")) {
                        throw new IllegalArgumentException("--layers takes declared or derived");
                    }
                    derive = v.equals("derived");
                }
                case "--out" -> out = Path.of(value(args, ++k, a));
                case "--top" -> top = whole(args, ++k, a);
                case "--explain" -> explain = whole(args, ++k, a);
                case "--quiet" -> quiet = true;
                case "-h", "--help" -> throw new IllegalArgumentException("help requested");
                default -> {
                    if (a.startsWith("--") || input != null) {
                        throw new IllegalArgumentException("unexpected argument: " + a);
                    }
                    input = Path.of(a);
                }
            }
        }
        if (input == null) {
            throw new IllegalArgumentException("no input file given");
        }
        return new Options(input, p, derive, out, top, explain, quiet);
    }

    private static int whole(String[] args, int k, String option) {
        try {
            return Integer.parseInt(value(args, k, option));
        } catch (NumberFormatException ex) {
            throw new IllegalArgumentException(option + " takes a whole number");
        }
    }

    private static String value(String[] args, int k, String option) {
        if (k >= args.length) {
            throw new IllegalArgumentException(option + " needs a value");
        }
        return args[k];
    }

    static int run(Options o) throws IOException {
        String kind = XmlFiles.rootName(o.input());
        SystemModel model;
        String language = null;
        String layering;
        List<String> dropped = List.of();
        List<String> notes = List.of();
        boolean declaredProject = false;
        if (kind.equals("aji-project")) {
            ProjectSpec spec = XmlFiles.readProject(o.input());
            Extractor.Extraction x = Extractor.run(spec, o.derive());
            model = x.model();
            language = spec.language() == Language.JAVA
                    ? "Java (type references and imports)" : "C++ (#include lines)";
            layering = x.layering();
            dropped = x.unlayered();
            notes = x.notes();
            declaredProject = x.layering().equals("declared");
            if (!model.layered()) {
                model = model.withLayers(Graphs.derivedLayers(model), Map.of());
                layering = "derived from the dependencies (longest path, bottom-aligned)";
            }
        } else if (kind.equals("system")) {
            model = XmlFiles.readSystem(o.input());
            layering = "given";
            if (!model.layered()) {
                model = model.withLayers(Graphs.derivedLayers(model), Map.of());
                layering = "derived (the model gave no layers)";
            }
        } else {
            throw new IOException(o.input() + ": expected <aji-project> or <system>, found <" + kind + ">");
        }
        if (model.files().isEmpty()) {
            throw new IOException(o.input() + ": no source files found; the M-score is undefined");
        }

        Result main = MScore.evaluate(model, o.params());
        List<Result> variants = new ArrayList<>();
        for (Params.CrossCount c : Params.CrossCount.values()) {
            for (Params.LowerScope l : Params.LowerScope.values()) {
                variants.add(MScore.evaluate(model, new Params(c, l, Params.UpwardPolicy.EXCLUDE)));
            }
        }
        Result derived = null;
        if (declaredProject) {
            SystemModel d = model.withLayers(Graphs.derivedLayers(model), Map.of());
            derived = MScore.evaluate(d, o.params().with(Params.UpwardPolicy.EXCLUDE));
        }

        String report = ReportWriter.text(new ReportWriter.Input(main, variants,
                o.input().toString(), language, layering, dropped, notes, derived, o.top(),
                Diagnosis.of(main, o.explain())));
        Path out = o.out() != null ? o.out() : Path.of("aji-out", safe(model.name()));
        Files.createDirectories(out);
        Files.writeString(out.resolve("report.txt"), report, StandardCharsets.UTF_8);
        Files.writeString(out.resolve("modules.csv"), ReportWriter.modulesCsv(main),
                StandardCharsets.UTF_8);
        XmlFiles.writeSystem(model, out.resolve("model.xml"),
                "written by aji from " + o.input().getFileName() + "; layers " + layering);
        if (!o.quiet()) {
            System.out.print(report);
        }
        System.out.println("Wrote report.txt, modules.csv and model.xml in "
                + out.toAbsolutePath().normalize());
        return 0;
    }

    private static String safe(String name) {
        return name.replaceAll("[^A-Za-z0-9._-]", "_");
    }
}
