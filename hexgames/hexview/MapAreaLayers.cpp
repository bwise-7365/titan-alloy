// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The area layers: background, terrain (fill, pattern, urban tint), regions (tint, outline), grid
// and rings; hexsheet2svg.py render()'s first rect, layer_terrain, layer_regions, layer_grid and
// layer_rings.
// ----------------------------------------------
#include "hexview/MapLayers.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace HexView {

  namespace {

    constexpr std::string_view kUrbanSymbols[] = {"city-major", "city", "capital"};
    constexpr Color kUrbanTint{0xe3, 0xe3, 0xe3, 255};  // panj/tempest's paleGray

    bool
    urbanSymbolP(const std::optional<std::string>& symbol)
    {
      return symbol.has_value() && std::find(std::begin(kUrbanSymbols), std::end(kUrbanSymbols),
                                             *symbol) != std::end(kUrbanSymbols);
    }

    std::optional<Pattern>
    patternOf(const HexXml::SheetTerrainDoc& t)
    {
      if ("none" == t.pattern) {
        return std::nullopt;
      }
      if ("dots" == t.pattern) {
        return Pattern::Dots;
      }
      if ("hatch" == t.pattern) {
        return Pattern::Hatch;
      }
      if ("mottle" == t.pattern) {
        return Pattern::Mottle;
      }
      if ("palms" == t.pattern) {
        return Pattern::Palms;
      }
      throw std::invalid_argument("terrain '" + t.id + "': pattern '" + t.pattern +
                                  "' is not one of none dots hatch mottle palms");
    }

    Commands
    hexPolygon(const MapContext& ctx, const HexId& id, double inset = 0.0)
    {
      const auto corners = ctx.frame.corners(id, inset);
      return polygonPath(std::vector<Pixel>(corners.begin(), corners.end()));
    }

    void
    addBackground(const MapContext& ctx, Scene& scene)
    {
      const Color bg =
          ctx.sheet.background.empty()
              ? Color{255, 255, 255, 255}
              : ctx.color(ctx.sheet.background, "sheet '" + ctx.sheet.id + "' background");
      scene.add(Layer::Background,
                pathPrimitive(rectPath(0, 0, ctx.frame.width(), ctx.frame.height()), Fill{bg, 1.0},
                              std::nullopt));
      return;
    }

    void
    addTerrain(const MapContext& ctx, Scene& scene)
    {
      std::set<std::string> urban;
      if ("buildings" == ctx.sheet.urban) {
        for (const HexXml::SheetHexDoc& h : ctx.sheet.hexes) {
          for (const HexXml::SheetGlyphDoc& g : h.glyphs) {
            if (urbanSymbolP(g.symbol)) {
              urban.insert(h.id);
            }
          }
        }
      }
      for (const HexCoord::Grid& grid : ctx.frame.grids()) {
        for (const HexId& id : cellOrder(grid)) {
          const std::string at = "hex '" + id.text + "'";
          const HexXml::SheetTerrainDoc& t = ctx.terrain(ctx.terrainOf.at(id.text), at);
          Primitive p = pathPrimitive(hexPolygon(ctx, id),
                                      Fill{ctx.color(t.fill, "terrain '" + t.id + "' fill"), 1.0},
                                      std::nullopt, ctx.hexHit(id));
          std::get<PathShape>(p.shape).pattern = patternOf(t);
          scene.add(Layer::Terrain, std::move(p));
          if (urban.contains(id.text)) {
            scene.add(Layer::Terrain, pathPrimitive(hexPolygon(ctx, id), Fill{kUrbanTint, 1.0},
                                                    std::nullopt, ctx.hexHit(id)));
          }
        }
      }
      return;
    }

    void
    addRegionOutline(const MapContext& ctx, const HexXml::SheetRegionDoc& reg,
                     const std::string& at, Scene& scene)
    {
      const std::set<std::string> members(reg.hexes.begin(), reg.hexes.end());
      Commands segments;
      for (const std::string& text : reg.hexes) {
        const HexId id{text};
        if (!ctx.terrainOf.contains(text)) {
          continue;  // the reference's outline skips an unknown hex (its tint has already thrown)
        }
        const HexCoord::Grid& g = ctx.frame.gridOf(id);
        const HexCoord::HexCentre centre = g.centreOf(*g.find(id));
        for (const HexCoord::Direction d : edgeOrder(g.spec().orientation)) {
          const std::optional<HexCoord::GridIndex> next = g.indexOf(centre.neighbour(d));
          if (!next.has_value() || !members.contains(g.idOf(*next).text)) {
            const auto [a, b] = ctx.frame.hexsideEnds(EdgeRef{id, d});
            segments.push_back(MoveTo{a});
            segments.push_back(LineTo{b});
          }
        }
      }
      const Stroke s = ctx.lineStroke(ctx.line(*reg.outline, at), at);
      scene.add(Layer::Regions, pathPrimitive(std::move(segments), std::nullopt, s));
      return;
    }

    void
    addRegions(const MapContext& ctx, Scene& scene)
    {
      for (const HexXml::SheetRegionDoc& reg : ctx.sheet.regions) {
        const std::string at = where(ctx.sheet, reg.sourceLine, "region '" + reg.name + "'");
        if (reg.tint.has_value()) {
          const Color tint = ctx.color(*reg.tint, at);
          for (const std::string& text : reg.hexes) {
            if (!ctx.terrainOf.contains(text)) {
              throw std::invalid_argument(at + ": hex '" + text + "' is not printed on any grid");
            }
            scene.add(Layer::Regions,
                      pathPrimitive(hexPolygon(ctx, HexId{text}), Fill{tint, reg.opacity},
                                    std::nullopt, ctx.hexHit(HexId{text})));
          }
        }
        if (reg.outline.has_value()) {
          addRegionOutline(ctx, reg, at, scene);
        }
      }
      return;
    }

    void
    addGrid(const MapContext& ctx, Scene& scene)
    {
      for (const HexCoord::Grid& grid : ctx.frame.grids()) {
        const double width = std::max(0.6, grid.spec().size * 0.035);
        for (const HexId& id : cellOrder(grid)) {
          const std::string at = "hex '" + id.text + "'";
          const HexXml::SheetTerrainDoc& t = ctx.terrain(ctx.terrainOf.at(id.text), at);
          const Stroke s{ctx.color(t.stroke, "terrain '" + t.id + "' stroke"),
                         width,
                         {},
                         1.0,
                         LineCap::Butt,
                         LineJoin::Miter};
          scene.add(Layer::Grid, pathPrimitive(hexPolygon(ctx, id), std::nullopt, s));
        }
      }
      return;
    }

  }  // namespace

  void
  addAreaLayers(const MapContext& ctx, Scene& scene)
  {
    addBackground(ctx, scene);
    addTerrain(ctx, scene);
    addRegions(ctx, scene);
    addGrid(ctx, scene);
    return;
  }

  void
  addRingsLayer(const MapContext& ctx, Scene& scene)
  {
    for (const HexXml::SheetHexDoc& h : ctx.sheet.hexes) {
      if (!h.ring.has_value()) {
        continue;
      }
      const std::string at = where(ctx.sheet, h.line, "hex '" + h.id + "' ring");
      const HexId id{h.id};
      if (!ctx.terrainOf.contains(h.id)) {
        throw std::invalid_argument(at + ": the hex is not printed on any grid");
      }
      const Stroke s{ctx.color(*h.ring, at),
                     h.ringWidth.value_or(ctx.sizeOf(id) * 0.12),
                     {},
                     1.0,
                     LineCap::Butt,
                     LineJoin::Round};
      scene.add(Layer::Rings,
                pathPrimitive(hexPolygon(ctx, id, 0.12), std::nullopt, s, ctx.hexHit(id)));
    }
    return;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
