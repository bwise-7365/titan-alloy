package aji.graph;

import aji.model.SystemModel;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.BitSet;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * Standard graph computations used by the evaluator: weakly and strongly connected
 * components, the pieces a graph falls into when nodes are taken out, a shortest cycle, a
 * longest-path layering, and propagation cost. All are pure functions.
 */
public final class Graphs {

    private Graphs() {
    }

    /**
     * Weakly connected components: files joined by an edge in either direction share a
     * component. Returns a component number for each node, numbered from 0 in order of first
     * appearance.
     */
    public static int[] weakComponents(int n, List<int[]> edges) {
        int[] parent = new int[n];
        for (int k = 0; k < n; k++) {
            parent[k] = k;
        }
        for (int[] e : edges) {
            int a = find(parent, e[0]);
            int b = find(parent, e[1]);
            if (a != b) {
                parent[Math.max(a, b)] = Math.min(a, b);
            }
        }
        int[] comp = new int[n];
        Map<Integer, Integer> number = new HashMap<>();
        for (int k = 0; k < n; k++) {
            comp[k] = number.computeIfAbsent(find(parent, k), r -> number.size());
        }
        return comp;
    }

    private static int find(int[] parent, int k) {
        while (parent[k] != k) {
            parent[k] = parent[parent[k]];
            k = parent[k];
        }
        return k;
    }

    /**
     * The sizes, largest first, of the connected pieces that remain when some nodes are taken
     * out. Edges count in either direction, as in {@link #weakComponents}.
     *
     * @param removed for each node, whether it is taken out
     */
    public static int[] pieceSizes(int n, List<int[]> edges, boolean[] removed) {
        List<int[]> kept = new ArrayList<>();
        for (int[] e : edges) {
            if (!removed[e[0]] && !removed[e[1]]) {
                kept.add(e);
            }
        }
        int[] comp = weakComponents(n, kept);
        int[] count = new int[n];
        for (int k = 0; k < n; k++) {
            if (!removed[k]) {
                count[comp[k]]++;
            }
        }
        return descending(Arrays.stream(count).filter(c -> c > 0).toArray());
    }

    /**
     * For each node, the sizes, largest first, of the connected pieces that remain of the
     * node's own component when that node is taken out. A node whose removal leaves two or
     * more pieces is an articulation point: it joins parts of the graph that would otherwise
     * be separate. Edges count in either direction.
     *
     * <p>This is the depth-first search of Hopcroft and Tarjan for articulation points,
     * written without recursion. It takes time proportional to the nodes plus the edges, and
     * gives the sizes that {@link #pieceSizes} gives for a connected graph when called once
     * for each node.
     */
    public static int[][] piecesWithoutEach(int n, List<int[]> edges) {
        int[][] adj = neighbours(n, edges);
        int[] disc = new int[n];     // order of discovery; -1 until the node is reached
        int[] low = new int[n];      // earliest node reachable from the node's subtree
        int[] size = new int[n];     // nodes in the node's subtree of the search
        int[] parent = new int[n];
        int[] order = new int[n];    // order[k] is the node discovered k-th
        int[] cutOff = new int[n];   // nodes in the subtrees that the node's removal cuts off
        List<List<Integer>> pieces = new ArrayList<>();
        for (int k = 0; k < n; k++) {
            pieces.add(new ArrayList<>());
        }
        Arrays.fill(disc, -1);
        int[] callNode = new int[n];
        int[] callEdge = new int[n];
        int counter = 0;
        for (int s = 0; s < n; s++) {
            if (disc[s] != -1) {
                continue;
            }
            int first = counter;
            int depth = 0;
            callNode[0] = s;
            callEdge[0] = 0;
            parent[s] = -1;
            order[counter] = s;
            disc[s] = low[s] = counter++;
            size[s] = 1;
            while (depth >= 0) {
                int v = callNode[depth];
                if (callEdge[depth] < adj[v].length) {
                    int w = adj[v][callEdge[depth]++];
                    if (disc[w] == -1) {
                        parent[w] = v;
                        order[counter] = w;
                        disc[w] = low[w] = counter++;
                        size[w] = 1;
                        depth++;
                        callNode[depth] = w;
                        callEdge[depth] = 0;
                    } else if (w != parent[v]) {
                        low[v] = Math.min(low[v], disc[w]);
                    }
                } else {
                    depth--;
                    if (depth >= 0) {
                        int u = callNode[depth];
                        size[u] += size[v];
                        low[u] = Math.min(low[u], low[v]);
                        if (low[v] >= disc[u]) {
                            // nothing in v's subtree reaches above u: without u it is a piece
                            pieces.get(u).add(size[v]);
                            cutOff[u] += size[v];
                        }
                    }
                }
            }
            // What is left of the component beside a node and the subtrees it cuts off is one
            // more piece. For the root of the search nothing is left.
            int total = counter - first;
            for (int k = first; k < counter; k++) {
                int v = order[k];
                int rest = total - 1 - cutOff[v];
                if (rest > 0) {
                    pieces.get(v).add(rest);
                }
            }
        }
        int[][] out = new int[n][];
        for (int k = 0; k < n; k++) {
            out[k] = descending(pieces.get(k).stream().mapToInt(Integer::intValue).toArray());
        }
        return out;
    }

