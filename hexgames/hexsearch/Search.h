// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Search over a shared const Board with per-thread scratch: one bounded multi-source Dijkstra
// primitive and the small family of walks the rulebooks need, as templates over a SearchGraph.
// ----------------------------------------------
#pragma once
#include "hexmodel/Board.h"
#include "hexmodel/Ids.h"

#include <concepts>
#include <cstdint>
#include <functional>
#include <optional>
#include <span>
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
  class SearchScratch {
  public:
    void ensure(std::size_t nodes);

  private:
    template <CostLike C> friend class Field;
    friend struct Algorithms;
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

}  // namespace HexSearch
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
