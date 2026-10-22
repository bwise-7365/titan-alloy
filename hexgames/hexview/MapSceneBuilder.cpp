// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The builder and the lookups every layer shares. Where the reference renderer warns and carries on
// (an unknown colour, terrain, line, hex or symbol), this throws, naming the element; the defaults
// it keeps are the reference's own for attributes a sheet leaves out (a glyph with no colour is
// #333). One reference behaviour is kept although it skips silently, and is listed in task 17's
// open questions: ids in a bulk <hexes> element that no grid prints (TRC lists row 0).
// ----------------------------------------------
#include "hexview/MapSceneBuilder.h"

#include "hexview/MapLayers.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <sstream>
#include <stdexcept>

namespace HexView {

  namespace {

    std::vector<double>
    parseDash(const std::string& text, std::string_view where)
    {
      std::string spaced = text;
      std::replace(spaced.begin(), spaced.end(), ',', ' ');
      std::istringstream in(spaced);
      std::vector<double> out;
      double v = 0.0;
      while (in >> v) {
        out.push_back(v);
      }
      if (!in.eof() || out.empty()) {
        throw std::invalid_argument(std::string(where) + ": dash '" + text +
                                    "' is not a list of numbers");
      }
      return out;
    }

    double
    slotAngle(std::string_view slot, std::string_view where)
    {
      static constexpr std::pair<std::string_view, double> kAngles[] = {
          {"e", 0},   {"se", 45},  {"s", 90},  {"sw", 135},
          {"w", 180}, {"nw", 225}, {"n", 270}, {"ne", 315}};
      for (const auto& [name, angle] : kAngles) {
        if (name == slot) {
          return angle;
        }
      }
      throw std::invalid_argument(std::string(where) + ": slot '" + std::string(slot) +
                                  "' is not a compass point");
    }

  }  // namespace

  std::string
  where(const HexXml::SheetDoc& sheet, int line, std::string_view element)
  {
    return "sheet '" + sheet.id + "' line " + std::to_string(line) + ": " + std::string(element);
  }

