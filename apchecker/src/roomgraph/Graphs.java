package roomgraph;

import java.util.ArrayDeque;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Deque;
import java.util.LinkedHashMap;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * Pure graph algorithms over a {@link PlanGraph}. Every method is a function of
 * its arguments alone: nothing here mutates the plan or keeps state between
 * calls. The checks are written against these, so a check reads as a statement
 * about the graph rather than as a traversal.
 */
public final class Graphs {

    private Graphs() {
        // Utility class.
    }

    /**
     * Breadth-first depth of every room reachable from {@code root}, ignoring
     * the rooms in {@code blocked}. Rooms that cannot be reached are simply
     * absent from the result; the caller must decide what that means.
     */
    /**
     * Depths from several roots at once: the distance to the nearest of them.
     * The outdoors may be several vertices - street, driveway, garden - and the
     * depth of a room is its distance from the nearest outdoors, not from an
     * arbitrarily chosen one.
     */
    public static Map<String, Integer> depths(PlanGraph plan, Set<String> roots, Set<String> blocked) {
        Map<String, Integer> depth = new LinkedHashMap<>();
        Deque<String> queue = new ArrayDeque<>();
        for (String root : roots) {
            if (blocked.contains(root)) {
                continue;
            }
            depth.put(root, 0);
            queue.addLast(root);
        }
        while (!queue.isEmpty()) {
            String current = queue.removeFirst();
            int next = depth.get(current) + 1;
            for (String neighbor : plan.neighbors(current)) {
                if (blocked.contains(neighbor) || depth.containsKey(neighbor)) {
                    continue;
                }
                depth.put(neighbor, next);
                queue.addLast(neighbor);
            }
        }
        return Collections.unmodifiableMap(depth);
    }

    public static Map<String, Integer> depths(PlanGraph plan, String root, Set<String> blocked) {
        if (blocked.contains(root)) {
            throw new IllegalArgumentException("The traversal root '" + root + "' cannot itself be blocked.");
        }
        Map<String, Integer> depth = new LinkedHashMap<>();
        Deque<String> queue = new ArrayDeque<>();
        depth.put(root, 0);
        queue.add(root);
        while (!queue.isEmpty()) {
            String current = queue.removeFirst();
            int next = depth.get(current) + 1;
            for (String neighbor : plan.neighbors(current)) {
                if (blocked.contains(neighbor)) {
                    continue;
                }
                if (!depth.containsKey(neighbor)) {
                    depth.put(neighbor, next);
                    queue.addLast(neighbor);
                }
            }
        }
        return Collections.unmodifiableMap(depth);
    }

    /** Every room reachable from {@code root} without entering a blocked room. */
    public static Set<String> reachable(PlanGraph plan, String root, Set<String> blocked) {
        return Collections.unmodifiableSet(new LinkedHashSet<>(depths(plan, root, blocked).keySet()));
    }

    /**
     * One shortest path from {@code from} to {@code to}, as a list of room ids
     * beginning with {@code from} and ending with {@code to}. The list is empty
     * when no path exists. Ties are broken by the declaration order of rooms,
     * so the result is deterministic.
     */
    public static List<String> shortestPath(PlanGraph plan, String from, String to) {
        Map<String, String> parent = new LinkedHashMap<>();
        Set<String> seen = new LinkedHashSet<>();
        Deque<String> queue = new ArrayDeque<>();
        seen.add(from);
        queue.add(from);
        while (!queue.isEmpty()) {
            String current = queue.removeFirst();
            if (current.equals(to)) {
                return rebuildPath(parent, from, to);
            }
            for (String neighbor : plan.neighbors(current)) {
                if (seen.add(neighbor)) {
                    parent.put(neighbor, current);
                    queue.addLast(neighbor);
                }
            }
        }
        return List.of();
    }

