package aji.deps;

/** One source file's relative path (forward slashes) and its text. */
public record SourceUnit(String path, String text) {

    /** The directory part of the path, without a trailing slash; empty at the root. */
    public String dir() {
        int slash = path.lastIndexOf('/');
        return slash < 0 ? "" : path.substring(0, slash);
    }
}
