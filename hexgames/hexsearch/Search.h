// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Search over a shared const Board with per-thread scratch: one bounded multi-source Dijkstra
// primitive and the small family of walks the rulebooks need, as templates over a SearchGraph.
// ----------------------------------------------
#pragma once
#include "hexmodel/Board.h"
#include "hexmodel/Ids.h"

#include <algorithm>
#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <queue>
#include <span>
#include <stdexcept>
#include <string>
#include <vector>

namespace HexSearch {

  using NodeIndex = std::uint32_t;

  // A graph the algorithms can walk: dense node indices and an arc visitor.
  template <class G>
  concept SearchGraph = requires(const G& g, NodeIndex n, std::function<void(NodeIndex, std::size_t)> visit) {
    { g.nodeCount() } -> std::convertible_to<std::size_t>;
    g.forEachArc(n, visit);  // visit(to, arcIndex)
  };

  // A cost that can be summed and compared, with a zero.
  template <class C>
  concept CostLike = std::totally_ordered<C> && requires(C a, C b) {
    { a + b } -> std::same_as<C>;
    { C::zero() } -> std::same_as<C>;
  };

  struct CostHalves {
    int halves = 0;
    constexpr auto operator<=>(const CostHalves&) const = default;
    friend constexpr CostHalves operator+(CostHalves l, CostHalves r) { return {l.halves + r.halves}; }
    static constexpr CostHalves zero() { return {0}; }
  };

  // Per-Session working memory. Generation-stamped so that clearing between searches is O(1); every
  // Field view remembers the generation it was made in and throws if read after a later search.
  template <CostLike C> class Field;

  class SearchScratch {
  public:
    void ensure(std::size_t nodes);

  private:
    template <CostLike C> friend class Field;
    friend struct Algorithms;

    // Added in M4: the two lines every algorithm shares. begin() sizes the scratch, stamps a fresh
    // generation (so every Field of an earlier search now throws) and empties the settled list;
    // fieldOf() hands back the view of what the search just wrote.
    std::uint32_t begin(std::size_t nodes);
    template <CostLike C> Field<C> fieldOf(std::uint32_t generation) const;

    std::uint32_t generation_ = 0;
    std::vector<std::uint32_t> stamp_;
    std::vector<CostHalves> dist_;
    std::vector<NodeIndex> origin_;
    std::vector<NodeIndex> prev_;
    std::vector<NodeIndex> settled_;
  };

  // The result of one search: reached set, distances, origins and paths, read out of the scratch.
  template <CostLike C>
  class Field {
  public:
    bool reachedP(NodeIndex) const;
    C distance(NodeIndex) const;         // throws if not reached
    NodeIndex origin(NodeIndex) const;   // which source reached it
    std::vector<NodeIndex> pathTo(NodeIndex) const;
    std::span<const NodeIndex> reached() const;  // settlement order

  private:
    friend struct Algorithms;
    friend class SearchScratch;
    // Added in M4: throws unless the scratch still holds this search's generation.
    void checkGeneration() const;

    const SearchScratch* scratch_ = nullptr;
    std::uint32_t generation_ = 0;
  };

  // Source with an individual starting budget (a unit's remaining allowance).
  struct Source {
    NodeIndex node;
    CostHalves spent = CostHalves::zero();
  };

  // Every algorithm takes the graph, the scratch, and predicates; none holds state.
  struct Algorithms {
    // The one primitive. arcCost(from, arcIndex) -> nullopt when the arc may not be taken; ceiling
    // bounds the search; terminalP(node) marks nodes that are settled but never expanded (a "must
    // stop" hex).
    template <SearchGraph G>
    static Field<CostHalves> dijkstraBounded(
        const G&, SearchScratch&, std::span<const Source>,
        const std::function<std::optional<CostHalves>(NodeIndex, std::size_t)>& arcCost,
        CostHalves ceiling, const std::function<bool(NodeIndex)>& terminalP);

    template <SearchGraph G>
    static std::optional<std::vector<NodeIndex>> aStar(
        const G&, SearchScratch&, NodeIndex from, NodeIndex to,
        const std::function<std::optional<CostHalves>(NodeIndex, std::size_t)>& arcCost,
        const std::function<CostHalves(NodeIndex)>& heuristic);

    // Unit-cost flood from several sources through allowed arcs, to a depth.
    template <SearchGraph G>
    static Field<CostHalves> bfsFlood(const G&, SearchScratch&, std::span<const NodeIndex> sources,
                                      const std::function<bool(NodeIndex, std::size_t)>& arcAllowedP,
                                      int maxDepth);

    // Connected components over allowed arcs: component index per node.
    template <SearchGraph G>
    static std::vector<std::uint32_t> components(const G&, SearchScratch&,
                                                  const std::function<bool(NodeIndex, std::size_t)>& arcAllowedP);

