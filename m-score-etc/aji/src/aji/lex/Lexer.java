package aji.lex;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;

/**
 * A small lexer for Java and C++ source, enough to find identifiers, dotted names and
 * {@code #include} lines while ignoring comments and literals.
 *
 * <p>It does not check that the source is well formed. Unterminated comments and literals end
 * at the end of the text (or, for ordinary literals, at the end of the line), so a damaged file
 * still yields its earlier tokens.
 */
public final class Lexer {

    public enum Dialect { JAVA, CPP }

    private static final Set<String> RAW_STRING_PREFIXES = Set.of("R", "u8R", "uR", "UR", "LR");

    private final String s;
    private final Dialect dialect;
    private final List<Token> out = new ArrayList<>();
    private int i;
    private int line = 1;
    private boolean atLineStart = true;

    private Lexer(String text, Dialect dialect) {
        this.s = text;
        this.dialect = dialect;
    }

    public static List<Token> tokens(String text, Dialect dialect) {
        Lexer lx = new Lexer(text, dialect);
        lx.run();
        return lx.out;
    }

    private void run() {
        while (i < s.length()) {
            char c = s.charAt(i);
            if (c == '\n') {
                line++;
                atLineStart = true;
                i++;
            } else if (Character.isWhitespace(c)) {
                i++;
            } else if (c == '/' && peek(1) == '/') {
                skipLineComment();
            } else if (c == '/' && peek(1) == '*') {
                skipBlockComment();
            } else if (c == '#' && atLineStart && dialect == Dialect.CPP) {
                directive();
            } else {
                atLineStart = false;
                if (c == '"') {
                    if (dialect == Dialect.JAVA && peek(1) == '"' && peek(2) == '"') {
                        textBlock();
                    } else {
                        quoted('"');
                    }
                } else if (c == '\'') {
                    quoted('\'');
                } else if (Character.isDigit(c) || (c == '.' && Character.isDigit(peek(1)))) {
                    number();
                } else if (Character.isJavaIdentifierStart(c)) {
                    identifier();
                } else {
                    punct();
                }
            }
        }
    }

    private char peek(int k) {
        int j = i + k;
        return j < s.length() ? s.charAt(j) : '\0';
    }

    private void skipLineComment() {
        while (i < s.length() && s.charAt(i) != '\n') {
            i++;
        }
    }

    private void skipBlockComment() {
        i += 2;
        while (i < s.length() && !(s.charAt(i) == '*' && peek(1) == '/')) {
            if (s.charAt(i) == '\n') {
                line++;
            }
            i++;
        }
        i = Math.min(s.length(), i + 2);
    }

    /** A preprocessor line; backslash-newline continues it. */
    private void directive() {
        int start = i;
        int startLine = line;
        StringBuilder b = new StringBuilder();
        while (i < s.length()) {
            char c = s.charAt(i);
            if (c == '\\' && (peek(1) == '\n' || (peek(1) == '\r' && peek(2) == '\n'))) {
                i += peek(1) == '\r' ? 3 : 2;
                line++;
                continue;
            }
            if (c == '\n') {
                break;
            }
            if (c == '/' && peek(1) == '*') {
                skipBlockComment();
                b.append(' ');
                continue;
            }
            if (c == '/' && peek(1) == '/') {
                skipLineComment();
                break;
            }
            b.append(c);
            i++;
        }
        if (i == start) {
            i++;
        }
        out.add(new Token(Token.Kind.DIRECTIVE, b.toString().trim(), startLine));
    }

    private void textBlock() {
        int startLine = line;
        i += 3;
        while (i < s.length()) {
            char c = s.charAt(i);
            if (c == '\\') {
                i += 2;
                continue;
            }
            if (c == '\n') {
                line++;
            }
            if (c == '"' && peek(1) == '"' && peek(2) == '"') {
                i += 3;
                break;
            }
            i++;
        }
        out.add(new Token(Token.Kind.STRING, "\"\"\"", startLine));
    }

    private void quoted(char q) {
        int start = i;
        i++;
        while (i < s.length()) {
            char c = s.charAt(i);
            if (c == '\\') {
                i += 2;
                continue;
            }
            if (c == '\n') {
                break;
            }
            i++;
            if (c == q) {
                break;
            }
        }
        out.add(new Token(Token.Kind.STRING, s.substring(start, Math.min(i, s.length())), line));
    }

    /** C++ raw string R"delim( ... )delim"; the prefix has already been read. */
    private void rawString() {
        int startLine = line;
        int open = s.indexOf('(', i);
        if (open < 0) {
            quoted('"');
            return;
        }
        String delim = s.substring(i + 1, open);
        String close = ")" + delim + "\"";
        int end = s.indexOf(close, open + 1);
        int stop = end < 0 ? s.length() : end + close.length();
        for (int j = i; j < stop; j++) {
            if (s.charAt(j) == '\n') {
                line++;
            }
        }
        i = stop;
        out.add(new Token(Token.Kind.STRING, "R\"\"", startLine));
    }

    private void number() {
        int start = i;
        while (i < s.length()) {
            char c = s.charAt(i);
            boolean exponentSign = (c == '+' || c == '-') && i > start
                    && "eEpP".indexOf(s.charAt(i - 1)) >= 0;
            boolean digitSeparator = c == '\'' && dialect == Dialect.CPP
                    && Character.isLetterOrDigit(peek(1));
            if (Character.isLetterOrDigit(c) || c == '_' || c == '.' || exponentSign || digitSeparator) {
                i++;
            } else {
                break;
            }
        }
        out.add(new Token(Token.Kind.NUMBER, s.substring(start, i), line));
    }

    private void identifier() {
        int start = i;
        i++;
        while (i < s.length() && Character.isJavaIdentifierPart(s.charAt(i))) {
            i++;
        }
        String id = s.substring(start, i);
        if (dialect == Dialect.CPP && i < s.length() && s.charAt(i) == '"'
                && RAW_STRING_PREFIXES.contains(id)) {
            rawString();
            return;
        }
        out.add(new Token(Token.Kind.IDENT, id, line));
    }

    private void punct() {
        char c = s.charAt(i);
        if (c == ':' && peek(1) == ':') {
            out.add(new Token(Token.Kind.PUNCT, "::", line));
            i += 2;
            return;
        }
        out.add(new Token(Token.Kind.PUNCT, String.valueOf(c), line));
        i++;
    }
}
