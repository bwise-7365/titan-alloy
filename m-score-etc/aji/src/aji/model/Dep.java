package aji.model;

import java.util.Comparator;

/**
 * A dependency between two files: file {@code from} depends on file {@code to}.
 *
 * <p>Each ordered pair of files counts once, however many references it stands for. This
 * follows the M-score paper, which treats every kind of dependency alike and ignores strength.
 */
public record Dep(String from, String to) {

    public static final Comparator<Dep> ORDER =
            Comparator.comparing(Dep::from).thenComparing(Dep::to);

    public Dep {
        if (from == null || to == null) {
            throw new IllegalArgumentException("a dependency is missing an end");
        }
        if (from.equals(to)) {
            throw new IllegalArgumentException("a file cannot depend on itself: " + from);
        }
    }
}
