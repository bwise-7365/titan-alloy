package aji.spec;

import aji.lex.Lexer;

import java.util.Locale;
import java.util.Set;

/** The source languages aji can read, with their file extensions. */
public enum Language {

    JAVA(Lexer.Dialect.JAVA, Set.of("java")),
    CPP(Lexer.Dialect.CPP, Set.of("h", "hh", "hpp", "hxx", "h++", "ipp", "tpp", "inl", "ixx",
            "c", "cc", "cpp", "cxx", "c++", "cppm"));

    private final Lexer.Dialect dialect;
    private final Set<String> extensions;

    Language(Lexer.Dialect dialect, Set<String> extensions) {
        this.dialect = dialect;
        this.extensions = extensions;
    }

    public Lexer.Dialect dialect() {
        return dialect;
    }

    public boolean accepts(String fileName) {
        int dot = fileName.lastIndexOf('.');
        return dot >= 0 && extensions.contains(fileName.substring(dot + 1).toLowerCase(Locale.ROOT));
    }

    /** Accepts "java", "cpp", "c++" or "cxx", in any case. */
    public static Language parse(String s) {
        String t = s.trim().toLowerCase(Locale.ROOT);
        return switch (t) {
            case "java" -> JAVA;
            case "cpp", "c++", "cxx" -> CPP;
            default -> throw new IllegalArgumentException("unknown language: " + s
                    + " (use java or cpp)");
        };
    }
}