    /** For each node, its neighbours when edges count in either direction. */
    private static int[][] neighbours(int n, List<int[]> edges) {
        int[] degree = new int[n];
        for (int[] e : edges) {
            degree[e[0]]++;
            degree[e[1]]++;
        }
        int[][] adj = new int[n][];
        for (int k = 0; k < n; k++) {
            adj[k] = new int[degree[k]];
        }
        int[] used = new int[n];
        for (int[] e : edges) {
            adj[e[0]][used[e[0]]++] = e[1];
            adj[e[1]][used[e[1]]++] = e[0];
        }
        return adj;
    }

    /** Sorts an array in place, largest first, and returns it. */
    private static int[] descending(int[] a) {
        Arrays.sort(a);
        for (int lo = 0, hi = a.length - 1; lo < hi; lo++, hi--) {
            int t = a[lo];
            a[lo] = a[hi];
            a[hi] = t;
        }
        return a;
    }

    /**
     * Strongly connected components by Tarjan's algorithm, written without recursion.
     * Components are numbered in the order Tarjan's algorithm completes them, so for every
     * edge a -> b between different components, comp[b] < comp[a].
     */
    public static int[] strongComponents(int[][] out) {
        int n = out.length;
        int[] index = new int[n];
        int[] low = new int[n];
        int[] comp = new int[n];
        Arrays.fill(index, -1);
        Arrays.fill(comp, -1);
        boolean[] onStack = new boolean[n];
        int[] stack = new int[n];
        int[] callNode = new int[n];
        int[] callEdge = new int[n];
        int sp = 0;
        int counter = 0;
        int compCount = 0;
        for (int s = 0; s < n; s++) {
            if (index[s] != -1) {
                continue;
            }
            int depth = 0;
            callNode[0] = s;
            callEdge[0] = 0;
            index[s] = low[s] = counter++;
            stack[sp++] = s;
            onStack[s] = true;
            while (depth >= 0) {
                int v = callNode[depth];
                if (callEdge[depth] < out[v].length) {
                    int w = out[v][callEdge[depth]++];
                    if (index[w] == -1) {
                        index[w] = low[w] = counter++;
                        stack[sp++] = w;
                        onStack[w] = true;
                        depth++;
                        callNode[depth] = w;
                        callEdge[depth] = 0;
                    } else if (onStack[w]) {
                        low[v] = Math.min(low[v], index[w]);
                    }
                } else {
                    if (low[v] == index[v]) {
                        int w;
                        do {
                            w = stack[--sp];
                            onStack[w] = false;
                            comp[w] = compCount;
                        } while (w != v);
                        compCount++;
                    }
                    depth--;
                    if (depth >= 0) {
                        int u = callNode[depth];
                        low[u] = Math.min(low[u], low[v]);
                    }
                }
            }
        }
        return comp;
    }

    /**
     * One of the shortest cycles among some nodes of a graph: each node of the result has an
     * edge to the next, and the last has an edge to the first. The result is empty when the
     * nodes hold no cycle. Found by a breadth-first search from each of the nodes in turn.
     *
     * @param out     for each node of the graph, the nodes it has an edge to
     * @param members the nodes the cycle may pass through
     */
    public static int[] shortestCycle(int[][] out, int[] members) {
        int n = out.length;
        boolean[] in = new boolean[n];
        for (int v : members) {
            in[v] = true;
        }
        int[] best = new int[0];
        int[] dist = new int[n];
        int[] prev = new int[n];
        int[] queue = new int[n];
        for (int s : members) {
            Arrays.fill(dist, -1);
            dist[s] = 0;
            int head = 0;
            int tail = 0;
            queue[tail++] = s;
            int last = -1;   // the node whose edge to s closes the cycle
            while (head < tail && last < 0) {
                int v = queue[head++];
                if (best.length > 0 && dist[v] + 1 >= best.length) {
                    break;   // a cycle through v is no shorter than the best so far
                }
                for (int w : out[v]) {
                    if (w == s) {
                        last = v;
                        break;
                    }
                    if (in[w] && dist[w] < 0) {
                        dist[w] = dist[v] + 1;
                        prev[w] = v;
                        queue[tail++] = w;
                    }
                }
            }
            if (last >= 0) {
                best = new int[dist[last] + 1];
                int v = last;
                for (int k = best.length - 1; k >= 0; k--) {
                    best[k] = v;
                    v = prev[v];
                }
            }
        }
        return best;
    }