    private static List<String> rebuildPath(Map<String, String> parent, String from, String to) {
        List<String> path = new ArrayList<>();
        String current = to;
        while (!current.equals(from)) {
            path.add(current);
            String previous = parent.get(current);
            if (previous == null) {
                throw new IllegalStateException("Broken predecessor chain at '" + current + "'.");
            }
            current = previous;
        }
        path.add(from);
        Collections.reverse(path);
        return List.copyOf(path);
    }

    /**
     * Monotone reachability, which is the operational form of the intimacy
     * gradient: the set of rooms reachable from {@code root} by a walk whose
     * privacy rank never decreases. Whether a walk may continue past a room
     * depends only on that room's rank, not on how the walk arrived, so a plain
     * breadth-first search with the rank test on each edge is sufficient.
     */
    public static Set<String> monotoneReachable(PlanGraph plan, String root) {
        Set<String> seen = new LinkedHashSet<>();
        Deque<String> queue = new ArrayDeque<>();
        seen.add(root);
        queue.add(root);
        while (!queue.isEmpty()) {
            String current = queue.removeFirst();
            PrivacyLevel here = plan.room(current).privacy();
            for (String neighbor : plan.neighbors(current)) {
                PrivacyLevel there = plan.room(neighbor).privacy();
                if (there.atLeastAsPrivateAs(here) && seen.add(neighbor)) {
                    queue.addLast(neighbor);
                }
            }
        }
        return Collections.unmodifiableSet(seen);
    }

    /**
     * The articulation points (cut vertices) of the subgraph induced by all
     * rooms except {@code excluded}. Removing a cut vertex disconnects the
     * subgraph, which is exactly the algebraic statement of "this room is a
     * passage to somewhere". Hopcroft and Tarjan's low-link algorithm; the
     * recursion depth is bounded by the number of rooms, so a house-scale plan
     * is in no danger of overflowing the stack.
     */
    public static Set<String> articulationPoints(PlanGraph plan, Set<String> excluded) {
        Set<String> vertices = new LinkedHashSet<>(plan.roomIds());
        vertices.removeAll(excluded);

        Map<String, Integer> discovery = new LinkedHashMap<>();
        Map<String, Integer> low = new LinkedHashMap<>();
        Set<String> cuts = new LinkedHashSet<>();
        int[] clock = {0};

        for (String vertex : vertices) {
            if (!discovery.containsKey(vertex)) {
                dfsArticulation(plan, vertices, vertex, null, discovery, low, cuts, clock);
            }
        }
        return Collections.unmodifiableSet(cuts);
    }

    private static void dfsArticulation(
            PlanGraph plan,
            Set<String> vertices,
            String current,
            String parent,
            Map<String, Integer> discovery,
            Map<String, Integer> low,
            Set<String> cuts,
            int[] clock) {

        discovery.put(current, clock[0]);
        low.put(current, clock[0]);
        clock[0]++;
        int childCount = 0;

        for (String neighbor : plan.neighbors(current)) {
            if (!vertices.contains(neighbor)) {
                continue;
            }
            if (neighbor.equals(parent)) {
                continue;
            }
            if (discovery.containsKey(neighbor)) {
                low.put(current, Math.min(low.get(current), discovery.get(neighbor)));
                continue;
            }
            childCount++;
            dfsArticulation(plan, vertices, neighbor, current, discovery, low, cuts, clock);
            low.put(current, Math.min(low.get(current), low.get(neighbor)));
            boolean isRoot = (parent == null);
            if (!isRoot && low.get(neighbor) >= discovery.get(current)) {
                cuts.add(current);
            }
        }
        if (parent == null && childCount > 1) {
            cuts.add(current);
        }
    }

    /**
     * True when the subgraph induced by {@code vertices} contains a cycle. For
     * an undirected graph this is decided by counting: a component with as many
     * edges as vertices, or more, must contain a cycle.
     */
    public static boolean inducedSubgraphHasCycle(PlanGraph plan, Set<String> vertices) {
        int edges = 0;
        for (Connection connection : plan.connections()) {
            if (vertices.contains(connection.a()) && vertices.contains(connection.b())) {
                edges++;
            }
        }
        int components = countInducedComponents(plan, vertices);
        // A forest on n vertices with c components has exactly n - c edges.
        return edges > (vertices.size() - components);
    }

