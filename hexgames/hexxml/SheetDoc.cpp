// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/SheetDoc.h"

#include "hexxml/DocDetail.h"

namespace HexXml {

  using Detail::checkEnum;
  using Detail::idrefs;
  using Detail::requiredChild;
  using Detail::splitTokens;

  namespace {

    constexpr std::initializer_list<std::string_view> kDirs = {"n", "ne", "e", "se",
                                                                 "s", "sw", "w", "nw"};
    constexpr std::initializer_list<std::string_view> kSlots = {"c",  "n",  "ne", "e", "se",
                                                                  "s", "sw", "w",  "nw"};

    SheetGridDoc
    parseGrid(const XmlNode& node)
    {
      SheetGridDoc g;
      g.id = node.optional("id");
      g.orientation = node.required("orientation");
      checkEnum(node, "orientation", g.orientation, {"flat", "pointy"});
      g.offset = node.required("offset");
      checkEnum(node, "offset", g.offset, {"odd", "even"});
      g.cols = node.requiredAs<int>("cols");
      g.rows = node.requiredAs<int>("rows");
      g.size = node.requiredAs<double>("size");
      g.ox = node.requiredAs<double>("ox");
      g.oy = node.requiredAs<double>("oy");
      g.idFormat = node.required("id-format");
      g.colStart = node.optionalAs<int>("col-start").value_or(1);
      g.rowStart = node.optionalAs<int>("row-start").value_or(1);
      g.colStep = node.optionalAs<int>("col-step").value_or(1);
      g.rowStep = node.optionalAs<int>("row-step").value_or(1);
      g.terrain = node.required("terrain");
      g.showIds = node.optionalAs<bool>("show-ids").value_or(true);
      g.idSide = node.optional("id-side").value_or("w");
      checkEnum(node, "id-side", g.idSide, kDirs);
      g.clip = node.optional("clip");
      return g;
    }

    SheetColorDoc
    parseColor(const XmlNode& node)
    {
      SheetColorDoc c;
      c.id = node.required("id");
      c.value = node.required("value");
      c.name = node.optional("name");
      return c;
    }

    SheetTerrainDoc
    parseTerrain(const XmlNode& node)
    {
      SheetTerrainDoc t;
      t.id = node.required("id");
      t.name = node.required("name");
      t.fill = node.required("fill");
      t.stroke = node.required("stroke");
      t.pattern = node.optional("pattern").value_or("none");
      checkEnum(node, "pattern", t.pattern, {"none", "dots", "hatch", "mottle", "palms"});
      return t;
    }

    SheetLineDoc
    parseLine(const XmlNode& node)
    {
      SheetLineDoc l;
      l.id = node.required("id");
      l.stroke = node.required("stroke");
      l.width = node.requiredAs<double>("width");
      l.dash = node.optional("dash");
      l.casing = node.optional("casing");
      l.casingWidth = node.optionalAs<double>("casing-width");
      l.ticks = node.optionalAs<bool>("ticks").value_or(false);
      l.opacity = node.optionalAs<double>("opacity");
      return l;
    }

    SheetMarkDoc
    parseMark(const XmlNode& node)
    {
      SheetMarkDoc m;
      m.id = node.required("id");
      m.name = node.optional("name");
      m.shape = node.required("shape");
      m.color = node.optional("color");
      m.size = node.optional("size");
      m.pictogram = node.optional("pictogram");
      m.across = node.optional("across");
      m.from = node.optional("from");
      m.sourceLine = node.line();
      return m;
    }

    SheetHexesDoc
    parseHexes(const XmlNode& node)
    {
      SheetHexesDoc h;
      h.terrain = node.required("terrain");
      h.ids = splitTokens(node.required("ids"));
      h.line = node.line();
      return h;
    }

    SheetGlyphDoc
    parseGlyph(const XmlNode& node)
    {
      SheetGlyphDoc g;
      g.symbol = node.optional("symbol");
      g.mark = node.optional("mark");
      if (g.symbol.has_value() == g.mark.has_value()) {
        throw std::invalid_argument(node.file() + ":" + std::to_string(node.line()) +
                                     ": glyph needs exactly one of symbol and mark");
      }
      g.slot = node.optional("slot").value_or("c");
      checkEnum(node, "slot", g.slot, kSlots);
      g.color = node.optional("color");
      g.text = node.optional("text");
      g.dir = node.optional("dir");
      if (g.dir) {
        checkEnum(node, "dir", *g.dir, kDirs);
      }
      g.scale = node.optionalAs<double>("scale").value_or(1.0);
      return g;
    }

    SheetSideGlyphDoc
    parseSideGlyph(const XmlNode& node)
    {
      SheetSideGlyphDoc s;
      s.dir = node.required("dir");
      checkEnum(node, "dir", s.dir, kDirs);
      s.symbol = node.required("symbol");
      s.color = node.optional("color");
      return s;
    }

