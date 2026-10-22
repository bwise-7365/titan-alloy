package roomgraph;

/**
 * One design rule, expressed as a pure function from a design to a verdict. A
 * check never mutates the plan and never reports anything about a feature the
 * plan does not declare.
 *
 * A check must return UNDETERMINED, not SATISFIED, when the design contains
 * nothing for it to talk about. The empty design is handled centrally in
 * {@link CheckRegistry}, but a check with its own presupposition - "there is a
 * stair", "there is a garage" - is responsible for its own.
 *
 * Rules normally come from the rule file. This interface is the escape hatch for
 * one that outgrows the language; nothing downstream can tell the difference.
 */
public interface Check {

    /** Short identifier, e.g. "APL-131a". Appears in every finding it raises. */
    String id();

    /** One line saying what the rule is and where it comes from. */
    String description();

    /**
     * How seriously to take a breach. One rule, one severity: a rule that would
     * be CODE in some circumstances and ADVICE in others is two rules.
     */
    Severity severity();

    /** The rule's verdict on the design. */
    Verdict judge(PlanGraph plan);
}
