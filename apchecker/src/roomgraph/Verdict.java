package roomgraph;

import java.util.List;

/**
 * What one rule had to say about one design.
 *
 * A caller that receives no verdict for a rule must read that as UNDETERMINED.
 * An implementation may therefore omit undetermined verdicts from its report;
 * this one includes them, because a reader is better served by seeing the whole
 * vector than by having to remember the convention.
 */
public record Verdict(String ruleId, Severity severity, Outcome outcome, List<Violation> violations) {

    public Verdict {
        if (ruleId == null || severity == null || outcome == null || violations == null) {
            throw new IllegalArgumentException("A verdict needs a rule, a severity, an outcome and a list.");
        }
        if ((outcome == Outcome.BROKEN) != !violations.isEmpty()) {
            throw new IllegalArgumentException("Rule '" + ruleId
                    + "': BROKEN means at least one violation, and any other outcome means none.");
        }
        violations = List.copyOf(violations);
    }

    public static Verdict satisfied(String ruleId, Severity severity) {
        return new Verdict(ruleId, severity, Outcome.SATISFIED, List.of());
    }

    public static Verdict undetermined(String ruleId, Severity severity) {
        return new Verdict(ruleId, severity, Outcome.UNDETERMINED, List.of());
    }

    public static Verdict broken(String ruleId, Severity severity, List<Violation> violations) {
        if (violations.isEmpty()) {
            throw new IllegalArgumentException("Rule '" + ruleId + "' is broken but says nothing about why.");
        }
        return new Verdict(ruleId, severity, Outcome.BROKEN, violations);
    }
}
