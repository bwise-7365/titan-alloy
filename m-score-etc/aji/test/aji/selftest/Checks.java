package aji.selftest;

/**
 * Counts and prints the checks of one test run. A check prints a PASS or FAIL line; an INFO
 * line is printed and not counted. The self-test and the tests on real projects share this
 * class, so that none of them needs a test library.
 */
public final class Checks {

    private int passed;
    private int failed;

    /** Records one check. The detail is printed only when the check fails. */
    public void check(String name, boolean ok, String detail) {
        if (ok) {
            passed++;
            System.out.println("  PASS " + name);
        } else {
            failed++;
            System.out.println("  FAIL " + name + (detail.isEmpty() ? "" : " -- " + detail));
        }
    }

    /** Prints a line that is not counted. */
    public void info(String text) {
        System.out.println("  INFO " + text);
    }

    /** Prints the totals and returns the exit status: 0 when no check failed, otherwise 1. */
    public int finish() {
        System.out.println();
        System.out.println(passed + " passed, " + failed + " failed");
        return failed == 0 ? 0 : 1;
    }
}