    SheetHexDoc
    parseHex(const XmlNode& node)
    {
      SheetHexDoc h;
      h.id = node.required("id");
      h.terrain = node.optional("terrain");
      h.ring = node.optional("ring");
      h.ringWidth = node.optionalAs<double>("ring-width");
      h.name = node.optional("name");
      h.line = node.line();
      for (const XmlNode& g : node.children("glyph")) {
        h.glyphs.push_back(parseGlyph(g));
      }
      for (const XmlNode& s : node.children("side")) {
        h.sides.push_back(parseSideGlyph(s));
      }
      return h;
    }

    SheetEdgeDoc
    parseEdge(const XmlNode& node)
    {
      SheetEdgeDoc e;
      e.at = node.required("at");
      e.line = node.optional("line");
      e.symbol = node.optional("symbol");
      e.mark = node.optional("mark");
      e.color = node.optional("color");
      e.label = node.optional("label");
      e.sourceLine = node.line();
      return e;
    }

    SheetPathDoc
    parsePath(const XmlNode& node)
    {
      SheetPathDoc p;
      p.id = node.optional("id");
      p.kind = node.required("kind");
      p.name = node.optional("name");
      p.line = node.required("line");
      p.edges = splitTokens(node.required("edges"));
      p.offset = node.optionalAs<double>("offset").value_or(0.0);
      p.ends = node.optional("ends");
      p.sourceLine = node.line();
      return p;
    }

    SheetLinkDoc
    parseLink(const XmlNode& node)
    {
      SheetLinkDoc l;
      l.id = node.optional("id");
      l.kind = node.required("kind");
      l.name = node.optional("name");
      l.line = node.required("line");
      l.hexes = splitTokens(node.required("hexes"));
      l.owner = node.optional("owner");
      l.ends = node.optional("ends");
      l.sourceLine = node.line();
      return l;
    }

    SheetJunctionDoc
    parseJunction(const XmlNode& node)
    {
      SheetJunctionDoc j;
      j.at = node.required("at");
      if (const std::optional<std::string> links = node.optional("links")) {
        j.links = splitTokens(*links);
      }
      if (const std::optional<std::string> paths = node.optional("paths")) {
        j.paths = splitTokens(*paths);
      }
      j.name = node.optional("name");
      j.sourceLine = node.line();
      if (j.links.empty() == j.paths.empty()) {
        throw std::invalid_argument(node.file() + ":" + std::to_string(node.line()) +
                                     ": junction at '" + j.at + "' must name links or paths, not both nor neither");
      }
      return j;
    }

    SheetRegionDoc
    parseRegion(const XmlNode& node)
    {
      SheetRegionDoc r;
      r.layer = node.required("layer");
      r.name = node.required("name");
      r.hexes = splitTokens(node.required("hexes"));
      r.tint = node.optional("tint");
      r.opacity = node.optionalAs<double>("opacity").value_or(0.35);
      r.outline = node.optional("outline");
      r.label = node.optional("label");
      r.labelAt = node.optional("label-at");
      r.sourceLine = node.line();
      return r;
    }

    SheetLabelDoc
    parseLabel(const XmlNode& node)
    {
      SheetLabelDoc l;
      l.text = node.required("text");
      l.at = node.optional("at");
      l.slot = node.optional("slot").value_or("c");
      checkEnum(node, "slot", l.slot, kSlots);
      l.x = node.optionalAs<double>("x");
      l.y = node.optionalAs<double>("y");
      l.path = node.optional("path");
      l.angle = node.optionalAs<double>("angle").value_or(0.0);
      l.size = node.requiredAs<double>("size");
      l.color = node.optional("color");
      l.weight = node.optional("weight").value_or("normal");
      checkEnum(node, "weight", l.weight, {"normal", "bold"});
      l.italic = node.optionalAs<bool>("italic").value_or(false);
      l.spacing = node.optionalAs<double>("spacing").value_or(0.0);
      l.halo = node.optionalAs<bool>("halo").value_or(false);
      l.anchor = node.optional("anchor").value_or("middle");
      checkEnum(node, "anchor", l.anchor, {"start", "middle", "end"});
      return l;
    }

    SheetPanelTextDoc
    parsePanelText(const XmlNode& node)
    {
      SheetPanelTextDoc t;
      t.x = node.requiredAs<double>("x");
      t.y = node.requiredAs<double>("y");
      t.size = node.requiredAs<double>("size");
      t.weight = node.optional("weight");
      t.color = node.optional("color");
      t.text = node.text();
      return t;
    }

    SheetBoxDoc
    parseBox(const XmlNode& node)
    {
      SheetBoxDoc b;
      b.label = node.optional("label");
      b.x = node.requiredAs<double>("x");
      b.y = node.requiredAs<double>("y");
      b.w = node.requiredAs<double>("w");
      b.h = node.requiredAs<double>("h");
      b.fill = node.optional("fill");
      return b;
    }

