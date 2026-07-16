package roomgraph;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import roomgraph.rules.RuleFormatException;

/**
 * The conformance suite, in Java.
 *
 * Every plan in conformance/ has an expected report beside it; a checker is
 * conforming exactly when, run over that plan and rules.xml, it produces a report
 * that {@link ReportCompare} judges to agree with the expected one. Agreement is
 * about CONTENT, not bytes: the order of verdicts and findings does not matter,
 * and a rule missing from a report is read as UNDETERMINED.
 *
 * With no arguments the suite runs the Java checker in process, so it needs no
 * child JVM, no separate classpath and no report written to disk to hand to a
 * second process. Run it in IntelliJ (the "ConformanceTest" configuration) or
 * from the command line:
 *
 * <pre>
 *   java -cp out/production/apchecker roomgraph.ConformanceTest
 * </pre>
 *
 * With {@code --cmd} the suite instead drives an external checker, so an
 * independent implementation - an optimized C++ checker, say - is held to the
 * same reports as the Java. For each plan it runs
 *
 * <pre>
 *   &lt;command&gt; --plan &lt;plan&gt; --rules &lt;rules&gt; --report &lt;tmp&gt;
 * </pre>
 *
 * and compares the report the command writes. The command is split on whitespace
 * and does not go through a shell, so it may name an executable and fixed flags
 * but not quoting, pipes or redirection. Its exit status is ignored: a conforming
 * checker exits nonzero when a CODE rule breaks - the Java one does - so only the
 * report content is compared, never the exit code.
 *
 * <pre>
 *   java -cp out/production/apchecker roomgraph.ConformanceTest --cmd "./build/roomgraph-cpp"
 * </pre>
 *
 * Exit status is 0 when every case conforms, 1 when any case fails or errors, and
 * 2 on a bad command line or a missing suite.
 */
public final class ConformanceTest {

    private static final String USAGE =
            "Usage: java roomgraph.ConformanceTest [--cmd \"<checker command>\"] [--dir <project-dir>]";

    /** One conformance case: a plan and the report it is expected to produce. */
    private record Case(String name, Path plan, Path expected) {
    }

    /**
     * The parsed command line. A null command means run the Java checker in
     * process; a null directory means find the suite by walking up from the
     * working directory.
     */
    private record Config(List<String> command, Path dir) {

        static Config parse(String[] args) {
            List<String> command = null;
            Path dir = null;
            int i = 0;
            while (i < args.length) {
                String flag = args[i];
                if (i + 1 >= args.length) {
                    throw new IllegalArgumentException("The flag " + flag + " needs a value.");
                }
                String value = args[i + 1];
                i += 2;
                switch (flag) {
                    case "--cmd" -> command = split(value);
                    case "--dir" -> dir = Path.of(value);
                    default -> throw new IllegalArgumentException("Unknown flag: " + flag);
                }
            }
            return new Config(command, dir);
        }

        /** The command string as an argument vector, split on runs of whitespace. */
        private static List<String> split(String command) {
            String trimmed = command.strip();
            if (trimmed.isEmpty()) {
                throw new IllegalArgumentException("--cmd needs a command to run.");
            }
            return Arrays.asList(trimmed.split("\\s+"));
        }
    }

    private ConformanceTest() {
        // Entry point only.
    }

    public static void main(String[] args) {
        Config config;
        try {
            config = Config.parse(args);
        } catch (IllegalArgumentException failure) {
            System.err.println(failure.getMessage());
            System.err.println(USAGE);
            System.exit(2);
            return;
        }

        Path base = (config.dir() != null ? config.dir() : findBase()).toAbsolutePath().normalize();
        Path rules = base.resolve("rules.xml");
        Path conformance = base.resolve("conformance");

        if (!Files.isDirectory(conformance) || !Files.isRegularFile(rules)) {
            System.err.println("Could not find the suite. Expected a 'conformance' directory and 'rules.xml'");
            System.err.println("under " + base + ".");
            System.err.println("Run from the apchecker directory, or pass it with --dir.");
            System.exit(2);
            return;
        }

        List<Case> cases = discover(conformance);
        if (cases.isEmpty()) {
            System.err.println("No conformance plans found in " + conformance + ".");
            System.exit(2);
            return;
        }

        if (config.command() != null) {
            System.out.println("Checker: " + String.join(" ", config.command()));
            System.out.println();
        }

        int failed = 0;
        for (Case one : cases) {
            List<String> complaints = check(one, rules, config.command(), base);
            if (complaints.isEmpty()) {
                System.out.println("  ok    " + one.name());
            } else {
                failed++;
                System.out.println(" FAIL   " + one.name());
                for (String complaint : complaints) {
                    System.out.println("            " + complaint);
                }
            }
        }

        System.out.println();
        System.out.println(cases.size() + " cases, " + (cases.size() - failed) + " passed, " + failed + " failed.");
        if (failed == 0) {
            System.out.println("Conforming.");
            System.exit(0);
        } else {
            System.out.println("NOT conforming.");
            System.exit(1);
        }
    }

