// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexgen/ToSheet.h"

#include "hexgen/Chains.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace HexGen {

  namespace {

    std::size_t
    commonest(const GenMap& m, std::size_t terrains)
    {
      std::vector<int> counts(terrains, 0);
      for (const std::size_t t : m.terrain) {
        ++counts[t];
      }
      return static_cast<std::size_t>(std::max_element(counts.begin(), counts.end()) - counts.begin());
    }

    HexXml::SheetGridDoc
    gridDocOf(const HexCoord::GridSpec& g, const std::string& terrain)
    {
      HexXml::SheetGridDoc doc;
      doc.id = g.id;
      const bool flat = HexCoord::Orientation::Flat == g.orientation;
      doc.orientation = flat ? "flat" : "pointy";
      doc.offset = HexCoord::Parity::Odd == g.offset ? "odd" : "even";
      doc.cols = g.cols;
      doc.rows = g.rows;
      doc.size = g.size;
      doc.ox = g.ox;
      doc.oy = g.oy;
      doc.idFormat = g.idFormat;
      doc.colStart = g.colStart;
      doc.rowStart = g.rowStart;
      doc.colStep = g.colStep;
      doc.rowStep = g.rowStep;
      doc.terrain = terrain;
      doc.idSide = flat ? "n" : "w";  // the reader's default for each orientation
      return doc;
    }

    std::string
    idOf(const GenMap& m, int hex)
    {
      return m.grid().idOf(HexCoord::GridIndex{hex % m.cols(), hex / m.cols()}).text;
    }

  }  // namespace

  HexXml::SheetDoc
  toSheet(const GenMap& m, const Settings& s)
  {
    HexXml::SheetDoc doc;
    doc.id = s.id;
    doc.title = s.title;
    doc.background = "paper";
    doc.urban = "symbol";
    doc.junctions = "implicit";
    doc.seed = s.seed;

    double right = 0.0;
    double bottom = 0.0;
    for (int hex = 0; hex < m.size(); ++hex) {
      for (const HexCoord::Pixel& p : m.grid().polygon(m.centre(hex))) {
        right = std::max(right, p.x);
        bottom = std::max(bottom, p.y);
      }
    }
    doc.width = right + kMargin;
    doc.height = bottom + kMargin;

    const std::size_t grounds = commonest(m, s.terrains.size());
    doc.grids.push_back(gridDocOf(m.grid().spec(), s.terrains[grounds].id));

    doc.palette.push_back({"paper", "#FFFFFF", std::nullopt});
    doc.palette.push_back({"grid-ink", "#808080", std::nullopt});
    for (const TerrainSetting& t : s.terrains) {
      doc.palette.push_back({"fill-" + t.id, t.fill, std::nullopt});
      HexXml::SheetTerrainDoc terrain;
      terrain.id = t.id;
      terrain.name = t.name;
      terrain.fill = "fill-" + t.id;
      terrain.stroke = "grid-ink";
      terrain.moveCost = t.moveCost;
      doc.terrains.push_back(terrain);
    }

    for (std::size_t t = 0; t < s.terrains.size(); ++t) {
      if (grounds == t) {
        continue;
      }
      HexXml::SheetHexesDoc hexes;
      hexes.terrain = s.terrains[t].id;
      for (int hex = 0; hex < m.size(); ++hex) {
        if (t == m.terrain[static_cast<std::size_t>(hex)]) {
          hexes.ids.push_back(idOf(m, hex));
        }
      }
      if (!hexes.ids.empty()) {
        doc.hexesBulk.push_back(hexes);
      }
    }

    for (int hex = 0; hex < static_cast<int>(m.alt.size()); ++hex) {
      const double alt = std::round(m.alt[static_cast<std::size_t>(hex)] * 10.0) / 10.0;
      if (0.0 != alt) {
        HexXml::SheetHexDoc h;
        h.id = idOf(m, hex);
        h.alt = alt;
        doc.hexes.push_back(h);
      }
    }

    if (s.roads) {
      doc.palette.push_back({"road-ink", s.roads->stroke, std::nullopt});
      HexXml::SheetLineDoc line;
      line.id = s.roads->line;
      line.stroke = "road-ink";
      line.width = s.roads->width;
      doc.lines.push_back(line);
      int k = 0;
      for (const std::vector<int>& chain : chainsOf(m.roads(), m.size())) {
        HexXml::SheetLinkDoc link;
        link.id = "road-" + std::to_string(++k);
        link.kind = "road";
        link.line = s.roads->line;
        for (const int hex : chain) {
          link.hexes.push_back(idOf(m, hex));
        }
        doc.links.push_back(link);
      }
    }
    return doc;
  }

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
