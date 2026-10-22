// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexview-internal: MapSceneBuilder split by layer. MapContext holds what every layer looks up (the
// palette, terrains, lines, each hex's terrain) and the reference renderer's small helpers; each
// add*Layer function transcribes one of hexsheet2svg.py's layer_* methods, in its element order.
// ----------------------------------------------
#pragma once
#include "hexview/MapSceneBuilder.h"
#include "hexview/Shapes.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace HexView {

  struct MapContext {
    const HexXml::SheetDoc& sheet;
    const MapFrame& frame;
    const SymbolLibrary& symbols;
    const MapStyle& style;
    std::map<std::string, std::optional<Color>> palette;  // nullopt: the value "none"
    std::map<std::string, const HexXml::SheetTerrainDoc*> terrains;
    std::map<std::string, const HexXml::SheetLineDoc*> lines;
    std::map<std::string, std::string> terrainOf;  // printed id -> terrain id

    MapContext(const HexXml::SheetDoc&, const MapFrame&, const SymbolLibrary&, const MapStyle&);

    // Palette lookups; each throws std::invalid_argument naming the colour and `where`.
    Color color(const std::string& id, std::string_view where) const;
    // The reference's col(id, default): an absent attribute takes the renderer's own default.
    Color colorOr(const std::optional<std::string>& id, Color fallback,
                  std::string_view where) const;
    const HexXml::SheetLineDoc& line(const std::string& id, std::string_view where) const;
    const HexXml::SheetTerrainDoc& terrain(const std::string& id, std::string_view where) const;
    // A legend mark by id; throws naming the mark and `where` when the legend has none.
    const HexXml::SheetMarkDoc& mark(const std::string& id, std::string_view where) const;

    // line_attrs and casing_attrs: the stroke of a line, and its casing if it has one.
    Stroke lineStroke(const HexXml::SheetLineDoc&, std::string_view where) const;
    std::optional<Stroke> casingStroke(const HexXml::SheetLineDoc&, std::string_view where) const;
    // Whether a hexside line is drawn with rounded corners (the reference's smooth_lines rule).
    bool roundedP(const std::string& lineId) const;

    double sizeOf(const HexId&) const;
    HexCoord::Orientation orientationOf(const HexId&) const;
    // Grid.slot_point: the centre, or k * size toward one of eight compass points.
    Pixel slotPoint(const HexId&, std::string_view slot, double k) const;
    // A hexside direction named in the hex's own orientation; throws naming `where`.
    HexCoord::Direction direction(const HexId&, std::string_view name,
                                  std::string_view where) const;
    double edgeAngle(const HexId&, HexCoord::Direction) const;
    HitTag hexHit(const HexId&) const;
    Font font(double size) const;  // the sheet's font family at a size
  };

  // "sheet 'id' line N: element" for error messages.
  std::string where(const HexXml::SheetDoc&, int line, std::string_view element);

  // A grid's printed ids in the reference's cell order: column by column, top to bottom.
  std::vector<HexId> cellOrder(const HexCoord::Grid&);

  // The six hexside directions in the reference's order (its edges dict): by outward angle, from 0
  // (pointy: e) or 30 degrees (flat: se).
  std::vector<HexCoord::Direction> edgeOrder(HexCoord::Orientation);

  Primitive pathPrimitive(Commands, std::optional<Fill>, std::optional<Stroke>, HitTag = NoHit{});
  TextShape textShape(Pixel at, std::string text, Font, Color, TextAnchor, TextBaseline,
                      double angle = 0.0);

  // mark_svg: a declared mark drawn at a point, turned by rot degrees and scaled by size (the hex
  // size), in `color`: its shape's body placed as the reference's <g transform> places it, a
  // pictogram symbol, or its name as text.
  std::vector<Primitive> markPrimitives(const MapContext&, const HexXml::SheetMarkDoc&, Pixel at,
                                        double rot, double size, Color color, HitTag hit);

  void addAreaLayers(const MapContext&, Scene&);  // background, terrain, regions, grid
  void addEdgesLayer(const MapContext&, Scene&);
  void addLinksLayer(const MapContext&, Scene&);
  void addRingsLayer(const MapContext&, Scene&);
  void addHexGlyphsLayer(const MapContext&, Scene&);
  void addSideGlyphsLayer(const MapContext&, Scene&);
  void addLabelsLayer(const MapContext&, Scene&);
  void addPanelsLayer(const MapContext&, Scene&);

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
