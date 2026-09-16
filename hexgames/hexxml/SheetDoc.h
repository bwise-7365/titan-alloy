// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A hexsheet document (map_graphics/xml/hexsheet.xsd), mirrored one to one. The sheet body is a
// mixed-content choice of hexes/hex/edge/path/link/region/label/panel, repeated in any order; parse()
// keeps each kind in its own vector, in the document order it was written (BoardBuilder needs no
// cross-kind interleaving: bulk and per-hex terrain, edges, links and regions each write disjoint
// Board state). Every type here is prefixed Sheet* so it cannot collide with the same-named element
// of another hexxml document (hexrules and hexsheet both have a "region", for instance).
// ----------------------------------------------
#pragma once
#include "hexxml/XmlDocument.h"

#include <optional>
#include <string>
#include <vector>

namespace HexXml {

  struct SheetGridDoc {
    std::optional<std::string> id;
    std::string orientation;  // flat | pointy
    std::string offset;       // odd | even
    int cols = 0;
    int rows = 0;
    double size = 0.0;
    double ox = 0.0;
    double oy = 0.0;
    std::string idFormat;
    int colStart = 1;
    int rowStart = 1;
    int colStep = 1;
    int rowStep = 1;
    std::string terrain;
    bool showIds = true;
    std::string idSide = "w";
    std::optional<std::string> clip;
  };

  struct SheetColorDoc {
    std::string id;
    std::string value;  // "#rrggbb" or "none"
    std::optional<std::string> name;
  };

  struct SheetTerrainDoc {
    std::string id;
    std::string name;
    std::string fill;
    std::string stroke;
    std::string pattern = "none";
  };

  struct SheetLineDoc {
    std::string id;
    std::string stroke;
    double width = 0.0;
    std::optional<std::string> dash;
    std::optional<std::string> casing;
    std::optional<double> casingWidth;
    bool ticks = false;
    std::optional<double> opacity;
  };

  struct SheetHexesDoc {  // bulk terrain assignment
    std::string terrain;
    std::vector<std::string> ids;
    int line = 0;
  };

  struct SheetGlyphDoc {
    std::string symbol;
    std::string slot = "c";
    std::optional<std::string> color;
    std::optional<std::string> text;
    std::optional<std::string> dir;
    double scale = 1.0;
  };

  struct SheetSideGlyphDoc {
    std::string dir;
    std::string symbol;
    std::optional<std::string> color;
  };

  struct SheetHexDoc {
    std::string id;
    std::optional<std::string> terrain;
    std::optional<std::string> ring;
    std::optional<double> ringWidth;
    std::optional<std::string> name;
    std::vector<SheetGlyphDoc> glyphs;
    std::vector<SheetSideGlyphDoc> sides;
    int line = 0;
  };

  struct SheetEdgeDoc {
    std::string at;  // "HEX:DIR"
    std::optional<std::string> line;
    std::optional<std::string> symbol;
    std::optional<std::string> color;
    std::optional<std::string> label;
    int sourceLine = 0;
  };

  struct SheetPathDoc {
    std::optional<std::string> id;
    std::string kind;
    std::optional<std::string> name;
    std::string line;
    std::vector<std::string> edges;  // "HEX:DIR" tokens
    double offset = 0.0;
    int sourceLine = 0;
  };

  struct SheetLinkDoc {
    std::string kind;
    std::optional<std::string> name;
    std::string line;
    std::vector<std::string> hexes;
    std::optional<std::string> owner;
    int sourceLine = 0;
  };

  struct SheetRegionDoc {
    std::string layer;
    std::string name;
    std::vector<std::string> hexes;
    std::optional<std::string> tint;
    double opacity = 0.35;
    std::optional<std::string> outline;
    std::optional<std::string> label;
    std::optional<std::string> labelAt;
    int sourceLine = 0;
  };

  struct SheetLabelDoc {
    std::string text;
    std::optional<std::string> at;
    std::string slot = "c";
    std::optional<double> x;
    std::optional<double> y;
    std::optional<std::string> path;
    double angle = 0.0;
    double size = 0.0;
    std::optional<std::string> color;
    std::string weight = "normal";
    bool italic = false;
    double spacing = 0.0;
    bool halo = false;
    std::string anchor = "middle";
  };

  struct SheetPanelTextDoc {
    double x = 0.0;
    double y = 0.0;
    double size = 0.0;
    std::optional<std::string> weight;
    std::optional<std::string> color;
    std::string text;
  };

  struct SheetBoxDoc {
    std::optional<std::string> label;
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    std::optional<std::string> fill;
  };

  struct SheetTrackDoc {
    double x = 0.0;
    double y = 0.0;
    double cellW = 0.0;
    double cellH = 0.0;
    std::string cells;  // whitespace-separated cell labels
    std::string direction = "h";
    std::optional<int> wrap;
    std::optional<std::string> fill;
  };

  struct SheetPanelTableRowDoc {
    std::vector<std::string> cells;
  };

  struct SheetPanelTableDoc {
    double x = 0.0;
    double y = 0.0;
    double cellW = 0.0;
    double cellH = 0.0;
    std::optional<double> size;
    std::vector<SheetPanelTableRowDoc> rows;
  };

  struct SheetPanelDoc {
    std::string id;
    std::optional<std::string> title;
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
    double rotate = 0.0;
    std::optional<std::string> fill;
    std::optional<std::string> stroke;
    std::vector<SheetPanelTextDoc> texts;
    std::vector<SheetBoxDoc> boxes;
    std::vector<SheetTrackDoc> tracks;
    std::vector<SheetPanelTableDoc> tables;
    int sourceLine = 0;
  };

  struct SheetDoc {
    std::string id;
    std::string title;
    std::optional<std::string> source;
    double width = 0.0;
    double height = 0.0;
    std::string background;
    std::string font = "Arial, Helvetica, sans-serif";
    std::string urban;  // buildings | symbol: how city hexes are drawn (required)

    std::vector<SheetGridDoc> grids;
    std::vector<SheetColorDoc> palette;
    std::vector<SheetTerrainDoc> terrains;
    std::vector<SheetLineDoc> lines;

    // Each in the document order it was written; the eight kinds may be interleaved in the file, but
    // nothing in BoardBuilder needs the cross-kind interleaving, only the order within a kind.
    std::vector<SheetHexesDoc> hexesBulk;
    std::vector<SheetHexDoc> hexes;
    std::vector<SheetEdgeDoc> edges;
    std::vector<SheetPathDoc> paths;
    std::vector<SheetLinkDoc> links;
    std::vector<SheetRegionDoc> regions;
    std::vector<SheetLabelDoc> labels;
    std::vector<SheetPanelDoc> panels;

    static SheetDoc parse(const XmlDocument&);
  };

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
