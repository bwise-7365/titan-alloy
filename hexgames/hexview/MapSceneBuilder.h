// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] The static map: a sheet document into the map layers of a Scene, layer for
// layer as hexsheet2svg.py renders it (background, terrain with patterns, regions, grid and printed
// ids, edges, links, rings, hex glyphs, side glyphs, labels, panels). Terrain and hexside primitives
// carry hit tags (hex indices from the MapFrame, which numbers hexes as BoardBuilder does), panels
// carry their panel id. The one choice a caller makes is the MapStyle.
// ----------------------------------------------
#pragma once
#include "hexview/LineGeometry.h"
#include "hexview/MapFrame.h"
#include "hexview/Scene.h"
#include "hexview/SymbolLibrary.h"
#include "hexxml/SheetDoc.h"

#include <optional>
#include <set>
#include <string>

namespace HexView {

  // The hand-scratched look of irrgo's Latrunculi board (hexview/Scratch.h) for the lines a reader's
  // eye follows: link chains whose kind contains "road" or "rail", and the rounded (river) hexside
  // lines, which are then drawn as scratched corner chains instead of rounded ones. Presentation
  // only (Ben, 2026-09-23); the reference style has none, so the SVG goldens are untouched.
  struct ScratchStyle {
    double roughness;  // [0, 1]
    double smoothing;  // [0, 1]
  };

  struct MapStyle {
    // Hexside line ids drawn with rounded corners; every other hexside line is drawn straight. As in
    // the reference, "river" also rounds every line whose id contains "river" (major-river ...).
    std::set<std::string> roundedLines;
    bool smoothLinksP = true;
    std::optional<ScratchStyle> scratch;
    // What the reference renderer does today: rivers rounded, links smoothed, nothing scratched.
    static MapStyle reference() { return MapStyle{{"river"}, true, std::nullopt}; }
  };

  class MapSceneBuilder {
  public:
    // `frame` must come from the sheet passed to build(). No Board: a map is drawn without rules (the
    // editor has none), and the frame's hex indices are the Board's.
    MapSceneBuilder(const MapFrame&, const SymbolLibrary&, MapStyle);

    // Adds the map layers to `scene`. Throws std::invalid_argument naming the element and its source
    // line for a reference the sheet does not resolve (palette colour, terrain, line, hex id, symbol).
    void build(const HexXml::SheetDoc& sheet, Scene& scene) const;

  private:
    const MapFrame& frame_;
    const SymbolLibrary& symbols_;
    MapStyle style_;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
