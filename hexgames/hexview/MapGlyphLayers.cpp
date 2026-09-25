// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The glyph layers: hexsheet2svg.py layer_hexglyphs (symbols in slots, glyph text, scattered
// buildings, legend marks) and layer_sideglyphs (symbols on hexsides, edge symbols and marks, edge
// labels).
// ----------------------------------------------
#include "hexview/Buildings.h"
#include "hexview/MapLayers.h"

#include <algorithm>
#include <stdexcept>

namespace HexView {

  namespace {

    constexpr std::string_view kUrbanSymbols[] = {"city-major", "city", "capital"};
    constexpr Color kGlyphDefault{0x33, 0x33, 0x33, 255};
    constexpr Color kInk{0x11, 0x11, 0x11, 255};
    constexpr Color kWhite{0xff, 0xff, 0xff, 255};
    constexpr Color kBlack{0, 0, 0, 255};

    double
    dirAngle(std::string_view dir, std::string_view at)
    {
      static constexpr std::pair<std::string_view, double> kAngles[] = {
          {"e", 0},   {"se", 45},  {"s", 90},  {"sw", 135},
          {"w", 180}, {"nw", 225}, {"n", 270}, {"ne", 315}};
      for (const auto& [name, angle] : kAngles) {
        if (name == dir) {
          return angle;
        }
      }
      throw std::invalid_argument(std::string(at) + ": dir '" + std::string(dir) +
                                  "' is not a compass point");
    }

    SymbolUse
    use(const MapContext& ctx, const std::string& symbol, Pixel at, double rot, double scale,
        Color color, std::string_view where)
    {
      if (!ctx.symbols.containsP(symbol)) {
        throw std::invalid_argument(std::string(where) + ": symbol '" + symbol +
                                    "' is not in the symbol library");
      }
      return SymbolUse{symbol, at, rot, scale, color};
    }

    Primitive
    buildings(const MapContext& ctx, const HexId& id, Pixel at, Color color)
    {
      Commands d;
      for (const Box& b : scatterBuildings(ctx.sheet.id, id.text, at, ctx.sizeOf(id))) {
        const Commands r = rectPath(b.x0, b.y0, b.x1 - b.x0, b.y1 - b.y0);
        d.insert(d.end(), r.begin(), r.end());
      }
      return pathPrimitive(std::move(d), Fill{color, 1.0}, std::nullopt, ctx.hexHit(id));
    }

    void
    addGlyph(const MapContext& ctx, const HexXml::SheetHexDoc& h, const HexXml::SheetGlyphDoc& gl,
             int nth, const std::string& at, Scene& scene)
    {
      const HexId id{h.id};
      const double size = ctx.sizeOf(id);
      Pixel p = ctx.slotPoint(id, gl.slot, 0.5);
      p.x += nth * size * 0.36;  // several glyphs in one slot are spread horizontally
      const double rot = gl.dir.has_value() ? dirAngle(*gl.dir, at) - 270.0 : 0.0;
      if (!gl.symbol.has_value()) {
        const HexXml::SheetMarkDoc& m = ctx.mark(*gl.mark, at);
        const Color color =
            ctx.colorOr(gl.color.has_value() ? gl.color : m.color, kGlyphDefault, at);
        for (Primitive& part : markPrimitives(ctx, m, p, rot, size * gl.scale, color, NoHit{})) {
          scene.add(Layer::HexGlyphs, std::move(part));
        }
        return;  // the reference draws no glyph text for a mark
      }
      const std::string& sym = *gl.symbol;
      const bool urbanP = "buildings" == ctx.sheet.urban &&
                          std::end(kUrbanSymbols) !=
                              std::find(std::begin(kUrbanSymbols), std::end(kUrbanSymbols), sym);
      if (urbanP) {
        scene.add(Layer::HexGlyphs, buildings(ctx, id, p, ctx.colorOr(gl.color, kInk, at)));
      }
      else if ("text" != sym) {
        scene.add(Layer::HexGlyphs, Primitive{use(ctx, sym, p, rot, size * gl.scale,
                                                  ctx.colorOr(gl.color, kGlyphDefault, at), at),
                                              NoHit{}});
      }
      if (gl.text.has_value()) {
        Font f = ctx.font(size * 0.30 * gl.scale);
        f.weight = FontWeight::Bold;
        const Color fill = "position-badge" == sym ? kWhite : kInk;
        scene.add(Layer::HexGlyphs, Primitive{textShape(p, *gl.text, f, fill, TextAnchor::Middle,
                                                        TextBaseline::Central),
                                              NoHit{}});
      }
      return;
    }

  }  // namespace

