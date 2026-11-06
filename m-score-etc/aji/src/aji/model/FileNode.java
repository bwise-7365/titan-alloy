package aji.model;

/**
 * One source file of the system being measured.
 *
 * <p>The identifier is the file's path relative to the project root, written with forward
 * slashes. Layer 1 is the top layer, the one most depended upon; larger numbers are lower
 * layers. Layer 0 means that no layer has been assigned yet.
 */
public record FileNode(String id, int layer) {

    public static final int UNASSIGNED = 0;

    public FileNode {
        if (id == null || id.isBlank()) {
            throw new IllegalArgumentException("a file has an empty identifier");
        }
        if (layer < 0) {
            throw new IllegalArgumentException("file " + id + " has a negative layer: " + layer);
        }
    }

    public FileNode withLayer(int newLayer) {
        return new FileNode(id, newLayer);
    }
}
