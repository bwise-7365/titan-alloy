package roomgraph;

/**
 * What a rule had to say about a design. Three values, not two.
 *
 * A partial design is not a small complete design. A rule about stairs, applied
 * to a design in which no stair has been placed, is not satisfied - the question
 * has not been asked yet. Reporting it as satisfied would tell a search that a
 * constraint had been met when nothing has been done about it, which is worse
 * than saying nothing.
 *
 * UNDETERMINED is produced in exactly two ways, and never inferred:
 *
 *   1. the design is empty - no rooms at all - in which case every rule is
 *      undetermined, without being run;
 *   2. the rule's own <given> presupposition does not hold.
 *
 * That second clause is why the language has <given>. Whether a rule applies is
 * a design decision, and design decisions belong in the rule file where they can
 * be read and argued with, not in an inference buried in an evaluator.
 */
public enum Outcome {

    /** The rule applies, and the design meets it. */
    SATISFIED,

    /** The rule applies, and the design breaks it. */
    BROKEN,

    /** The rule does not yet apply: there is nothing here for it to talk about. */
    UNDETERMINED
}
