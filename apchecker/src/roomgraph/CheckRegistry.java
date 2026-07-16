package roomgraph;

import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

import roomgraph.rules.DeclaredRuleCheck;
import roomgraph.rules.Rule;
import roomgraph.rules.RuleFormatException;
import roomgraph.rules.RuleReader;

/**
 * The list of rules to run.
 *
 * Rules normally come from a rule file, so adding one means editing XML and
 * recompiling nothing. The hand-written escape hatch is still open: a Check
 * written in Java can be appended to the same list, and the rest of the program
 * cannot tell the difference. That matters, because the next rule somebody wants
 * may sit just past the boundary of the language, and the answer to that must
 * not always be "extend the language".
 */
public final class CheckRegistry {

    private CheckRegistry() {
        // Utility class.
    }

    /** Every declared rule, in the order the file declares them. */
    public static List<Check> fromRuleFile(Path ruleFile) throws RuleFormatException {
        Rule.Set ruleSet = RuleReader.read(ruleFile);
        return DeclaredRuleCheck.from(ruleSet);
    }

    /**
     * Declared rules, followed by any hand-written ones. Nothing is hard-coded
     * here today; the parameter is the seam.
     */
    public static List<Check> combined(List<Check> declared, List<Check> handWritten) {
        List<Check> all = new ArrayList<>(declared);
        all.addAll(handWritten);
        return List.copyOf(all);
    }

    /**
     * The verdict of every rule on the plan, in rule order: satisfied or broken,
     * for all of them, not merely the broken ones. This is what a search needs;
     * a list of violations cannot distinguish a rule that passed from a rule
     * that was never run.
     */
    public static List<Verdict> judge(List<Check> checks, PlanGraph plan) {
        List<Verdict> verdicts = new ArrayList<>();
        for (Check check : checks) {
            // The empty design is settled here, once, rather than in sixteen
            // rules. Nothing has been placed, so there is nothing any rule can
            // yet be right or wrong about.
            if (plan.isEmpty()) {
                verdicts.add(Verdict.undetermined(check.id(), check.severity()));
            } else {
                verdicts.add(check.judge(plan));
            }
        }
        return List.copyOf(verdicts);
    }

    /** Just the findings, in rule order, for a caller that is a person. */
    public static List<Violation> violations(List<Verdict> verdicts) {
        List<Violation> found = new ArrayList<>();
        for (Verdict verdict : verdicts) {
            found.addAll(verdict.violations());
        }
        return List.copyOf(found);
    }
}