    // How many of the targets are reachable from `from` without passing a blocked node; stops as
    // soon as `threshold` are found (Tarawa's communication needs two).
    template <SearchGraph G>
    static int reachCount(const G&, SearchScratch&, NodeIndex from, std::span<const NodeIndex> targets,
                          const std::function<bool(NodeIndex)>& blockedP, int threshold);
  };

  // Walks that are specific to the hex lattice rather than to a graph.
  // Each step strictly farther from `origin` (Dai Senso retreat); returns the reachable end hexes.
  std::vector<HexModel::HexIndex> monotoneWalk(const HexModel::Board&, HexModel::HexIndex origin,
                                               HexModel::HexIndex from, int steps,
                                               const std::function<bool(HexModel::HexIndex)>& allowedP);
  // Flood fill bounded by wall hexsides (Tarawa's fire arcs, layers bounded by an edge terrain).
  std::vector<HexModel::HexIndex> regionFlood(const HexModel::Board&, HexModel::HexIndex seed,
                                              const std::function<bool(HexModel::HexIndex, HexModel::Direction)>& wallP);

  // Graph adaptors over the Board.
  class HexAdjacencyGraph {
  public:
    using EdgePredicate = std::function<bool(HexModel::HexIndex, HexModel::Direction)>;
    HexAdjacencyGraph(const HexModel::Board&, EdgePredicate crossableP);
    std::size_t nodeCount() const;
    void forEachArc(NodeIndex, const std::function<void(NodeIndex, std::size_t)>&) const;  // arcIndex = direction
  private:
    const HexModel::Board& board_;
    EdgePredicate crossableP_;
  };

  class NetworkGraph {
  public:
    using LinkPredicate = std::function<bool(std::size_t link)>;
    NetworkGraph(const HexModel::Board&, HexModel::NetworkId, LinkPredicate usableP);
    std::size_t nodeCount() const;
    void forEachArc(NodeIndex, const std::function<void(NodeIndex, std::size_t)>&) const;  // arcIndex = link
  private:
    const HexModel::Board& board_;
    HexModel::NetworkId network_;
    LinkPredicate usableP_;
  };

  // ---- definitions of the templates declared above ---------------------------------------------
  // The algorithms are templates over the graph, so they live here; SearchScratch, the two Board
  // adaptors and the two lattice walks are ordinary functions and live in Search.cpp.

  namespace Detail {

    // One priority-queue entry. The node breaks ties so that two runs over the same graph settle
    // nodes in the same order: every field read downstream of a search may reach a PRNG.
    struct Frontier {
      CostHalves cost;
      NodeIndex node = 0;
    };

    struct FarthestFirst {
      bool
      operator()(const Frontier& l, const Frontier& r) const
      {
        if (l.cost != r.cost) {
          return r.cost < l.cost;
        }
        return r.node < l.node;
      }
    };

    using Queue = std::priority_queue<Frontier, std::vector<Frontier>, FarthestFirst>;

    // The visitor type the SearchGraph concept names; the algorithms build one per expansion.
    using ArcVisitor = std::function<void(NodeIndex, std::size_t)>;

  }  // namespace Detail

  template <CostLike C>
  Field<C>
  SearchScratch::fieldOf(std::uint32_t generation) const
  {
    Field<C> field;
    field.scratch_ = this;
    field.generation_ = generation;
    return field;
  }

  template <CostLike C>
  void
  Field<C>::checkGeneration() const
  {
    if (nullptr == scratch_) {
      throw std::invalid_argument("HexSearch::Field: read before any search produced it");
    }
    if (generation_ != scratch_->generation_) {
      throw std::invalid_argument("HexSearch::Field: read after a later search reused the scratch");
    }
    return;
  }

  template <CostLike C>
  bool
  Field<C>::reachedP(NodeIndex n) const
  {
    checkGeneration();
    return n < scratch_->stamp_.size() && scratch_->stamp_[n] == generation_;
  }

  template <CostLike C>
  C
  Field<C>::distance(NodeIndex n) const
  {
    if (!reachedP(n)) {
      throw std::invalid_argument("HexSearch::Field::distance: node " + std::to_string(n) +
                                   " was not reached");
    }
    return scratch_->dist_[n];
  }

  template <CostLike C>
  NodeIndex
  Field<C>::origin(NodeIndex n) const
  {
    if (!reachedP(n)) {
      throw std::invalid_argument("HexSearch::Field::origin: node " + std::to_string(n) +
                                   " was not reached");
    }
    return scratch_->origin_[n];
  }

