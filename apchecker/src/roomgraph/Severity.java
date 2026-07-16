package roomgraph;

/**
 * How seriously to take a finding.
 *
 * The distinction matters because the rules come from two different places.
 * A CODE finding is a model-building-code requirement and is not negotiable.
 * A PATTERN finding is a design convention from the literature: worth a fight,
 * but a designer may knowingly overrule it. ADVICE is a remark, not a fault.
 */
public enum Severity {
    CODE,
    PATTERN,
    ADVICE
}