    SheetTrackDoc
    parseTrack(const XmlNode& node)
    {
      SheetTrackDoc t;
      t.x = node.requiredAs<double>("x");
      t.y = node.requiredAs<double>("y");
      t.cellW = node.requiredAs<double>("cell-w");
      t.cellH = node.requiredAs<double>("cell-h");
      t.cells = node.required("cells");
      t.direction = node.optional("direction").value_or("h");
      checkEnum(node, "direction", t.direction, {"h", "v"});
      t.wrap = node.optionalAs<int>("wrap");
      t.fill = node.optional("fill");
      return t;
    }

    SheetPanelTableDoc
    parsePanelTable(const XmlNode& node)
    {
      SheetPanelTableDoc t;
      t.x = node.requiredAs<double>("x");
      t.y = node.requiredAs<double>("y");
      t.cellW = node.requiredAs<double>("cell-w");
      t.cellH = node.requiredAs<double>("cell-h");
      t.size = node.optionalAs<double>("size");
      for (const XmlNode& row : node.children("row")) {
        SheetPanelTableRowDoc r;
        for (const XmlNode& cell : row.children("cell")) {
          r.cells.push_back(cell.text());
        }
        t.rows.push_back(std::move(r));
      }
      return t;
    }

    SheetPanelDoc
    parsePanel(const XmlNode& node)
    {
      SheetPanelDoc p;
      p.id = node.required("id");
      p.title = node.optional("title");
      p.x = node.requiredAs<double>("x");
      p.y = node.requiredAs<double>("y");
      p.w = node.requiredAs<double>("w");
      p.h = node.requiredAs<double>("h");
      p.rotate = node.optionalAs<double>("rotate").value_or(0.0);
      p.fill = node.optional("fill");
      p.stroke = node.optional("stroke");
      p.sourceLine = node.line();
      for (const XmlNode& c : node.children()) {
        const std::string n = c.name();
        if ("text" == n) {
          p.texts.push_back(parsePanelText(c));
        } else if ("box" == n) {
          p.boxes.push_back(parseBox(c));
        } else if ("track" == n) {
          p.tracks.push_back(parseTrack(c));
        } else if ("table" == n) {
          p.tables.push_back(parsePanelTable(c));
        }
      }
      return p;
    }

  }  // namespace

  SheetDoc
  SheetDoc::parse(const XmlDocument& doc)
  {
    const XmlNode root = doc.root();
    if ("sheet" != root.name()) {
      throw std::invalid_argument(root.file() + ":" + std::to_string(root.line()) +
                                   ": expected root element 'sheet', found '" + root.name() + "'");
    }

    SheetDoc s;
    s.id = root.required("id");
    s.title = root.required("title");
    s.source = root.optional("source");
    s.width = root.requiredAs<double>("width");
    s.height = root.requiredAs<double>("height");
    s.background = root.required("background");
    s.font = root.optional("font").value_or(s.font);
    s.urban = root.required("urban");
    checkEnum(root, "urban", s.urban, {"buildings", "symbol"});
    s.junctions = root.required("junctions");
    checkEnum(root, "junctions", s.junctions, {"implicit", "explicit"});

    for (const XmlNode& g : root.children("grid")) {
      s.grids.push_back(parseGrid(g));
    }

    const XmlNode palette = requiredChild(root, "palette");
    for (const XmlNode& c : palette.children("color")) {
      s.palette.push_back(parseColor(c));
    }

    const XmlNode terrains = requiredChild(root, "terrains");
    for (const XmlNode& t : terrains.children("terrain")) {
      s.terrains.push_back(parseTerrain(t));
    }

    if (const std::optional<XmlNode> lines = root.child("lines")) {
      for (const XmlNode& l : lines->children("line")) {
        s.lines.push_back(parseLine(l));
      }
    }
    if (const std::optional<XmlNode> legend = root.child("legend")) {
      for (const XmlNode& m : legend->children("mark")) {
        s.legend.push_back(parseMark(m));
      }
    }

    for (const XmlNode& c : root.children()) {
      const std::string n = c.name();
      if ("hexes" == n) {
        s.hexesBulk.push_back(parseHexes(c));
      } else if ("hex" == n) {
        s.hexes.push_back(parseHex(c));
      } else if ("edge" == n) {
        s.edges.push_back(parseEdge(c));
      } else if ("path" == n) {
        s.paths.push_back(parsePath(c));
      } else if ("link" == n) {
        s.links.push_back(parseLink(c));
      } else if ("junction" == n) {
        s.junctionElements.push_back(parseJunction(c));
      } else if ("region" == n) {
        s.regions.push_back(parseRegion(c));
      } else if ("label" == n) {
        s.labels.push_back(parseLabel(c));
      } else if ("panel" == n) {
        s.panels.push_back(parsePanel(c));
      }
    }

    if ("implicit" == s.junctions && !s.junctionElements.empty()) {
      throw std::invalid_argument(root.file() + ":" + std::to_string(s.junctionElements.front().sourceLine) +
                                   ": a sheet with junctions=\"implicit\" may not contain junction elements");
    }
    return s;
  }

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