  template <CostLike C>
  std::vector<NodeIndex>
  Field<C>::pathTo(NodeIndex n) const
  {
    if (!reachedP(n)) {
      throw std::invalid_argument("HexSearch::Field::pathTo: node " + std::to_string(n) +
                                   " was not reached");
    }
    std::vector<NodeIndex> path;
    NodeIndex here = n;
    path.push_back(here);
    while (scratch_->prev_[here] != here) {
      here = scratch_->prev_[here];
      path.push_back(here);
    }
    std::reverse(path.begin(), path.end());
    return path;
  }

  template <CostLike C>
  std::span<const NodeIndex>
  Field<C>::reached() const
  {
    checkGeneration();
    return std::span<const NodeIndex>(scratch_->settled_);
  }

  template <SearchGraph G>
  Field<CostHalves>
  Algorithms::dijkstraBounded(const G& graph, SearchScratch& scratch, std::span<const Source> sources,
                               const std::function<std::optional<CostHalves>(NodeIndex, std::size_t)>& arcCost,
                               CostHalves ceiling, const std::function<bool(NodeIndex)>& terminalP)
  {
    const std::uint32_t generation = scratch.begin(graph.nodeCount());

    Detail::Queue queue;
    for (const Source& source : sources) {
      if (source.node >= scratch.stamp_.size() || ceiling < source.spent) {
        continue;
      }
      const bool fresh = scratch.stamp_[source.node] != generation;
      if (fresh || source.spent < scratch.dist_[source.node]) {
        scratch.stamp_[source.node] = generation;
        scratch.dist_[source.node] = source.spent;
        scratch.origin_[source.node] = source.node;
        scratch.prev_[source.node] = source.node;
        queue.push(Detail::Frontier{source.spent, source.node});
      }
    }

    while (!queue.empty()) {
      const Detail::Frontier top = queue.top();
      queue.pop();
      if (scratch.dist_[top.node] < top.cost) {
        continue;  // a stale entry: this node settled at a smaller cost already
      }
      scratch.settled_.push_back(top.node);
      if (terminalP && terminalP(top.node)) {
        continue;  // settled, never expanded: a hex the rules make a unit stop in
      }
      const Detail::ArcVisitor visit = [&](NodeIndex to, std::size_t arcIndex) {
        const std::optional<CostHalves> step = arcCost(top.node, arcIndex);
        if (!step) {
          return;
        }
        const CostHalves reached = top.cost + *step;
        if (ceiling < reached) {
          return;
        }
        const bool fresh = scratch.stamp_[to] != generation;
        if (fresh || reached < scratch.dist_[to]) {
          scratch.stamp_[to] = generation;
          scratch.dist_[to] = reached;
          scratch.origin_[to] = scratch.origin_[top.node];
          scratch.prev_[to] = top.node;
          queue.push(Detail::Frontier{reached, to});
        }
        return;
      };
      graph.forEachArc(top.node, visit);
    }

    return scratch.fieldOf<CostHalves>(generation);
  }

  template <SearchGraph G>
  std::optional<std::vector<NodeIndex>>
  Algorithms::aStar(const G& graph, SearchScratch& scratch, NodeIndex from, NodeIndex to,
                     const std::function<std::optional<CostHalves>(NodeIndex, std::size_t)>& arcCost,
                     const std::function<CostHalves(NodeIndex)>& heuristic)
  {
    const std::uint32_t generation = scratch.begin(graph.nodeCount());
    if (from >= scratch.stamp_.size() || to >= scratch.stamp_.size()) {
      throw std::invalid_argument("HexSearch::aStar: endpoint outside the graph");
    }

    scratch.stamp_[from] = generation;
    scratch.dist_[from] = CostHalves::zero();
    scratch.origin_[from] = from;
    scratch.prev_[from] = from;

    Detail::Queue queue;
    queue.push(Detail::Frontier{heuristic(from), from});
    while (!queue.empty()) {
      const Detail::Frontier top = queue.top();
      queue.pop();
      const CostHalves here = scratch.dist_[top.node];
      if (top.cost < here + heuristic(top.node)) {
        continue;  // stale: this node has since been reached more cheaply
      }
      if (top.node == to) {
        return scratch.fieldOf<CostHalves>(generation).pathTo(to);
      }
      scratch.settled_.push_back(top.node);
      const Detail::ArcVisitor visit = [&](NodeIndex next, std::size_t arcIndex) {
        const std::optional<CostHalves> step = arcCost(top.node, arcIndex);
        if (!step) {
          return;
        }
        const CostHalves reached = here + *step;
        const bool fresh = scratch.stamp_[next] != generation;
        if (fresh || reached < scratch.dist_[next]) {
          scratch.stamp_[next] = generation;
          scratch.dist_[next] = reached;
          scratch.origin_[next] = from;
          scratch.prev_[next] = top.node;
          queue.push(Detail::Frontier{reached + heuristic(next), next});
        }
        return;
      };
      graph.forEachArc(top.node, visit);
    }
    return std::nullopt;
  }