  std::vector<HexId>
  cellOrder(const HexCoord::Grid& g)
  {
    std::vector<std::pair<HexCoord::GridIndex, HexId>> cells;
    for (const HexId& id : g.ids()) {
      cells.emplace_back(*g.find(id), id);
    }
    std::sort(cells.begin(), cells.end(),
              [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<HexId> out;
    for (const auto& [index, id] : cells) {
      out.push_back(id);
    }
    return out;
  }

  std::vector<HexCoord::Direction>
  edgeOrder(HexCoord::Orientation o)
  {
    std::vector<HexCoord::Direction> dirs;
    for (int k = 0; k < HexCoord::kDirections; ++k) {
      dirs.push_back(static_cast<HexCoord::Direction>(k));
    }
    std::sort(dirs.begin(), dirs.end(), [o](HexCoord::Direction a, HexCoord::Direction b) {
      return HexCoord::edgeAngleDegrees(a, o) < HexCoord::edgeAngleDegrees(b, o);
    });
    return dirs;
  }

  Primitive
  pathPrimitive(Commands commands, std::optional<Fill> fill, std::optional<Stroke> stroke,
                HitTag hit)
  {
    PathShape s;
    s.commands = std::move(commands);
    s.fill = std::move(fill);
    s.stroke = std::move(stroke);
    return Primitive{s, std::move(hit)};
  }

  TextShape
  textShape(Pixel at, std::string text, Font font, Color color, TextAnchor anchor,
            TextBaseline baseline, double angle)
  {
    TextShape t;
    t.at = at;
    t.text = std::move(text);
    t.font = std::move(font);
    t.color = color;
    t.anchor = anchor;
    t.baseline = baseline;
    t.angleDegrees = angle;
    return t;
  }

  MapContext::MapContext(const HexXml::SheetDoc& s, const MapFrame& f, const SymbolLibrary& lib,
                         const MapStyle& st)
      : sheet(s), frame(f), symbols(lib), style(st)
  {
    for (const HexXml::SheetColorDoc& c : sheet.palette) {
      palette[c.id] =
          "none" == c.value
              ? std::nullopt
              : std::optional<Color>(Color::parse(c.value, "palette colour '" + c.id + "'"));
    }
    for (const HexXml::SheetTerrainDoc& t : sheet.terrains) {
      terrains[t.id] = &t;
    }
    for (const HexXml::SheetLineDoc& l : sheet.lines) {
      lines[l.id] = &l;
    }
    for (std::size_t g = 0; g < frame.grids().size(); ++g) {
      for (const HexId& id : frame.grids()[g].ids()) {
        terrainOf[id.text] = sheet.grids[g].terrain;
      }
    }
    for (const HexXml::SheetHexesDoc& bulk : sheet.hexesBulk) {
      for (const std::string& id : bulk.ids) {
        if (terrainOf.contains(id)) {  // the reference skips ids no grid prints (open question)
          terrainOf[id] = bulk.terrain;
        }
      }
    }
    for (const HexXml::SheetHexDoc& h : sheet.hexes) {
      if (h.terrain.has_value() && terrainOf.contains(h.id)) {
        terrainOf[h.id] = *h.terrain;
      }
    }
  }

  Color
  MapContext::color(const std::string& id, std::string_view at) const
  {
    const auto it = palette.find(id);
    if (palette.end() == it) {
      throw std::invalid_argument(std::string(at) + ": colour '" + id + "' is not in the palette");
    }
    if (!it->second.has_value()) {
      throw std::invalid_argument(std::string(at) + ": colour '" + id +
                                  "' is none, which cannot paint here");
    }
    return *it->second;
  }

  Color
  MapContext::colorOr(const std::optional<std::string>& id, Color fallback,
                      std::string_view at) const
  {
    return id.has_value() ? color(*id, at) : fallback;
  }

  const HexXml::SheetLineDoc&
  MapContext::line(const std::string& id, std::string_view at) const
  {
    const auto it = lines.find(id);
    if (lines.end() == it) {
      throw std::invalid_argument(std::string(at) + ": line '" + id + "' is not declared");
    }
    return *it->second;
  }

  const HexXml::SheetTerrainDoc&
  MapContext::terrain(const std::string& id, std::string_view at) const
  {
    const auto it = terrains.find(id);
    if (terrains.end() == it) {
      throw std::invalid_argument(std::string(at) + ": terrain '" + id + "' is not declared");
    }
    return *it->second;
  }

  const HexXml::SheetMarkDoc&
  MapContext::mark(const std::string& id, std::string_view at) const
  {
    for (const HexXml::SheetMarkDoc& m : sheet.legend) {
      if (id == m.id) {
        return m;
      }
    }
    throw std::invalid_argument(std::string(at) + ": mark '" + id +
                                "' is not in the sheet's legend");
  }

  Stroke
  MapContext::lineStroke(const HexXml::SheetLineDoc& l, std::string_view at) const
  {
    Stroke s;
    s.color = color(l.stroke, std::string(at) + ", line '" + l.id + "' stroke");
    s.width = l.width;
    s.dash = l.dash.has_value() ? parseDash(*l.dash, at) : std::vector<double>{};
    s.opacity = l.opacity.value_or(1.0);
    s.cap = LineCap::Round;
    s.join = LineJoin::Round;
    return s;
  }

  std::optional<Stroke>
  MapContext::casingStroke(const HexXml::SheetLineDoc& l, std::string_view at) const
  {
    if (!l.casing.has_value()) {
      return std::nullopt;
    }
    return Stroke{color(*l.casing, std::string(at) + ", line '" + l.id + "' casing"),
                  l.casingWidth.value_or(l.width * 2.2),
                  {},
                  1.0,
                  LineCap::Round,
                  LineJoin::Round};
  }

  bool
  MapContext::roundedP(const std::string& lineId) const
  {
    return style.roundedLines.contains(lineId) ||
           (style.roundedLines.contains("river") && std::string::npos != lineId.find("river"));
  }

  double
  MapContext::sizeOf(const HexId& id) const
  {
    return frame.gridOf(id).spec().size;
  }

  HexCoord::Orientation
  MapContext::orientationOf(const HexId& id) const
  {
    return frame.gridOf(id).spec().orientation;
  }

  Pixel
  MapContext::slotPoint(const HexId& id, std::string_view slot, double k) const
  {
    const Pixel c = frame.centre(id);
    if ("c" == slot) {
      return c;
    }
    const double a = slotAngle(slot, "hex '" + id.text + "'") * std::numbers::pi / 180.0;
    const double size = sizeOf(id);
    return Pixel{c.x + k * size * std::cos(a), c.y + k * size * std::sin(a)};
  }

  HexCoord::Direction
  MapContext::direction(const HexId& id, std::string_view name, std::string_view at) const
  {
    try {
      return HexCoord::fromCompass(name, orientationOf(id));
    }
    catch (const std::invalid_argument& e) {
      throw std::invalid_argument(std::string(at) + ": " + e.what());
    }
  }

  double
  MapContext::edgeAngle(const HexId& id, HexCoord::Direction d) const
  {
    return HexCoord::edgeAngleDegrees(d, orientationOf(id));
  }

  HitTag
  MapContext::hexHit(const HexId& id) const
  {
    return HexHit{frame.index(id)};
  }

  Font
  MapContext::font(double size) const
  {
    Font f;
    f.family = sheet.font;
    f.size = size;
    return f;
  }

  MapSceneBuilder::MapSceneBuilder(const MapFrame& frame, const SymbolLibrary& symbols,
                                   MapStyle style)
      : frame_(frame), symbols_(symbols), style_(std::move(style))
  {}

  void
  MapSceneBuilder::build(const HexXml::SheetDoc& sheet, Scene& scene) const
  {
    const MapContext ctx(sheet, frame_, symbols_, style_);
    addAreaLayers(ctx, scene);
    addEdgesLayer(ctx, scene);
    addLinksLayer(ctx, scene);
    addRingsLayer(ctx, scene);
    addHexGlyphsLayer(ctx, scene);
    addSideGlyphsLayer(ctx, scene);
    addLabelsLayer(ctx, scene);
    addPanelsLayer(ctx, scene);
    return;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
