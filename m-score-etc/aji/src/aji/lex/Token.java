package aji.lex;

/**
 * One token of C-family source text.
 *
 * <p>Comments are dropped. A string or character literal becomes one {@code STRING} token. In
 * C++, a whole preprocessor line (with its continuation lines) becomes one {@code DIRECTIVE}
 * token whose text is the line itself.
 */
public record Token(Kind kind, String text, int line) {

    public enum Kind { IDENT, NUMBER, STRING, PUNCT, DIRECTIVE }

    public boolean is(Kind k, String s) {
        return kind == k && text.equals(s);
    }

    public boolean isPunct(String s) {
        return is(Kind.PUNCT, s);
    }

    public boolean isIdent() {
        return kind == Kind.IDENT;
    }
}