  template <SearchGraph G>
  Field<CostHalves>
  Algorithms::bfsFlood(const G& graph, SearchScratch& scratch, std::span<const NodeIndex> sources,
                        const std::function<bool(NodeIndex, std::size_t)>& arcAllowedP, int maxDepth)
  {
    const std::uint32_t generation = scratch.begin(graph.nodeCount());

    std::vector<NodeIndex> frontier;
    for (NodeIndex source : sources) {
      if (source >= scratch.stamp_.size() || scratch.stamp_[source] == generation) {
        continue;
      }
      scratch.stamp_[source] = generation;
      scratch.dist_[source] = CostHalves::zero();
      scratch.origin_[source] = source;
      scratch.prev_[source] = source;
      scratch.settled_.push_back(source);
      frontier.push_back(source);
    }

    for (int depth = 1; depth <= maxDepth && !frontier.empty(); ++depth) {
      std::vector<NodeIndex> next;
      for (NodeIndex node : frontier) {
        const Detail::ArcVisitor visit = [&](NodeIndex to, std::size_t arcIndex) {
          if (!arcAllowedP(node, arcIndex) || scratch.stamp_[to] == generation) {
            return;
          }
          scratch.stamp_[to] = generation;
          scratch.dist_[to] = CostHalves{depth};
          scratch.origin_[to] = scratch.origin_[node];
          scratch.prev_[to] = node;
          scratch.settled_.push_back(to);
          next.push_back(to);
          return;
        };
        graph.forEachArc(node, visit);
      }
      frontier = std::move(next);
    }

    return scratch.fieldOf<CostHalves>(generation);
  }

  template <SearchGraph G>
  std::vector<std::uint32_t>
  Algorithms::components(const G& graph, SearchScratch& scratch,
                          const std::function<bool(NodeIndex, std::size_t)>& arcAllowedP)
  {
    const std::uint32_t generation = scratch.begin(graph.nodeCount());
    const std::size_t nodes = graph.nodeCount();
    std::vector<std::uint32_t> component(nodes, 0);

    std::uint32_t next = 0;
    for (std::size_t seed = 0; seed < nodes; ++seed) {
      const NodeIndex start = static_cast<NodeIndex>(seed);
      if (scratch.stamp_[start] == generation) {
        continue;
      }
      const std::uint32_t label = next++;
      scratch.stamp_[start] = generation;
      component[start] = label;
      std::vector<NodeIndex> stack{start};
      while (!stack.empty()) {
        const NodeIndex node = stack.back();
        stack.pop_back();
        const Detail::ArcVisitor visit = [&](NodeIndex to, std::size_t arcIndex) {
          if (!arcAllowedP(node, arcIndex) || scratch.stamp_[to] == generation) {
            return;
          }
          scratch.stamp_[to] = generation;
          component[to] = label;
          stack.push_back(to);
          return;
        };
        graph.forEachArc(node, visit);
      }
    }
    return component;
  }

  template <SearchGraph G>
  int
  Algorithms::reachCount(const G& graph, SearchScratch& scratch, NodeIndex from,
                          std::span<const NodeIndex> targets, const std::function<bool(NodeIndex)>& blockedP,
                          int threshold)
  {
    const std::uint32_t generation = scratch.begin(graph.nodeCount());
    if (from >= scratch.stamp_.size()) {
      throw std::invalid_argument("HexSearch::reachCount: source outside the graph");
    }
    if (blockedP(from)) {
      return 0;
    }

    int found = 0;
    const auto countIfTarget = [&](NodeIndex node) {
      for (NodeIndex target : targets) {
        if (target == node) {
          ++found;
          return;
        }
      }
      return;
    };

    scratch.stamp_[from] = generation;
    scratch.settled_.push_back(from);
    countIfTarget(from);
    std::vector<NodeIndex> frontier{from};
    while (!frontier.empty() && found < threshold) {
      std::vector<NodeIndex> next;
      for (NodeIndex node : frontier) {
        const Detail::ArcVisitor visit = [&](NodeIndex to, std::size_t) {
          if (scratch.stamp_[to] == generation || blockedP(to)) {
            return;
          }
          scratch.stamp_[to] = generation;
          scratch.settled_.push_back(to);
          countIfTarget(to);
          next.push_back(to);
          return;
        };
        graph.forEachArc(node, visit);
        if (threshold <= found) {
          break;
        }
      }
      frontier = std::move(next);
    }
    return found;
  }

}  // namespace HexSearch
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