    /**
     * Every plan in the directory that has an expected report beside it, in a
     * stable order so the run reads the same way each time.
     */
    private static List<Case> discover(Path conformance) {
        List<Case> cases = new ArrayList<>();
        try (var entries = Files.list(conformance)) {
            entries.map(Path::getFileName)
                    .map(Path::toString)
                    .filter(name -> name.endsWith(".xml") && !name.endsWith(".expected.xml"))
                    .sorted()
                    .forEach(name -> {
                        String base = name.substring(0, name.length() - ".xml".length());
                        Path expected = conformance.resolve(base + ".expected.xml");
                        if (Files.isRegularFile(expected)) {
                            cases.add(new Case(base, conformance.resolve(name), expected));
                        }
                    });
        } catch (IOException failure) {
            throw new RuntimeException("Could not list " + conformance + ": " + failure.getMessage(), failure);
        }
        return cases;
    }

    /**
     * Empty when the case conforms; otherwise the disagreements, or the single
     * error that stopped the case from producing a report at all. A null command
     * runs the Java checker in process; otherwise the external command produces
     * the report.
     */
    private static List<String> check(Case one, Path rules, List<String> command, Path base) {
        Path actual = null;
        try {
            actual = command == null
                    ? runChecker(one.plan(), rules)
                    : runExternal(command, one.plan(), rules, base);
            var expectedReport = ReportCompare.read(one.expected());
            var actualReport = ReportCompare.read(actual);
            return ReportCompare.compare(expectedReport, actualReport);
        } catch (PlanFormatException failure) {
            return List.of("bad plan: " + failure.getMessage());
        } catch (RuleFormatException failure) {
            return List.of("bad rule file: " + failure.getMessage());
        } catch (InterruptedException failure) {
            Thread.currentThread().interrupt();
            return List.of("interrupted while waiting for the checker");
        } catch (Exception failure) {
            return List.of("error: " + failure.getClass().getSimpleName() + ": " + failure.getMessage());
        } finally {
            if (actual != null) {
                try {
                    Files.deleteIfExists(actual);
                } catch (IOException ignored) {
                    // A leftover temp file is not a conformance failure.
                }
            }
        }
    }

    /** Runs the Java checker over one plan and writes its report to a temp file. */
    private static Path runChecker(Path plan, Path rules)
            throws PlanFormatException, RuleFormatException, IOException {
        List<Check> checks = CheckRegistry.fromRuleFile(rules);
        PlanGraph graph = PlanReader.read(plan);
        List<Verdict> verdicts = CheckRegistry.judge(checks, graph);

        Path report = Files.createTempFile("conformance-", ".xml");
        Files.writeString(report, ReportWriter.toXml(graph, verdicts), StandardCharsets.UTF_8);
        return report;
    }

    /**
     * Runs an external checker over one plan and returns the temp file it was
     * asked to write. The command runs from the project directory, so a relative
     * program name and relative paths resolve as they would for a hand-typed run.
     * The exit status is deliberately not consulted; the report it leaves behind
     * is the whole of the evidence.
     */
    private static Path runExternal(List<String> command, Path plan, Path rules, Path base)
            throws IOException, InterruptedException {
        Path report = Files.createTempFile("conformance-", ".xml");

        List<String> full = new ArrayList<>(command);
        full.add("--plan");
        full.add(plan.toString());
        full.add("--rules");
        full.add(rules.toString());
        full.add("--report");
        full.add(report.toString());

        ProcessBuilder builder = new ProcessBuilder(full);
        builder.directory(base.toFile());
        builder.redirectOutput(ProcessBuilder.Redirect.DISCARD);
        builder.redirectError(ProcessBuilder.Redirect.DISCARD);
        builder.start().waitFor();
        return report;
    }

    /**
     * The project directory. Walks up from the working directory looking for the
     * one that holds the suite, so the test runs whether it is launched from the
     * project root or from a nested output directory.
     */
    private static Path findBase() {
        Path dir = Path.of("").toAbsolutePath();
        for (Path at = dir; at != null; at = at.getParent()) {
            if (Files.isDirectory(at.resolve("conformance")) && Files.isRegularFile(at.resolve("rules.xml"))) {
                return at;
            }
        }
        return dir;
    }
}
