package aji.model;

import java.util.Locale;

/**
 * The choices the published M-score definition leaves open, made visible as switches.
 *
 * @param cross  what the cross-layer numerator counts (ambiguity 2 of the session notes)
 * @param lower  which lower layers count (ambiguity 3 of the session notes)
 * @param upward what to do with dependencies that point from an upper layer to a lower one
 */
public record Params(CrossCount cross, LowerScope lower, UpwardPolicy upward) {

    /**
     * PAIRS: distinct (lower file, module file) dependency pairs, so the penalty is a true
     * density in [0, 1]. FILES: distinct lower-layer files that depend on the module.
     */
    public enum CrossCount { PAIRS, FILES }

    /** ALL: every layer below the module's layer. ADJACENT: only the next layer down. */
    public enum LowerScope { ALL, ADJACENT }

    /**
     * EXCLUDE: leave upward dependencies out of the score and list them. FAIL: refuse to score
     * a system that has any.
     */
    public enum UpwardPolicy { EXCLUDE, FAIL }

    public static Params defaults() {
        return new Params(CrossCount.PAIRS, LowerScope.ALL, UpwardPolicy.EXCLUDE);
    }

    public Params with(CrossCount c) {
        return new Params(c, lower, upward);
    }

    public Params with(LowerScope l) {
        return new Params(cross, l, upward);
    }

    public Params with(UpwardPolicy u) {
        return new Params(cross, lower, u);
    }

    public String label() {
        return "cross=" + cross + ", lower=" + lower;
    }

    public static <E extends Enum<E>> E parse(Class<E> type, String s) {
        try {
            return Enum.valueOf(type, s.trim().toUpperCase(Locale.ROOT));
        } catch (IllegalArgumentException e) {
            throw new IllegalArgumentException("unknown value \"" + s + "\" for "
                    + type.getSimpleName());
        }
    }
}
