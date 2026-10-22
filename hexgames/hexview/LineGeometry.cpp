// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Transcribed from hexsheet2svg.py (link_strokes, side_chains, rounded_path). The reference walks
// Python dicts, whose iteration order is insertion order; the graphs here keep that order (nodes
// are numbered as first seen) so the polylines come out in the reference's order.
// ----------------------------------------------
#include "hexview/LineGeometry.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <stdexcept>

namespace HexView {

  namespace {

    // An undirected graph whose nodes are numbered in the order they are first added, and which
    // lists the nodes that have neighbours in the order they were first linked (the reference's
    // nbrs dict).
    template <class Key> class OrderedGraph {
    public:
      std::size_t
      node(const Key& key)
      {
        const auto it = number_.find(key);
        if (number_.end() != it) {
          return it->second;
        }
        const std::size_t n = number_.size();
        number_.emplace(key, n);
        return n;
      }

      // Adds a <-> b once, registering a before b among the linked nodes.
      void
      link(std::size_t a, std::size_t b)
      {
        const std::size_t ia = linked(a);
        const std::size_t ib = linked(b);
        std::vector<std::size_t>& na = nbrs_[ia];
        if (na.end() == std::find(na.begin(), na.end(), b)) {
          na.push_back(b);
          nbrs_[ib].push_back(a);
        }
        return;
      }

      const std::vector<std::size_t>&
      order() const
      {
        return order_;
      }
      const std::vector<std::size_t>&
      nbrs(std::size_t n) const
      {
        return nbrs_[slot_.at(n)];
      }

    private:
      std::size_t
      linked(std::size_t n)
      {
        const auto it = slot_.find(n);
        if (slot_.end() != it) {
          return it->second;
        }
        slot_.emplace(n, order_.size());
        order_.push_back(n);
        nbrs_.emplace_back();
        return order_.size() - 1;
      }

      std::map<Key, std::size_t> number_;
      std::map<std::size_t, std::size_t> slot_;  // node -> position in order_
      std::vector<std::size_t> order_;           // linked nodes, in first-linked order
      std::vector<std::vector<std::size_t>> nbrs_;
    };

    using Edge = std::pair<std::size_t, std::size_t>;

    Edge
    edgeOf(std::size_t a, std::size_t b)
    {
      return a < b ? Edge{a, b} : Edge{b, a};
    }

    Pixel
    midpoint(Pixel a, Pixel b)
    {
      return Pixel{(a.x + b.x) / 2.0, (a.y + b.y) / 2.0};
    }

    // What a walk writes: its first point(s) at the start node, a point per edge taken, and a last
    // point when it stops at an end or junction (nothing when it closes a loop).
    template <class Start, class Step, class Stop> struct WalkPoints {
      Start start;
      Step step;
      Stop stop;
    };

    // Walks from every end or junction, then round every loop left over, as both reference walks
    // do.
    template <class Key, class Start, class Step, class Stop>
    std::vector<Polyline>
    walkAll(const OrderedGraph<Key>& g, const WalkPoints<Start, Step, Stop>& w, bool closeLoopsP)
    {
      std::set<Edge> done;
      const auto walk = [&](std::size_t prev, std::size_t cur, Polyline pts) {
        done.insert(edgeOf(prev, cur));
        w.step(prev, cur, pts);
        while (2 == g.nbrs(cur).size()) {
          const std::vector<std::size_t>& ns = g.nbrs(cur);
          const std::size_t nxt = ns[1] == prev ? ns[0] : ns[1];
          if (done.count(edgeOf(cur, nxt))) {
            return pts;  // back where a closed loop started
          }
          done.insert(edgeOf(cur, nxt));
          w.step(cur, nxt, pts);
          prev = cur;
          cur = nxt;
        }
        w.stop(cur, pts);
        return pts;
      };
      std::vector<Polyline> lines;
      for (const std::size_t n : g.order()) {
        if (2 != g.nbrs(n).size()) {
          for (const std::size_t m : g.nbrs(n)) {
            if (!done.count(edgeOf(n, m))) {
              lines.push_back(walk(n, m, w.start(n, true)));
            }
          }
        }
      }
      for (const std::size_t n : g.order()) {  // loops made only of pass-through nodes
        for (const std::size_t m : g.nbrs(n)) {
          if (!done.count(edgeOf(n, m))) {
            Polyline pts = walk(n, m, w.start(n, false));
            if (closeLoopsP) {
              pts.push_back(pts.front());
            }
            lines.push_back(pts);
          }
        }
      }
      return lines;
    }

    using VertexKey = std::pair<long long, long long>;

