package aji.extract;

import java.util.regex.Pattern;

/**
 * A path pattern matched against a file's path relative to the project root, written with
 * forward slashes. Matching ignores case.
 *
 * <ul>
 *   <li>{@code *} matches any run of characters within one directory name;</li>
 *   <li>{@code ?} matches one character other than a slash;</li>
 *   <li>{@code **} matches any run of characters, slashes included, and {@code **}{@code /}
 *       also matches nothing at all;</li>
 *   <li>a pattern ending in a slash matches everything below that directory;</li>
 *   <li>a pattern with no slash matches a file name at any depth.</li>
 * </ul>
 */
public record Glob(String pattern, Pattern regex) {

    public static Glob of(String pattern) {
        String p = pattern.trim().replace('\\', '/');
        if (p.startsWith("./")) {
            p = p.substring(2);
        }
        if (p.endsWith("/")) {
            p = p + "**";
        }
        if (!p.contains("/")) {
            p = "**/" + p;
        }
        return new Glob(pattern, Pattern.compile(toRegex(p),
                Pattern.CASE_INSENSITIVE | Pattern.UNICODE_CASE));
    }

    public boolean matches(String relPath) {
        return regex.matcher(relPath).matches();
    }

    private static String toRegex(String p) {
        StringBuilder b = new StringBuilder();
        int i = 0;
        while (i < p.length()) {
            char c = p.charAt(i);
            if (c == '*' && i + 1 < p.length() && p.charAt(i + 1) == '*') {
                if (i + 2 < p.length() && p.charAt(i + 2) == '/') {
                    b.append("(?:.*/)?");
                    i += 3;
                } else {
                    b.append(".*");
                    i += 2;
                }
            } else if (c == '*') {
                b.append("[^/]*");
                i++;
            } else if (c == '?') {
                b.append("[^/]");
                i++;
            } else {
                b.append(Pattern.quote(String.valueOf(c)));
                i++;
            }
        }
        return b.toString();
    }
}