    private static int countInducedComponents(PlanGraph plan, Set<String> vertices) {
        Set<String> seen = new LinkedHashSet<>();
        int components = 0;
        for (String start : vertices) {
            if (seen.contains(start)) {
                continue;
            }
            components++;
            Deque<String> stack = new ArrayDeque<>();
            stack.push(start);
            seen.add(start);
            while (!stack.isEmpty()) {
                String current = stack.pop();
                for (String neighbor : plan.neighbors(current)) {
                    if (vertices.contains(neighbor) && seen.add(neighbor)) {
                        stack.push(neighbor);
                    }
                }
            }
        }
        return components;
    }

    /**
     * The least number of connections between any room of {@code from} and any room of
     * {@code to}, walking in the graph with the rooms of {@code avoid} deleted.
     * Returns {@link Double#POSITIVE_INFINITY} when no such walk exists, which
     * includes the case where every source or every target is itself avoided.
     *
     * Infinity is a value here, not an error: several rules turn on the
     * difference between "the trip gets longer" and "the trip becomes
     * impossible", so the caller must be able to see both.
     */
    public static double distance(PlanGraph plan, Set<String> from, Set<String> to, Set<String> avoid) {
        Set<String> sources = new LinkedHashSet<>(from);
        sources.removeAll(avoid);
        Set<String> targets = new LinkedHashSet<>(to);
        targets.removeAll(avoid);
        if (sources.isEmpty() || targets.isEmpty()) {
            return Double.POSITIVE_INFINITY;
        }

        Map<String, Integer> depth = new LinkedHashMap<>();
        Deque<String> queue = new ArrayDeque<>();
        for (String source : sources) {
            depth.put(source, 0);
            queue.addLast(source);
        }
        while (!queue.isEmpty()) {
            String current = queue.removeFirst();
            if (targets.contains(current)) {
                return depth.get(current);
            }
            int next = depth.get(current) + 1;
            for (String neighbor : plan.neighbors(current)) {
                if (avoid.contains(neighbor)) {
                    continue;
                }
                if (!depth.containsKey(neighbor)) {
                    depth.put(neighbor, next);
                    queue.addLast(neighbor);
                }
            }
        }
        return Double.POSITIVE_INFINITY;
    }

    /**
     * The centroid: those rooms of {@code among} that minimize the sum of their
     * distances to the rooms of {@code over}, measured in the graph with
     * {@code avoid} deleted.
     *
     * The result is the whole set of minimizers, not one of them. A tie is a
     * fact about the plan, and a rule that silently broke it would give an
     * answer that depended on the order the rooms happened to be declared in.
     *
     * A candidate that cannot reach every room of {@code over} has an infinite
     * total and is never a minimizer. When no candidate has a finite total, the
     * centroid is empty.
     */
    public static Set<String> centroid(PlanGraph plan, Set<String> among, Set<String> over, Set<String> avoid) {
        Set<String> candidates = new LinkedHashSet<>(among);
        candidates.removeAll(avoid);
        Set<String> targets = new LinkedHashSet<>(over);
        targets.removeAll(avoid);

        double best = Double.POSITIVE_INFINITY;
        Set<String> winners = new LinkedHashSet<>();

        for (String candidate : candidates) {
            Map<String, Integer> depth = depths(plan, candidate, avoid);
            if (!depth.keySet().containsAll(targets)) {
                continue;
            }
            double total = 0.0;
            for (String target : targets) {
                total += depth.get(target);
            }
            if (total < best) {
                best = total;
                winners = new LinkedHashSet<>();
                winners.add(candidate);
            } else if (total == best) {
                winners.add(candidate);
            }
        }
        return Collections.unmodifiableSet(winners);
    }
}
