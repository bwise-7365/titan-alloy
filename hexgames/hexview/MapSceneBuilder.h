// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] The static map: a sheet document into the map layers of a Scene, layer for
// layer as hexsheet2svg.py renders it (background, terrain with patterns, regions, grid and printed
// ids, edges, links, rings, hex glyphs, side glyphs, labels, panels). Terrain, hexside and link
// primitives carry hit tags. The one choice a caller makes is the MapStyle.
// ----------------------------------------------
#pragma once
#include "hexmodel/Board.h"
#include "hexview/LineGeometry.h"
#include "hexview/MapFrame.h"
#include "hexview/Scene.h"
#include "hexview/SymbolLibrary.h"
#include "hexxml/SheetDoc.h"

#include <set>
#include <string>

namespace HexView {

  struct MapStyle {
    // Hexside line ids drawn with rounded corners; every other hexside line is drawn straight.
    std::set<std::string> roundedLines;
    bool smoothLinksP = true;
    // What the reference renderer does today: rivers rounded, links smoothed.
    static MapStyle reference() { return MapStyle{{"river"}, true}; }
  };

  class MapSceneBuilder {
  public:
    // `board` supplies the dense hex and space indices for hit tags; it must come from `sheet`.
    MapSceneBuilder(const MapFrame&, const HexModel::Board&, const SymbolLibrary&, MapStyle);

    // Adds the map layers to `scene`. Throws std::invalid_argument naming the element and its source
    // line for a reference the sheet does not resolve (palette colour, terrain, line, hex id, symbol).
    void build(const HexXml::SheetDoc& sheet, Scene& scene) const;

  private:
    const MapFrame& frame_;
    const HexModel::Board& board_;
    const SymbolLibrary& symbols_;
    MapStyle style_;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
