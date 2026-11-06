package aji.extract;

import aji.lex.Lexer;
import aji.lex.Token;
import aji.model.Dep;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Finds file-to-file dependencies in Java source without compiling it.
 *
 * <p>File A depends on file B when A imports a type declared in B, or when A's code names such
 * a type, either by a simple name that is visible in A or by a qualified name. A simple name is
 * visible through a single-type import, through A's own package, or through an on-demand
 * import, with that order of precedence (as in the Java language). Names in comments and
 * literals are ignored.
 *
 * <p>This is an approximation. A local variable or method whose name equals a type name in
 * scope is mistaken for that type; Java's naming conventions make this rare. Member types
 * reached only through inheritance are not seen, but a file that inherits from a class
 * already depends on that class's file.
 */
public final class JavaDeps {

    private JavaDeps() {
    }

    /** A declared type: its fully qualified name, its simple name, and whether it is top-level. */
    record TypeDecl(String fqn, String simple, boolean topLevel) {
    }

    /** What one file declares and mentions. */
    record Facts(String path, String pkg, List<String> imports, List<String> staticImports,
                 List<String> onDemand, List<TypeDecl> declared, List<List<String>> chains) {
    }

    public static Found find(List<SourceUnit> units) {
        List<Facts> all = new ArrayList<>();
        for (SourceUnit u : units) {
            all.add(facts(u.path(), Lexer.tokens(u.text(), Lexer.Dialect.JAVA)));
        }
        List<String> notes = new ArrayList<>();
        Map<String, String> fqnToFile = new HashMap<>();
        Map<String, Map<String, String>> packageTypes = new HashMap<>();
        for (Facts f : all) {
            for (TypeDecl t : f.declared()) {
                String earlier = fqnToFile.putIfAbsent(t.fqn(), f.path());
                if (earlier != null && !earlier.equals(f.path())) {
                    notes.add("type " + t.fqn() + " is declared in both " + earlier + " and "
                            + f.path() + "; the first is used");
                }
                if (t.topLevel()) {
                    packageTypes.computeIfAbsent(f.pkg(), k -> new HashMap<>())
                            .putIfAbsent(t.simple(), fqnToFile.get(t.fqn()));
                }
            }
        }
        Set<Dep> deps = new HashSet<>();
        for (Facts f : all) {
            for (String target : targets(f, fqnToFile, packageTypes)) {
                if (!target.equals(f.path())) {
                    deps.add(new Dep(f.path(), target));
                }
            }
        }
        return new Found(deps, notes);
    }

    private static Set<String> targets(Facts f, Map<String, String> fqnToFile,
                                       Map<String, Map<String, String>> packageTypes) {
        Set<String> targets = new HashSet<>();
        Map<String, String> visible = new HashMap<>();
        for (String od : f.onDemand()) {
            visible.putAll(packageTypes.getOrDefault(od, Map.of()));
            String outer = fqnToFile.get(od);
            if (outer != null) {
                targets.add(outer);
                for (Map.Entry<String, String> e : fqnToFile.entrySet()) {
                    String k = e.getKey();
                    if (k.startsWith(od + ".") && k.indexOf('.', od.length() + 1) < 0) {
                        visible.put(k.substring(od.length() + 1), e.getValue());
                    }
                }
            }
        }
        visible.putAll(packageTypes.getOrDefault(f.pkg(), Map.of()));
        for (String imp : f.imports()) {
            String file = prefixFile(split(imp), 1, fqnToFile);
            if (file != null) {
                targets.add(file);
                visible.put(imp.substring(imp.lastIndexOf('.') + 1), file);
            }
        }
        for (String si : f.staticImports()) {
            String file = prefixFile(split(si), 1, fqnToFile);
            if (file != null) {
                targets.add(file);
            }
        }
        for (List<String> chain : f.chains()) {
            String file = visible.get(chain.get(0));
            if (file == null && chain.size() > 1) {
                file = prefixFile(chain, 2, fqnToFile);
            }
            if (file != null) {
                targets.add(file);
            }
        }
        return targets;
    }

    /** The file of the shortest prefix (of at least {@code min} names) that is a known type. */
    private static String prefixFile(List<String> names, int min, Map<String, String> fqnToFile) {
        StringBuilder b = new StringBuilder();
        for (int k = 0; k < names.size(); k++) {
            if (k > 0) {
                b.append('.');
            }
            b.append(names.get(k));
            if (k + 1 >= min) {
                String file = fqnToFile.get(b.toString());
                if (file != null) {
                    return file;
                }
            }
        }
        return null;
    }

