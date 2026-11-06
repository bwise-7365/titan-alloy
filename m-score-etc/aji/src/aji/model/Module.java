package aji.model;

import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * A module: files of one layer that are connected, directly or indirectly, by dependencies
 * inside that layer (in either direction).
 *
 * @param layer the layer's rank, 1 (top) to n (bottom), after empty layers are removed
 * @param files the module's files, sorted
 */
public record Module(int layer, List<String> files) {

    public Module {
        files = List.copyOf(files);
    }

    public int size() {
        return files.size();
    }

    /**
     * The directory that all the module's files share, with its closing slash, or the empty
     * string when they share none.
     */
    public String directory() {
        String first = files.get(0);
        int end = first.length();
        for (String f : files) {
            int k = 0;
            while (k < end && k < f.length() && f.charAt(k) == first.charAt(k)) {
                k++;
            }
            end = k;
        }
        int slash = first.substring(0, end).lastIndexOf('/');
        return slash < 0 ? "" : first.substring(0, slash + 1);
    }

    /**
     * The module's files grouped by the folder directly below {@link #directory()}, in the
     * order of the folders' names. A folder's name ends with a slash. The files directly in
     * the directory are under the empty name.
     */
    public Map<String, List<String>> folders() {
        String dir = directory();
        Map<String, List<String>> folders = new TreeMap<>();
        for (String f : files) {
            String rest = f.substring(dir.length());
            int s = rest.indexOf('/');
            folders.computeIfAbsent(s < 0 ? "" : rest.substring(0, s + 1), k -> new ArrayList<>())
                    .add(f);
        }
        return folders;
    }

    /**
     * A short description: the file itself for a single file; otherwise the directory the
     * files share, the subdirectories directly below it that they spread over (when there are
     * at most four), and the file count.
     */
    public String label() {
        if (files.size() == 1) {
            return files.get(0);
        }
        String dir = directory();
        List<String> subdirs = new ArrayList<>(folders().keySet());
        boolean direct = subdirs.remove("");
        String where = dir.isEmpty() ? "(root)/" : dir;
        if (!subdirs.isEmpty() && subdirs.size() <= 4) {
            where += "{" + String.join(", ", subdirs) + (direct ? ", files" : "") + "}";
        } else {
            where += "*";
        }
        return where + " [" + files.size() + " files]";
    }
}