    VertexKey
    vertexKey(Pixel p)
    {
      // Corners computed from neighbouring hexes differ by float noise; the odd offset keeps that
      // noise from straddling a bucket boundary.
      return {static_cast<long long>(std::floor(p.x * 2.0 + 0.137)),
              static_cast<long long>(std::floor(p.y * 2.0 + 0.137))};
    }

    bool
    samePointP(Pixel a, Pixel b)
    {
      return a.x == b.x && a.y == b.y;
    }

  }  // namespace

  std::vector<Polyline>
  smoothedLinks(const std::vector<std::vector<HexId>>& chains, const MapFrame& frame)
  {
    std::vector<std::vector<LinkNode>> nodes;
    for (const std::vector<HexId>& chain : chains) {
      std::vector<LinkNode> row;
      for (const HexId& hex : chain) {
        row.push_back(LinkNode{hex, hex.text});
      }
      nodes.push_back(row);
    }
    return smoothedLinks(nodes, frame);
  }

  std::vector<Polyline>
  smoothedLinks(const std::vector<std::vector<LinkNode>>& chains, const MapFrame& frame)
  {
    OrderedGraph<std::string> g;
    std::map<std::size_t, Pixel> centre;
    for (const std::vector<LinkNode>& chain : chains) {
      std::vector<std::size_t> known;
      for (const LinkNode& place : chain) {
        const std::size_t n = g.node(place.key);
        centre[n] = frame.centre(place.hex);
        known.push_back(n);
      }
      for (std::size_t k = 1; k < known.size(); ++k) {
        if (known[k - 1] != known[k]) {
          g.link(known[k - 1], known[k]);
        }
      }
    }
    const auto start = [&](std::size_t n, bool endP) {
      return endP ? Polyline{centre.at(n)} : Polyline{};
    };
    const auto step = [&](std::size_t a, std::size_t b, Polyline& pts) {
      pts.push_back(midpoint(centre.at(a), centre.at(b)));
      return;
    };
    const auto stop = [&](std::size_t n, Polyline& pts) {
      pts.push_back(centre.at(n));
      return;
    };
    return walkAll(
        g, WalkPoints<decltype(start), decltype(step), decltype(stop)>{start, step, stop}, true);
  }

  std::vector<Polyline>
  cornerChains(const std::vector<std::pair<Pixel, Pixel>>& hexsides)
  {
    OrderedGraph<VertexKey> g;
    std::map<std::size_t, Pixel> point;  // the first pixel seen for each corner
    for (const auto& [a, b] : hexsides) {
      const VertexKey ka = vertexKey(a);
      const VertexKey kb = vertexKey(b);
      if (ka == kb) {
        continue;
      }
      const std::size_t na = g.node(ka);
      const std::size_t nb = g.node(kb);
      point.emplace(na, a);
      point.emplace(nb, b);
      g.link(na, nb);
    }
    // A loop's walk ends by stepping back onto its first corner, which the chain then repeats.
    const auto start = [&](std::size_t n, bool) { return Polyline{point.at(n)}; };
    const auto step = [&](std::size_t, std::size_t b, Polyline& pts) {
      pts.push_back(point.at(b));
      return;
    };
    const auto stop = [](std::size_t, Polyline&) { return; };
    return walkAll(
        g, WalkPoints<decltype(start), decltype(step), decltype(stop)>{start, step, stop}, false);
  }

  std::vector<PathCommand>
  roundedCorners(const Polyline& pts)
  {
    if (pts.empty()) {
      throw std::invalid_argument("roundedCorners: empty chain");
    }
    std::vector<PathCommand> out;
    if (pts.size() > 3 && samePointP(pts.front(), pts.back())) {
      const Polyline ring(pts.begin(), pts.end() - 1);
      out.push_back(MoveTo{midpoint(ring.back(), ring.front())});
      for (std::size_t i = 0; i < ring.size(); ++i) {
        out.push_back(QuadTo{ring[i], midpoint(ring[i], ring[(i + 1) % ring.size()])});
      }
      return out;
    }
    if (pts.size() < 3) {
      return {MoveTo{pts.front()}, LineTo{pts.back()}};
    }
    out.push_back(MoveTo{pts.front()});
    out.push_back(LineTo{midpoint(pts[0], pts[1])});
    for (std::size_t i = 1; i + 1 < pts.size(); ++i) {
      out.push_back(QuadTo{pts[i], midpoint(pts[i], pts[i + 1])});
    }
    out.push_back(LineTo{pts.back()});
    return out;
  }

  std::vector<PathCommand>
  straightChain(const Polyline& chain)
  {
    if (chain.empty()) {
      throw std::invalid_argument("straightChain: empty chain");
    }
    std::vector<PathCommand> out{MoveTo{chain.front()}};
    for (std::size_t i = 1; i < chain.size(); ++i) {
      out.push_back(LineTo{chain[i]});
    }
    return out;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
