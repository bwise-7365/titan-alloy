package roomgraph;

import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;

import roomgraph.rules.RuleFormatException;
import roomgraph.rules.RuleSchema;

/**
 * Command line entry point.
 *
 * <pre>
 *   java roomgraph.Main --plan plan.xml --rules rules.xml \
 *                       [--svg out.svg] [--plan-schema plan.xsd] [--rules-schema rules.xsd]
 * </pre>
 *
 * Prints every finding, and writes the justified graph when asked. Schema
 * validation is optional and gives a sharper message than the readers' own,
 * though the readers check everything again in any case. The exit status is 1
 * when any CODE-severity finding was raised, so the tool composes with a build.
 */
public final class Main {

    public static void main(String[] args) {
        Options options;
        try {
            options = Options.parse(args);
        } catch (IllegalArgumentException failure) {
            System.err.println(failure.getMessage());
            System.err.println();
            System.err.println(Options.USAGE);
            System.exit(2);
            return;
        }

        try {
            System.exit(run(options));
        } catch (PlanFormatException failure) {
            System.err.println("Bad plan: " + failure.getMessage());
            System.exit(2);
        } catch (RuleFormatException failure) {
            System.err.println("Bad rule file: " + failure.getMessage());
            System.exit(2);
        } catch (IOException failure) {
            System.err.println("Could not write the drawing: " + failure.getMessage());
            System.exit(2);
        }
    }

    private static int run(Options options) throws PlanFormatException, RuleFormatException, IOException {
        validate(options);

        List<Check> checks = CheckRegistry.fromRuleFile(options.rules());
        PlanGraph plan = PlanReader.read(options.plan());
        List<Verdict> verdicts = CheckRegistry.judge(checks, plan);
        List<Violation> violations = CheckRegistry.violations(verdicts);

        report(plan, verdicts);
        write(options, plan, verdicts, violations);

        return hasCodeViolation(verdicts) ? 1 : 0;
    }

    private static void validate(Options options) throws PlanFormatException, RuleFormatException {
        if (options.ruleSchema().isPresent()) {
            RuleSchema.validate(options.rules(), options.ruleSchema().get());
            System.out.println("Validated " + options.rules() + " against " + options.ruleSchema().get() + ".");
        }
        if (options.planSchema().isPresent()) {
            PlanValidator.validate(options.plan(), options.planSchema().get());
            System.out.println("Validated " + options.plan() + " against " + options.planSchema().get() + ".");
        }
        if (options.ruleSchema().isPresent() || options.planSchema().isPresent()) {
            System.out.println();
        }
    }

    private static void write(Options options, PlanGraph plan, List<Verdict> verdicts, List<Violation> violations)
            throws IOException {

        if (options.svg().isPresent()) {
            Path out = options.svg().get();
            Files.writeString(out, SvgWriter.toSvg(plan, violations), StandardCharsets.UTF_8);
            System.out.println();
            System.out.println("Wrote " + out.toAbsolutePath());
        }
        if (options.report().isPresent()) {
            Path out = options.report().get();
            Files.writeString(out, ReportWriter.toXml(plan, verdicts), StandardCharsets.UTF_8);
            System.out.println();
            System.out.println("Wrote " + out.toAbsolutePath());
        }
    }

    /**
     * The whole verdict vector, not merely the broken rules. A partial design is
     * meant to break rules it has not yet satisfied; what a caller needs is which
     * ones, and which ones it has already got right.
     */
    private static void report(PlanGraph plan, List<Verdict> verdicts) {
        System.out.println("Plan: " + plan.name());
        System.out.println("Rooms: " + plan.rooms().size() + ", connections: " + plan.connections().size());
        System.out.println();

        for (Verdict verdict : verdicts) {
            String mark = switch (verdict.outcome()) {
                case SATISFIED -> "  ok   ";
                case BROKEN -> " BROKE ";
                case UNDETERMINED -> "  ??   ";
            };
            System.out.println(mark + " [" + verdict.severity() + "] " + verdict.ruleId());
            for (Violation violation : verdict.violations()) {
                System.out.println("            " + violation.message());
            }
        }
    }

    /** Only a BROKEN code rule fails the build. An undetermined one is not a fault. */
    private static boolean hasCodeViolation(List<Verdict> verdicts) {
        for (Verdict verdict : verdicts) {
            if (verdict.outcome() == Outcome.BROKEN && verdict.severity() == Severity.CODE) {
                return true;
            }
        }
        return false;
    }

    private Main() {
        // Entry point only.
    }
}