    private static List<String> split(String dotted) {
        return List.of(dotted.split("\\."));
    }

    /** Reads the package, imports, declared types and dotted names of one file. */
    static Facts facts(String path, List<Token> t) {
        String pkg = "";
        List<String> imports = new ArrayList<>();
        List<String> staticImports = new ArrayList<>();
        List<String> onDemand = new ArrayList<>();
        List<TypeDecl> declared = new ArrayList<>();
        List<List<String>> chains = new ArrayList<>();
        List<String> openTypes = new ArrayList<>();
        List<Integer> openDepths = new ArrayList<>();
        String pending = null;
        int depth = 0;
        int i = 0;
        while (i < t.size()) {
            Token k = t.get(i);
            if (k.isPunct("{")) {
                depth++;
                if (pending != null) {
                    openTypes.add(pending);
                    openDepths.add(depth);
                    pending = null;
                }
                i++;
            } else if (k.isPunct("}")) {
                if (!openDepths.isEmpty() && openDepths.get(openDepths.size() - 1) == depth) {
                    openTypes.remove(openTypes.size() - 1);
                    openDepths.remove(openDepths.size() - 1);
                }
                depth--;
                i++;
            } else if (k.isPunct(";")) {
                pending = null;
                i++;
            } else if (depth == 0 && k.is(Token.Kind.IDENT, "package")) {
                List<String> name = new ArrayList<>();
                i = dotted(t, i + 1, name);
                pkg = String.join(".", name);
            } else if (depth == 0 && k.is(Token.Kind.IDENT, "import")) {
                i = importStatement(t, i + 1, imports, staticImports, onDemand);
            } else if (k.isIdent() && declaresType(t, i)) {
                String name = t.get(i + 1).text();
                String outer = openTypes.isEmpty() ? pkg
                        : (pkg.isEmpty() ? "" : pkg + ".") + String.join(".", openTypes);
                String fqn = outer.isEmpty() ? name : outer + "." + name;
                declared.add(new TypeDecl(fqn, name, openTypes.isEmpty()));
                pending = name;
                i += 2;
            } else if (k.isIdent()) {
                List<String> chain = new ArrayList<>();
                boolean member = i > 0 && t.get(i - 1).isPunct(".");
                i = dotted(t, i, chain);
                if (!member) {
                    chains.add(chain);
                }
            } else {
                i++;
            }
        }
        return new Facts(path, pkg, imports, staticImports, onDemand, declared, chains);
    }

    /** Reads IDENT ('.' IDENT)* starting at {@code i}; returns the index after it. */
    private static int dotted(List<Token> t, int i, List<String> names) {
        if (i < t.size() && t.get(i).isIdent()) {
            names.add(t.get(i).text());
            i++;
            while (i + 1 < t.size() && t.get(i).isPunct(".") && t.get(i + 1).isIdent()) {
                names.add(t.get(i + 1).text());
                i += 2;
            }
        }
        return i;
    }

    private static int importStatement(List<Token> t, int i, List<String> imports,
                                       List<String> staticImports, List<String> onDemand) {
        boolean isStatic = i < t.size() && t.get(i).is(Token.Kind.IDENT, "static");
        if (isStatic) {
            i++;
        }
        List<String> names = new ArrayList<>();
        i = dotted(t, i, names);
        boolean star = i + 1 < t.size() && t.get(i).isPunct(".") && t.get(i + 1).isPunct("*");
        if (star) {
            i += 2;
        }
        String name = String.join(".", names);
        if (!name.isEmpty()) {
            if (isStatic) {
                staticImports.add(name);
            } else if (star) {
                onDemand.add(name);
            } else {
                imports.add(name);
            }
        }
        while (i < t.size() && !t.get(i).isPunct(";")) {
            i++;
        }
        return i + 1;
    }

    /** True when token i begins a class, interface, enum, record or annotation declaration. */
    private static boolean declaresType(List<Token> t, int i) {
        String w = t.get(i).text();
        if (!(w.equals("class") || w.equals("interface") || w.equals("enum") || w.equals("record"))) {
            return false;
        }
        if (i > 0 && (t.get(i - 1).isPunct(".") || t.get(i - 1).isPunct("::"))) {
            return false;
        }
        if (i + 1 >= t.size() || !t.get(i + 1).isIdent()) {
            return false;
        }
        if (w.equals("record")) {
            return i + 2 < t.size() && (t.get(i + 2).isPunct("(") || t.get(i + 2).isPunct("<"));
        }
        return true;
    }
}
