package aji.graph;

import aji.model.Dep;
import aji.model.FileNode;
import aji.model.SystemModel;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * The dependency graph of a {@link SystemModel} with files numbered 0..n-1.
 * {@code out[a]} lists the files that file {@code a} depends on.
 */
public record IndexedGraph(List<String> ids, Map<String, Integer> index, int[][] out) {

    public static IndexedGraph of(SystemModel m) {
        List<String> ids = new ArrayList<>();
        Map<String, Integer> index = new HashMap<>();
        for (FileNode f : m.files()) {
            index.put(f.id(), ids.size());
            ids.add(f.id());
        }
        List<List<Integer>> lists = new ArrayList<>();
        for (int k = 0; k < ids.size(); k++) {
            lists.add(new ArrayList<>());
        }
        for (Dep d : m.deps()) {
            lists.get(index.get(d.from())).add(index.get(d.to()));
        }
        int[][] out = new int[ids.size()][];
        for (int k = 0; k < out.length; k++) {
            out[k] = lists.get(k).stream().mapToInt(Integer::intValue).toArray();
        }
        return new IndexedGraph(List.copyOf(ids), Map.copyOf(index), out);
    }

    public int size() {
        return ids.size();
    }
}
