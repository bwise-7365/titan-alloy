// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The line layers: hexsheet2svg.py layer_edges (paths along hexsides, straight edges, rounded
// rivers) and layer_links (smoothed road and railway networks, casings, rail ticks).
// ----------------------------------------------
#include "hexview/MapLayers.h"
#include "hexview/Scratch.h"

#include <cmath>
#include <functional>
#include <stdexcept>

namespace HexView {

  namespace {

    bool
    nearP(Pixel p, Pixel q)
    {
      return std::fabs(p.x - q.x) < 0.5 && std::fabs(p.y - q.y) < 0.5;
    }

    // SplitMix64 over a chain's identity: the same chain gets the same wobble on every redraw.
    std::uint64_t
    mixSeed(std::uint64_t index)
    {
      std::uint64_t z = (index + 0x9E3779B97F4A7C15ull) * 0xBF58476D1CE4E5B9ull;
      z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
      return z ^ (z >> 31);
    }

    // The style's scratch applied to one polyline, seeded by `name` and its ordinal in the group.
    Polyline
    scratched(const MapContext& ctx, const Polyline& p, const std::string& name, std::size_t ordinal)
    {
      if (!ctx.style.scratch.has_value() || p.size() < 2) {
        return p;
      }
      const ScratchSpec spec{ctx.style.scratch->roughness, ctx.style.scratch->smoothing,
                             ctx.frame.grids().front().spec().size, 10,
                             mixSeed(std::hash<std::string>{}(name) + ordinal)};
      return scratch(p, spec);
    }

    bool
    scratchedKindP(const std::string& kind)
    {
      return std::string::npos != kind.find("road") || std::string::npos != kind.find("rail");
    }

    // chain_path: consecutive hexsides joined into polylines, broken where they do not touch, each
    // side moved `offset` pixels toward its hex's centre.
    Commands
    chainPath(const MapContext& ctx, const std::vector<std::string>& edges, double offset)
    {
      std::vector<Polyline> parts;
      Polyline cur;
      std::optional<Pixel> last;
      for (const std::string& token : edges) {
        const EdgeRef e = ctx.frame.edgeRef(token);
        auto [a, b] = ctx.frame.hexsideEnds(e);
        if (0.0 != offset) {
          const Pixel c = ctx.frame.centre(e.hex);
          const Pixel m = ctx.frame.hexsideMidpoint(e);
          const double len = std::hypot(c.x - m.x, c.y - m.y);
          const double ux = (c.x - m.x) / len;
          const double uy = (c.y - m.y) / len;
          a = Pixel{a.x + ux * offset, a.y + uy * offset};
          b = Pixel{b.x + ux * offset, b.y + uy * offset};
        }
        if (!last.has_value()) {
          cur = {a, b};
        }
        else if (nearP(*last, a)) {
          cur.push_back(b);
        }
        else if (nearP(*last, b)) {
          cur.push_back(a);
        }
        else if (2 == cur.size() && (nearP(cur[0], a) || nearP(cur[0], b))) {
          // the previous side was taken the wrong way round
          cur = {cur[1], cur[0]};
          cur.push_back(nearP(cur.back(), a) ? b : a);
        }
        else {
          parts.push_back(cur);
          cur = {a, b};
        }
        last = cur.back();
      }
      if (!cur.empty()) {
        parts.push_back(cur);
      }
      Commands out;
      for (const Polyline& p : parts) {
        if (p.size() >= 2) {
          const Commands piece = straightChain(p);
          out.insert(out.end(), piece.begin(), piece.end());
        }
      }
      return out;
    }

    void
    addStroked(Scene& scene, Layer layer, const Commands& d, const HexXml::SheetLineDoc& l,
               const MapContext& ctx, const std::string& at, HitTag hit)
    {
      const std::optional<Stroke> casing = ctx.casingStroke(l, at);
      if (casing.has_value()) {
        scene.add(layer, pathPrimitive(d, std::nullopt, casing));
      }
      scene.add(layer, pathPrimitive(d, std::nullopt, ctx.lineStroke(l, at), std::move(hit)));
      return;
    }

    void
    addPaths(const MapContext& ctx, Scene& scene)
    {
      for (const HexXml::SheetPathDoc& p : ctx.sheet.paths) {
        const std::string at =
            where(ctx.sheet, p.sourceLine, "path '" + p.name.value_or(p.kind) + "'");
        const HexXml::SheetLineDoc& l = ctx.line(p.line, at);
        addStroked(scene, Layer::Edges, chainPath(ctx, p.edges, p.offset), l, ctx, at, NoHit{});
      }
      return;
    }

  }  // namespace