  void
  addHexGlyphsLayer(const MapContext& ctx, Scene& scene)
  {
    for (const HexXml::SheetHexDoc& h : ctx.sheet.hexes) {
      const std::string at = where(ctx.sheet, h.line, "hex '" + h.id + "' glyph");
      if (!ctx.terrainOf.contains(h.id)) {
        throw std::invalid_argument(at + ": the hex is not printed on any grid");
      }
      std::map<std::string, int> slotCount;
      for (const HexXml::SheetGlyphDoc& gl : h.glyphs) {
        addGlyph(ctx, h, gl, slotCount[gl.slot]++, at, scene);
      }
    }
    return;
  }

  void
  addSideGlyphsLayer(const MapContext& ctx, Scene& scene)
  {
    for (const HexXml::SheetHexDoc& h : ctx.sheet.hexes) {
      const HexId id{h.id};
      for (const HexXml::SheetSideGlyphDoc& s : h.sides) {
        const std::string at = where(ctx.sheet, h.line, "hex '" + h.id + "' side " + s.dir);
        const HexCoord::Direction d = ctx.direction(id, s.dir, at);
        const Pixel p = ctx.frame.hexsideMidpoint(EdgeRef{id, d});
        scene.add(Layer::SideGlyphs,
                  Primitive{use(ctx, s.symbol, p, ctx.edgeAngle(id, d) - 90.0, ctx.sizeOf(id),
                                ctx.colorOr(s.color, kGlyphDefault, at), at),
                            HexsideHit{ctx.frame.index(id), d}});
      }
    }
    for (const HexXml::SheetEdgeDoc& e : ctx.sheet.edges) {
      if (!e.symbol.has_value() && !e.mark.has_value()) {
        continue;
      }
      const std::string at = where(ctx.sheet, e.sourceLine, "edge '" + e.at + "'");
      const EdgeRef ref = ctx.frame.edgeRef(e.at);
      const Pixel p = ctx.frame.hexsideMidpoint(ref);
      const double size = ctx.sizeOf(ref.hex);
      const double angle = ctx.edgeAngle(ref.hex, ref.dir);
      const HitTag hit = HexsideHit{ctx.frame.index(ref.hex), ref.dir};
      if (e.symbol.has_value()) {
        scene.add(Layer::SideGlyphs, Primitive{use(ctx, *e.symbol, p, angle, size,
                                                   ctx.colorOr(e.color, kWhite, at), at),
                                               hit});
      }
      else {
        const HexXml::SheetMarkDoc& m = ctx.mark(*e.mark, at);
        // across="edge": the mark's x axis lies across the hexside
        const double rot = "edge" == m.across.value_or("") ? angle : angle - 90.0;
        const Color color = ctx.colorOr(e.color.has_value() ? e.color : m.color, kWhite, at);
        for (Primitive& part : markPrimitives(ctx, m, p, rot, size, color, hit)) {
          scene.add(Layer::SideGlyphs, std::move(part));
        }
      }
      if (e.label.has_value()) {
        Font f = ctx.font(size * 0.28);
        f.italicP = true;
        TextShape t = textShape(Pixel{p.x, p.y + size * 0.75}, *e.label, f, kWhite,
                                TextAnchor::Middle, TextBaseline::Alphabetic);
        t.halo = Stroke{kBlack, 0.3, {}, 1.0, LineCap::Butt, LineJoin::Miter};
        scene.add(Layer::SideGlyphs, Primitive{t, NoHit{}});
      }
    }
    return;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