    /**
     * Longest-path layering of the condensation graph, with the files that nothing depends on
     * placed in the bottom layer.
     *
     * <p>A file's height is 0 when no other file depends on it; otherwise it is one more than
     * the largest height among the files that depend on it (files in one cycle share a height).
     * Its layer is {@code H - height + 1}, where H is the largest height, so layer 1 is the top
     * and every dependency points from a larger layer number to a smaller one, or stays within
     * a cycle. This is a textbook layering (as in Sugiyama-style graph drawing), not the
     * Design Rule Hierarchy clustering that the M-score paper used.
     */
    public static Map<String, Integer> derivedLayers(SystemModel m) {
        IndexedGraph g = IndexedGraph.of(m);
        int n = g.size();
        int[] comp = strongComponents(g.out());
        int k = Arrays.stream(comp).max().orElse(-1) + 1;
        int[][] byComp = membersByComponent(comp, k);
        int[] height = new int[k];
        for (int c = k - 1; c >= 0; c--) {
            for (int a : byComp[c]) {
                for (int b : g.out()[a]) {
                    int cb = comp[b];
                    if (cb != c) {
                        height[cb] = Math.max(height[cb], height[c] + 1);
                    }
                }
            }
        }
        int top = Arrays.stream(height).max().orElse(0);
        Map<String, Integer> layer = new HashMap<>();
        for (int a = 0; a < n; a++) {
            layer.put(g.ids().get(a), top - height[comp[a]] + 1);
        }
        return layer;
    }

    /**
     * Propagation cost (MacCormack, Rusnak and Baldwin): the density of the visibility matrix,
     * that is, the number of ordered pairs (a, b) such that b is reachable from a along
     * dependencies, counting a itself, divided by N squared.
     */
    public static double propagationCost(SystemModel m) {
        IndexedGraph g = IndexedGraph.of(m);
        int n = g.size();
        if (n == 0) {
            return Double.NaN;
        }
        return (double) cumulativeDependency(reach(g.out())) / ((double) n * n);
    }

    /**
     * For each node, the set of nodes it reaches along edges, counting itself. Computed on the
     * condensation graph: the nodes of one strongly connected component share one set, and
     * Tarjan's numbering lets the sets be built in one pass from the components that are
     * reached toward those that reach them.
     */
    public static BitSet[] reach(int[][] out) {
        int n = out.length;
        int[] comp = strongComponents(out);
        int k = Arrays.stream(comp).max().orElse(-1) + 1;
        int[][] byComp = membersByComponent(comp, k);
        BitSet[] ofComp = new BitSet[k];
        for (int c = 0; c < k; c++) {
            ofComp[c] = new BitSet(n);
            for (int a : byComp[c]) {
                ofComp[c].set(a);
            }
            for (int a : byComp[c]) {
                for (int b : out[a]) {
                    if (comp[b] != c) {
                        ofComp[c].or(ofComp[comp[b]]);
                    }
                }
            }
        }
        BitSet[] reach = new BitSet[n];
        for (int a = 0; a < n; a++) {
            reach[a] = ofComp[comp[a]];
        }
        return reach;
    }

    /**
     * Lakos's cumulative component dependency: the sum over the nodes of the number of nodes
     * each one reaches, counting itself.
     */
    public static long cumulativeDependency(BitSet[] reach) {
        long total = 0;
        for (BitSet r : reach) {
            total += r.cardinality();
        }
        return total;
    }

    /** The out-lists with one edge taken out; the arrays are new, the input is unchanged. */
    public static int[][] without(int[][] out, int from, int to) {
        int[][] copy = out.clone();
        copy[from] = Arrays.stream(out[from]).filter(b -> b != to).toArray();
        return copy;
    }

    private static int[][] membersByComponent(int[] comp, int k) {
        int[] count = new int[k];
        for (int c : comp) {
            count[c]++;
        }
        int[][] members = new int[k][];
        for (int c = 0; c < k; c++) {
            members[c] = new int[count[c]];
        }
        int[] fill = new int[k];
        for (int a = 0; a < comp.length; a++) {
            members[comp[a]][fill[comp[a]]++] = a;
        }
        return members;
    }
}