  void
  addEdgesLayer(const MapContext& ctx, Scene& scene)
  {
    addPaths(ctx, scene);
    std::vector<std::string> roundedOrder;  // line ids in first-seen order
    std::map<std::string, std::vector<std::pair<Pixel, Pixel>>> rounded;
    for (const HexXml::SheetEdgeDoc& e : ctx.sheet.edges) {
      if (!e.line.has_value()) {
        continue;
      }
      const std::string at = where(ctx.sheet, e.sourceLine, "edge '" + e.at + "'");
      const EdgeRef ref = ctx.frame.edgeRef(e.at);
      const auto ends = ctx.frame.hexsideEnds(ref);
      if (ctx.roundedP(*e.line)) {
        if (!rounded.contains(*e.line)) {
          roundedOrder.push_back(*e.line);
        }
        rounded[*e.line].push_back(ends);
        continue;
      }
      const Stroke s = ctx.lineStroke(ctx.line(*e.line, at), at);
      scene.add(Layer::Edges, pathPrimitive({MoveTo{ends.first}, LineTo{ends.second}}, std::nullopt,
                                            s, HexsideHit{ctx.frame.index(ref.hex), ref.dir}));
    }
    for (const std::string& id : roundedOrder) {
      const std::string at = "sheet '" + ctx.sheet.id + "': edges of line '" + id + "'";
      Commands d;
      std::size_t ordinal = 0;
      for (const Polyline& chain : cornerChains(rounded.at(id))) {
        // rounded as the reference draws rivers, or scratched instead when the style asks
        const Commands piece = ctx.style.scratch.has_value()
                                   ? straightChain(scratched(ctx, chain, id, ordinal++))
                                   : roundedCorners(chain);
        d.insert(d.end(), piece.begin(), piece.end());
      }
      scene.add(Layer::Edges,
                pathPrimitive(std::move(d), std::nullopt, ctx.lineStroke(ctx.line(id, at), at)));
    }
    return;
  }

  namespace {

    struct LinkGroup {
      std::string kind;
      std::string line;
      std::vector<std::vector<LinkNode>> chains;
      std::string where;
    };

    std::vector<LinkGroup>
    linkGroups(const MapContext& ctx)
    {
      const bool explicitP = "explicit" == ctx.sheet.junctions;
      std::map<std::pair<std::string, std::string>, std::size_t> junctionOf;  // (hex, link id) -> n
      for (std::size_t n = 0; n < ctx.sheet.junctionElements.size(); ++n) {
        for (const std::string& lid : ctx.sheet.junctionElements[n].links) {
          junctionOf[{ctx.sheet.junctionElements[n].at, lid}] = n;
        }
      }
      std::vector<LinkGroup> groups;
      std::map<std::string, std::size_t> groupOf;
      for (std::size_t i = 0; i < ctx.sheet.links.size(); ++i) {
        const HexXml::SheetLinkDoc& lk = ctx.sheet.links[i];
        const std::string key = lk.kind + '\x1f' + lk.line + '\x1f' + lk.owner.value_or("") +
                                '\x1f' + (explicitP ? "" : lk.name.value_or(""));
        if (!groupOf.contains(key)) {
          groupOf[key] = groups.size();
          groups.push_back(LinkGroup{
              lk.kind,
              lk.line,
              {},
              where(ctx.sheet, lk.sourceLine, "link '" + lk.name.value_or(lk.kind) + "'")});
        }
        const std::string chainKey = lk.id.value_or("link" + std::to_string(i));
        std::vector<LinkNode> chain;
        for (const std::string& hex : lk.hexes) {
          std::string node = hex;
          if (explicitP) {
            const auto it = junctionOf.find({hex, chainKey});
            node = junctionOf.end() != it ? "J\x1f" + hex + '\x1f' + std::to_string(it->second)
                                          : "C\x1f" + chainKey + '\x1f' + hex;
          }
          chain.push_back(LinkNode{HexId{hex}, node});
        }
        groups[groupOf[key]].chains.push_back(chain);
      }
      return groups;
    }

    // With smoothLinksP off, each chain is drawn centre to centre.
    std::vector<Polyline>
    centreLines(const MapContext& ctx, const std::vector<std::vector<LinkNode>>& chains)
    {
      std::vector<Polyline> out;
      for (const std::vector<LinkNode>& chain : chains) {
        Polyline p;
        for (const LinkNode& n : chain) {
          p.push_back(ctx.frame.centre(n.hex));
        }
        out.push_back(p);
      }
      return out;
    }

  }  // namespace

  void
  addLinksLayer(const MapContext& ctx, Scene& scene)
  {
    for (const LinkGroup& g : linkGroups(ctx)) {
      const HexXml::SheetLineDoc& l = ctx.line(g.line, g.where);
      Commands d;
      const std::vector<Polyline> parts =
          ctx.style.smoothLinksP ? smoothedLinks(g.chains, ctx.frame) : centreLines(ctx, g.chains);
      std::size_t ordinal = 0;
      for (const Polyline& p : parts) {
        if (p.size() >= 2) {
          const Commands piece = straightChain(
              scratchedKindP(g.kind) ? scratched(ctx, p, g.kind + g.line + g.where, ordinal++) : p);
          d.insert(d.end(), piece.begin(), piece.end());
        }
      }
      if (d.empty()) {
        continue;
      }
      addStroked(scene, Layer::Links, d, l, ctx, g.where, NoHit{});
      if (l.ticks) {
        const Stroke ticks{ctx.color(l.stroke, g.where),
                           l.width * 3,
                           {l.width * 0.9, l.width * 2.6},
                           1.0,
                           LineCap::Butt,
                           LineJoin::Miter};
        scene.add(Layer::Links, pathPrimitive(d, std::nullopt, ticks));
      }
    }
    return;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
