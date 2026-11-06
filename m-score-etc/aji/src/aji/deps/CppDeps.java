package aji.deps;

import aji.lex.Lexer;
import aji.lex.Token;
import aji.model.Dep;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.TreeMap;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * Finds file-to-file dependencies in C++ source from its {@code #include} lines.
 *
 * <p>File A depends on file B when A includes B directly. This is the physical dependency
 * graph; a file that uses a name declared in a header it reaches only indirectly (through
 * another header) is not linked to that header. Includes inside {@code #if 0} blocks are
 * counted; includes inside comments are not.
 *
 * <p>An include is resolved by trying, in order: the including file's own directory (for the
 * quoted form), each declared include directory, and finally a search of all project files
 * whose path ends with the included path. When that search finds several files, the one
 * sharing the longest directory prefix with the including file wins. Includes that match no
 * project file (the standard library, Eigen, Qt and so on) are ignored.
 */
public final class CppDeps {

    private static final Pattern INCLUDE =
            Pattern.compile("^#\\s*include\\s*([<\"])([^>\"]+)[>\"]");

    private CppDeps() {
    }

    public static Found find(List<SourceUnit> units, List<String> includeDirs) {
        Map<String, String> byLowerPath = new HashMap<>();
        Map<String, List<String>> byLowerName = new HashMap<>();
        for (SourceUnit u : units) {
            String lower = u.path().toLowerCase(Locale.ROOT);
            byLowerPath.put(lower, u.path());
            byLowerName.computeIfAbsent(baseName(lower), k -> new ArrayList<>()).add(u.path());
        }
        Set<Dep> deps = new HashSet<>();
        Map<String, Integer> unresolvedQuoted = new TreeMap<>();
        Set<String> unresolvedAngled = new HashSet<>();
        Set<String> ambiguous = new HashSet<>();
        for (SourceUnit u : units) {
            for (Token tok : Lexer.tokens(u.text(), Lexer.Dialect.CPP)) {
                if (tok.kind() != Token.Kind.DIRECTIVE) {
                    continue;
                }
                Matcher m = INCLUDE.matcher(tok.text());
                if (!m.find()) {
                    continue;
                }
                boolean quoted = m.group(1).equals("\"");
                String inc = m.group(2).trim().replace('\\', '/');
                String target = resolve(u, inc, quoted, includeDirs, byLowerPath, byLowerName,
                        ambiguous);
                if (target == null) {
                    if (quoted) {
                        unresolvedQuoted.merge(inc, 1, Integer::sum);
                    } else {
                        unresolvedAngled.add(inc);
                    }
                } else if (!target.equals(u.path())) {
                    deps.add(new Dep(u.path(), target));
                }
            }
        }
        List<String> notes = new ArrayList<>();
        notes.add(unresolvedAngled.size() + " distinct <...> includes matched no project file"
                + " (normally the standard library and external packages)");
        if (!unresolvedQuoted.isEmpty()) {
            notes.add(unresolvedQuoted.size() + " distinct \"...\" includes matched no project file: "
                    + abbreviate(unresolvedQuoted.keySet(), 12));
        }
        if (!ambiguous.isEmpty()) {
            notes.add(ambiguous.size() + " includes matched several files and were resolved by the"
                    + " nearest directory: " + abbreviate(ambiguous, 8));
        }
        return new Found(deps, notes);
    }

    private static String resolve(SourceUnit from, String inc, boolean quoted,
                                  List<String> includeDirs, Map<String, String> byLowerPath,
                                  Map<String, List<String>> byLowerName, Set<String> ambiguous) {
        if (quoted) {
            String hit = byLowerPath.get(normalize(join(from.dir(), inc)).toLowerCase(Locale.ROOT));
            if (hit != null) {
                return hit;
            }
        }
        for (String dir : includeDirs) {
            String hit = byLowerPath.get(normalize(join(dir, inc)).toLowerCase(Locale.ROOT));
            if (hit != null) {
                return hit;
            }
        }
        String key = stripParents(normalize(inc)).toLowerCase(Locale.ROOT);
        List<String> candidates = new ArrayList<>();
        for (String p : byLowerName.getOrDefault(baseName(key), List.of())) {
            String lower = p.toLowerCase(Locale.ROOT);
            if (lower.equals(key) || lower.endsWith("/" + key)) {
                candidates.add(p);
            }
        }
        if (candidates.isEmpty()) {
            return null;
        }
        if (candidates.size() == 1) {
            return candidates.get(0);
        }
        String best = null;
        int bestShared = -1;
        boolean tie = false;
        for (String c : candidates) {
            int shared = sharedDirs(from.path(), c);
            if (shared > bestShared) {
                best = c;
                bestShared = shared;
                tie = false;
            } else if (shared == bestShared) {
                tie = true;
                if (c.compareTo(best) < 0) {
                    best = c;
                }
            }
        }
        if (tie) {
            ambiguous.add(from.path() + " -> " + inc);
        }
        return best;
    }

    static String join(String dir, String rel) {
        return dir.isEmpty() ? rel : dir + "/" + rel;
    }

    /** Removes "." segments and resolves ".." segments where possible. */
    static String normalize(String path) {
        List<String> parts = new ArrayList<>();
        for (String seg : path.split("/")) {
            if (seg.isEmpty() || seg.equals(".")) {
                continue;
            }
            if (seg.equals("..") && !parts.isEmpty() && !parts.get(parts.size() - 1).equals("..")) {
                parts.remove(parts.size() - 1);
            } else {
                parts.add(seg);
            }
        }
        return String.join("/", parts);
    }

    private static String stripParents(String path) {
        String p = path;
        while (p.startsWith("../")) {
            p = p.substring(3);
        }
        return p;
    }

    private static String baseName(String path) {
        return path.substring(path.lastIndexOf('/') + 1);
    }

    private static int sharedDirs(String a, String b) {
        String[] x = a.split("/");
        String[] y = b.split("/");
        int k = 0;
        while (k < x.length - 1 && k < y.length - 1 && x[k].equalsIgnoreCase(y[k])) {
            k++;
        }
        return k;
    }

    private static String abbreviate(Iterable<String> items, int max) {
        StringBuilder b = new StringBuilder();
        int k = 0;
        int total = 0;
        for (String s : items) {
            total++;
            if (k < max) {
                b.append(k == 0 ? "" : ", ").append(s);
                k++;
            }
        }
        if (total > max) {
            b.append(", ... (").append(total - max).append(" more)");
        }
        return b.toString();
    }
}
