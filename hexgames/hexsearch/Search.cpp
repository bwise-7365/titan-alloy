// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The non-template half of hexsearch: the scratch's own bookkeeping, the two walks that are
// specific to the hex lattice rather than to a graph, and the two Board adaptors.
// ----------------------------------------------
#include "hexsearch/Search.h"

#include <algorithm>
#include <stdexcept>

namespace HexSearch {

  using HexModel::Board;
  using HexModel::Direction;
  using HexModel::HexIndex;

  void
  SearchScratch::ensure(std::size_t nodes)
  {
    if (stamp_.size() < nodes) {
      stamp_.resize(nodes, 0);
      dist_.resize(nodes, CostHalves::zero());
      origin_.resize(nodes, 0);
      prev_.resize(nodes, 0);
    }
    return;
  }

  std::uint32_t
  SearchScratch::begin(std::size_t nodes)
  {
    ensure(nodes);
    ++generation_;
    settled_.clear();
    return generation_;
  }

  std::vector<HexIndex>
  monotoneWalk(const Board& board, HexIndex origin, HexIndex from, int steps,
                const std::function<bool(HexIndex)>& allowedP)
  {
    if (0 > steps) {
      throw std::invalid_argument("HexSearch::monotoneWalk: a negative number of steps");
    }
    std::vector<HexIndex> layer{from};
    for (int step = 0; step < steps; ++step) {
      std::vector<HexIndex> next;
      for (HexIndex here : layer) {
        const int hereDistance = board.distance(origin, here);
        for (int d = 0; d < HexCoord::kDirections; ++d) {
          const std::optional<HexIndex> neighbour = board.neighbour(here, static_cast<Direction>(d));
          if (!neighbour) {
            continue;
          }
          if (board.distance(origin, *neighbour) <= hereDistance) {
            continue;  // every step is strictly farther from the origin
          }
          if (!allowedP(*neighbour)) {
            continue;
          }
          if (next.end() == std::find(next.begin(), next.end(), *neighbour)) {
            next.push_back(*neighbour);
          }
        }
      }
      layer = std::move(next);
    }
    return layer;
  }

  std::vector<HexIndex>
  regionFlood(const Board& board, HexIndex seed, const std::function<bool(HexIndex, Direction)>& wallP)
  {
    std::vector<bool> seen(board.hexCount(), false);
    if (seed.value >= board.hexCount()) {
      throw std::invalid_argument("HexSearch::regionFlood: seed outside the board");
    }
    seen[seed.value] = true;
    std::vector<HexIndex> reached{seed};
    for (std::size_t i = 0; i < reached.size(); ++i) {
      const HexIndex here = reached[i];
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const Direction direction = static_cast<Direction>(d);
        if (wallP(here, direction)) {
          continue;
        }
        const std::optional<HexIndex> neighbour = board.neighbour(here, direction);
        if (!neighbour || seen[neighbour->value]) {
          continue;
        }
        seen[neighbour->value] = true;
        reached.push_back(*neighbour);
      }
    }
    return reached;
  }

  HexAdjacencyGraph::HexAdjacencyGraph(const Board& board, EdgePredicate crossableP)
    : board_(board), crossableP_(std::move(crossableP))
  {
  }

  std::size_t
  HexAdjacencyGraph::nodeCount() const
  {
    return board_.hexCount();
  }

  void
  HexAdjacencyGraph::forEachArc(NodeIndex node, const std::function<void(NodeIndex, std::size_t)>& visit) const
  {
    const HexIndex here{node};
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const Direction direction = static_cast<Direction>(d);
      const std::optional<HexIndex> neighbour = board_.neighbour(here, direction);
      if (!neighbour) {
        continue;
      }
      if (crossableP_ && !crossableP_(here, direction)) {
        continue;
      }
      visit(neighbour->value, static_cast<std::size_t>(d));
    }
    return;
  }

  NetworkGraph::NetworkGraph(const Board& board, HexModel::NetworkId network, LinkPredicate usableP)
    : board_(board), network_(network), usableP_(std::move(usableP))
  {
  }

  std::size_t
  NetworkGraph::nodeCount() const
  {
    return board_.hexCount();
  }

  void
  NetworkGraph::forEachArc(NodeIndex node, const std::function<void(NodeIndex, std::size_t)>& visit) const
  {
    const HexModel::LinkNetwork& network = board_.network(network_);
    for (std::size_t link : network.linksAt(HexIndex{node})) {
      if (usableP_ && !usableP_(link)) {
        continue;
      }
      const HexModel::LinkNetwork::Link& arc = network.links()[link];
      const HexIndex other = arc.a.value == node ? arc.b : arc.a;
      visit(other.value, link);
    }
    return;
  }

}  // namespace HexSearch
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
