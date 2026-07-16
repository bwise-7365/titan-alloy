package roomgraph;

import java.nio.file.Path;
import java.util.Optional;

/**
 * The command line, parsed.
 *
 * Flags rather than positions: there are now five paths a run may involve, and
 * a positional list of five would be unreadable and easy to get wrong in a
 * build script.
 */
public record Options(Path plan, Path rules, Optional<Path> svg, Optional<Path> report,
        Optional<Path> planSchema, Optional<Path> ruleSchema) {

    public static final String USAGE =
            "Usage: java roomgraph.Main --plan <plan.xml> --rules <rules.xml>\n"
            + "                          [--svg <out.svg>] [--report <report.xml>]\n"
            + "                          [--plan-schema <plan.xsd>]\n"
            + "                          [--rules-schema <rules.xsd>]\n"
            + "\n"
            + "Exit status is 1 when any CODE-severity finding was raised, 2 on a bad file.";

    public Options {
        if (plan == null || rules == null) {
            throw new IllegalArgumentException("Both --plan and --rules are required.");
        }
    }

    /** Throws IllegalArgumentException with a usable message on any fault. */
    public static Options parse(String[] args) {
        Path plan = null;
        Path rules = null;
        Path svg = null;
        Path report = null;
        Path planSchema = null;
        Path ruleSchema = null;

        int i = 0;
        while (i < args.length) {
            String flag = args[i];
            if (i + 1 >= args.length) {
                throw new IllegalArgumentException("The flag " + flag + " needs a value.");
            }
            String value = args[i + 1];
            i += 2;

            switch (flag) {
                case "--plan" -> plan = Path.of(value);
                case "--rules" -> rules = Path.of(value);
                case "--svg" -> svg = Path.of(value);
                case "--report" -> report = Path.of(value);
                case "--plan-schema" -> planSchema = Path.of(value);
                case "--rules-schema" -> ruleSchema = Path.of(value);
                default -> throw new IllegalArgumentException("Unknown flag: " + flag);
            }
        }

        if (plan == null) {
            throw new IllegalArgumentException("--plan is required.");
        }
        if (rules == null) {
            throw new IllegalArgumentException("--rules is required.");
        }
        return new Options(plan, rules,
                Optional.ofNullable(svg),
                Optional.ofNullable(report),
                Optional.ofNullable(planSchema),
                Optional.ofNullable(ruleSchema));
    }
}
