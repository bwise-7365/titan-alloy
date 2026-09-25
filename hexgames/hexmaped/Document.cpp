// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmaped/Document.h"

#include "hexmaped/Schema.h"
#include "hexmaped/SheetWriter.h"
#include "hexxml/XmlDocument.h"

#include <algorithm>
#include <cmath>
#include <set>
#include <sstream>
#include <stdexcept>

namespace HexMapEd {

  namespace {

    std::vector<std::string>
    tokens(const std::string& text)
    {
      std::vector<std::string> out;
      std::istringstream in(text);
      std::string tok;
      while (in >> tok) {
        out.push_back(tok);
      }
      return out;
    }

    void
    eraseId(std::vector<std::string>& ids, std::string_view id)
    {
      ids.erase(std::remove(ids.begin(), ids.end(), std::string(id)), ids.end());
      return;
    }

  }  // namespace

  std::string
  Document::schemaLocationFor(const std::filesystem::path& file)
  {
    // hexsheet.xsd relative to the file: the schema lives in map_graphics/xml, found by walking up
    // from the file (a sheet saved under tools/reader/work/<map>/chain/ still names it correctly)
    std::error_code ec;
    std::filesystem::path dir = std::filesystem::absolute(file, ec).parent_path();
    for (std::filesystem::path up = dir; !up.empty() && up != up.root_path(); up = up.parent_path()) {
      const std::filesystem::path xsd = up / "map_graphics" / "xml" / "hexsheet.xsd";
      if (std::filesystem::exists(xsd, ec)) {
        const std::filesystem::path rel = std::filesystem::relative(xsd, dir, ec);
        return ec ? "hexsheet.xsd" : rel.generic_string();
      }
    }
    return "hexsheet.xsd";
  }

  Document
  Document::load(const std::filesystem::path& path)
  {
    return Document(HexXml::SheetDoc::parse(HexXml::XmlDocument::load(path)), path);
  }

  Document::Document(HexXml::SheetDoc sheet, std::filesystem::path path)
    : sheet_(std::move(sheet)), frame_(SheetFrame::of(sheet_)), path_(std::move(path))
  {
  }

  void
  Document::save()
  {
    if (path_.empty()) {
      throw std::invalid_argument("the document has no path: use saveAs");
    }
    writeSheet(path_, sheet_, schemaLocationFor(path_));
    dirtyP_ = false;
    return;
  }

  void
  Document::saveAs(const std::filesystem::path& path)
  {
    path_ = path;
    save();
    return;
  }

  // ---------------------------------------------------------------- queries

  std::string
  Document::terrainOf(std::string_view hex) const
  {
    for (const HexXml::SheetHexesDoc& bulk : sheet_.hexesBulk) {
      if (std::find(bulk.ids.begin(), bulk.ids.end(), std::string(hex)) != bulk.ids.end()) {
        return bulk.terrain;
      }
    }
    const HexXml::SheetHexDoc* h = hexElement(hex);
    if (nullptr != h && h->terrain.has_value()) {
      return *h->terrain;
    }
    return sheet_.grids.front().terrain;
  }

  const HexXml::SheetHexDoc*
  Document::hexElement(std::string_view hex) const
  {
    for (const HexXml::SheetHexDoc& h : sheet_.hexes) {
      if (h.id == hex) {
        return &h;
      }
    }
    return nullptr;
  }

  std::optional<std::string>
  Document::lineOfEdge(const Hexside& side) const
  {
    const std::string mine = frame_.token(side);
    std::optional<std::string> theirs;
    const std::optional<std::string> other = frame_.neighbour(side.hex, side.dir);
    if (other.has_value()) {
      theirs = frame_.token(Hexside{*other, HexCoord::opposite(side.dir)});
    }
    for (const HexXml::SheetEdgeDoc& e : sheet_.edges) {
      if (e.line.has_value() && (e.at == mine || (theirs.has_value() && e.at == *theirs))) {
        return e.line;
      }
    }
    return std::nullopt;
  }

  std::vector<std::string>
  Document::edgeTokensAt(std::string_view hex) const
  {
    std::vector<std::string> out;
    for (int k = 0; k < HexCoord::kDirections; ++k) {
      const Hexside side{std::string(hex), static_cast<HexCoord::Direction>(k)};
      const std::optional<std::string> line = lineOfEdge(side);
      if (line.has_value()) {
        out.push_back(frame_.token(side) + " " + *line);
      }
    }
    return out;
  }

  std::vector<std::string>
  Document::terrainIds() const
  {
    std::vector<std::string> out;
    for (const HexXml::SheetTerrainDoc& t : sheet_.terrains) {
      out.push_back(t.id);
    }
    return out;
  }

  std::vector<std::string>
  Document::lineIds() const
  {
    std::vector<std::string> out;
    for (const HexXml::SheetLineDoc& l : sheet_.lines) {
      out.push_back(l.id);
    }
    return out;
  }

  std::vector<std::string>
  Document::colourIds() const
  {
    std::vector<std::string> out;
    for (const HexXml::SheetColorDoc& c : sheet_.palette) {
      out.push_back(c.id);
    }
    return out;
  }

  std::vector<std::string>
  Document::linkKinds() const
  {
    std::set<std::string> kinds{"rail", "road"};
    for (const HexXml::SheetLinkDoc& l : sheet_.links) {
      kinds.insert(l.kind);
    }
    return std::vector<std::string>(kinds.begin(), kinds.end());
  }

  // ---------------------------------------------------------------- checks at the boundary

  void
  Document::requireHex(std::string_view hex, std::string_view where) const
  {
    Schema::requireHexId(hex, where);
    if (!frame_.printsP(hex)) {
      throw std::invalid_argument(std::string(where) + ": hex '" + std::string(hex) + "' is not on the sheet");
    }
    return;
  }

  void
  Document::requireTerrain(std::string_view id, std::string_view where) const
  {
    for (const HexXml::SheetTerrainDoc& t : sheet_.terrains) {
      if (t.id == id) {
        return;
      }
    }
    throw std::invalid_argument(std::string(where) + ": terrain '" + std::string(id) + "' is not declared by the sheet");
  }

  void
  Document::requireLine(std::string_view id, std::string_view where) const
  {
    for (const HexXml::SheetLineDoc& l : sheet_.lines) {
      if (l.id == id) {
        return;
      }
    }
    throw std::invalid_argument(std::string(where) + ": line '" + std::string(id) + "' is not declared by the sheet");
  }

  void
  Document::requireColour(std::string_view id, std::string_view where) const
  {
    for (const HexXml::SheetColorDoc& c : sheet_.palette) {
      if (c.id == id) {
        return;
      }
    }
    throw std::invalid_argument(std::string(where) + ": colour '" + std::string(id) + "' is not in the palette");
  }

  HexXml::SheetHexDoc&
  Document::hexElementFor(HexXml::SheetDoc& sheet, std::string_view hex) const
  {
    for (HexXml::SheetHexDoc& h : sheet.hexes) {
      if (h.id == hex) {
        return h;
      }
    }
    HexXml::SheetHexDoc h;
    h.id = std::string(hex);
    sheet.hexes.push_back(h);
    return sheet.hexes.back();
  }

  // ---------------------------------------------------------------- edits

  void
  Document::edit(const std::function<void(HexXml::SheetDoc&)>& apply)
  {
    HexXml::SheetDoc next = sheet_;
    apply(next);
    SheetFrame frame = SheetFrame::of(next);  // a bad clip is refused here, before anything changes
    undo_.push_back(std::move(sheet_));
    redo_.clear();
    sheet_ = std::move(next);
    frame_ = std::move(frame);
    dirtyP_ = true;
    return;
  }

  void
  Document::setTerrain(std::string_view hex, std::string_view terrainId)
  {
    requireHex(hex, "setTerrain");
    requireTerrain(terrainId, "setTerrain");
    const std::string id(hex);
    const std::string terrain(terrainId);
    edit([&](HexXml::SheetDoc& s) {
      for (HexXml::SheetHexesDoc& bulk : s.hexesBulk) {
        eraseId(bulk.ids, id);
      }
      for (HexXml::SheetHexDoc& h : s.hexes) {
        if (h.id == id) {
          h.terrain.reset();
        }
      }
      if (terrain == s.grids.front().terrain) {
        return;
      }
      for (HexXml::SheetHexesDoc& bulk : s.hexesBulk) {
        if (bulk.terrain == terrain) {
          bulk.ids.push_back(id);
          return;
        }
      }
      HexXml::SheetHexesDoc bulk;
      bulk.terrain = terrain;
      bulk.ids.push_back(id);
      s.hexesBulk.push_back(bulk);
    });
    return;
  }

  void
  Document::toggleEdge(const Hexside& side, std::string_view lineId)
  {
    requireHex(side.hex, "toggleEdge");
    requireLine(lineId, "toggleEdge");
    const std::string mine = frame_.token(side);
    std::optional<std::string> theirs;
    const std::optional<std::string> other = frame_.neighbour(side.hex, side.dir);
    if (other.has_value()) {
      theirs = frame_.token(Hexside{*other, HexCoord::opposite(side.dir)});
    }
    const std::string line(lineId);
    edit([&](HexXml::SheetDoc& s) {
      const auto sameSideP = [&](const HexXml::SheetEdgeDoc& e) {
        return e.line == line && (e.at == mine || (theirs.has_value() && e.at == *theirs));
      };
      const auto it = std::find_if(s.edges.begin(), s.edges.end(), sameSideP);
      if (it != s.edges.end()) {
        s.edges.erase(it);
        return;
      }
      HexXml::SheetEdgeDoc e;
      e.at = frame_.canonical(side);
      e.line = line;
      s.edges.push_back(e);
    });
    return;
  }

  void
  Document::addLinkStep(std::string_view a, std::string_view b, std::string_view kind, std::string_view lineId)
  {
    requireHex(a, "addLinkStep");
    requireHex(b, "addLinkStep");
    requireLine(lineId, "addLinkStep");
    bool adjacentP = false;
    for (int k = 0; k < HexCoord::kDirections; ++k) {
      if (frame_.neighbour(a, static_cast<HexCoord::Direction>(k)) == std::string(b)) {
        adjacentP = true;
      }
    }
    if (!adjacentP) {
      throw std::invalid_argument("addLinkStep: hexes " + std::string(a) + " and " + std::string(b) + " are not neighbours");
    }
    const std::string ha(a);
    const std::string hb(b);
    const std::string k(kind);
    const std::string line(lineId);
    edit([&](HexXml::SheetDoc& s) {
      for (HexXml::SheetLinkDoc& l : s.links) {
        if (l.kind != k || l.line != line) {
          continue;
        }
        for (std::size_t i = 0; i + 1 < l.hexes.size(); ++i) {
          if ((l.hexes[i] == ha && l.hexes[i + 1] == hb) || (l.hexes[i] == hb && l.hexes[i + 1] == ha)) {
            return;  // already a step of this chain
          }
        }
      }
      for (HexXml::SheetLinkDoc& l : s.links) {
        if (l.kind != k || l.line != line) {
          continue;
        }
        if (l.hexes.back() == ha) {
          l.hexes.push_back(hb);
          return;
        }
        if (l.hexes.back() == hb) {
          l.hexes.push_back(ha);
          return;
        }
        if (l.hexes.front() == ha) {
          l.hexes.insert(l.hexes.begin(), hb);
          return;
        }
        if (l.hexes.front() == hb) {
          l.hexes.insert(l.hexes.begin(), ha);
          return;
        }
      }
      HexXml::SheetLinkDoc l;
      l.kind = k;
      l.line = line;
      l.hexes = {ha, hb};
      s.links.push_back(l);
    });
    return;
  }

  void
  Document::removeLinkStep(std::string_view a, std::string_view b)
  {
    requireHex(a, "removeLinkStep");
    requireHex(b, "removeLinkStep");
    const std::string ha(a);
    const std::string hb(b);
    edit([&](HexXml::SheetDoc& s) {
      std::vector<HexXml::SheetLinkDoc> out;
      for (const HexXml::SheetLinkDoc& l : s.links) {
        std::size_t cut = l.hexes.size();
        for (std::size_t i = 0; i + 1 < l.hexes.size(); ++i) {
          if ((l.hexes[i] == ha && l.hexes[i + 1] == hb) || (l.hexes[i] == hb && l.hexes[i + 1] == ha)) {
            cut = i;
            break;
          }
        }
        if (cut == l.hexes.size()) {
          out.push_back(l);
          continue;
        }
        HexXml::SheetLinkDoc head = l;
        head.hexes.assign(l.hexes.begin(), l.hexes.begin() + static_cast<std::ptrdiff_t>(cut) + 1);
        HexXml::SheetLinkDoc tail = l;
        tail.hexes.assign(l.hexes.begin() + static_cast<std::ptrdiff_t>(cut) + 1, l.hexes.end());
        if (head.hexes.size() >= 2) {
          out.push_back(head);
        }
        if (tail.hexes.size() >= 2) {
          out.push_back(tail);
        }
      }
      s.links = std::move(out);
    });
    return;
  }

  void
  Document::setName(std::string_view hex, std::optional<std::string> name)
  {
    requireHex(hex, "setName");
    edit([&](HexXml::SheetDoc& s) {
      HexXml::SheetHexDoc& h = hexElementFor(s, hex);
      h.name = (name.has_value() && !name->empty()) ? name : std::nullopt;
    });
    return;
  }

  void
  Document::addGlyph(std::string_view hex, std::string_view symbol, std::string_view slot, std::optional<std::string> colour)
  {
    requireHex(hex, "addGlyph");
    Schema::requireSymbol(symbol, "addGlyph");
    Schema::requireSlot(slot, "addGlyph");
    if (colour.has_value()) {
      requireColour(*colour, "addGlyph");
    }
    edit([&](HexXml::SheetDoc& s) {
      HexXml::SheetGlyphDoc g;
      g.symbol = std::string(symbol);
      g.slot = std::string(slot);
      g.color = colour;
      hexElementFor(s, hex).glyphs.push_back(g);
    });
    return;
  }

  void
  Document::removeGlyph(std::string_view hex, std::size_t index)
  {
    requireHex(hex, "removeGlyph");
    const HexXml::SheetHexDoc* h = hexElement(hex);
    if (nullptr == h || index >= h->glyphs.size()) {
      throw std::invalid_argument("removeGlyph: hex '" + std::string(hex) + "' has no glyph " + std::to_string(index));
    }
    edit([&](HexXml::SheetDoc& s) {
      HexXml::SheetHexDoc& e = hexElementFor(s, hex);
      e.glyphs.erase(e.glyphs.begin() + static_cast<std::ptrdiff_t>(index));
    });
    return;
  }

  void
  Document::setRing(std::string_view hex, std::optional<std::string> colourId)
  {
    requireHex(hex, "setRing");
    if (colourId.has_value()) {
      requireColour(*colourId, "setRing");
    }
    edit([&](HexXml::SheetDoc& s) { hexElementFor(s, hex).ring = colourId; });
    return;
  }

  std::vector<std::string>
  Document::clippedIds(const HexXml::SheetGridDoc& g) const
  {
    std::vector<std::string> clipped;
    const HexCoord::Grid& grid = frame_.grids().front();
    for (const std::string& tok : tokens(g.clip.value_or(""))) {
      const std::size_t dash = tok.find('-');
      if (std::string::npos == dash) {
        clipped.push_back(tok);
        continue;
      }
      for (const HexCoord::HexId& r : grid.expandRange(HexCoord::HexId{tok.substr(0, dash)}, HexCoord::HexId{tok.substr(dash + 1)})) {
        clipped.push_back(r.text);
      }
    }
    return clipped;
  }

  namespace {

    HexCoord::Grid
    unclippedTwin(const HexCoord::Grid& grid)
    {
      HexCoord::GridSpec spec = grid.spec();
      spec.clip.clear();
      return HexCoord::Grid(spec);
    }

    std::string
    joined(const std::vector<std::string>& ids)
    {
      std::string text;
      for (const std::string& c : ids) {
        text += (text.empty() ? "" : " ") + c;
      }
      return text;
    }

  }  // namespace

  std::optional<std::string>
  Document::cellAt(Pixel p) const
  {
    const HexCoord::Grid twin = unclippedTwin(frame_.grids().front());
    const std::optional<HexCoord::HexCentre> c = twin.hexAt(p);
    if (!c.has_value()) {
      return std::nullopt;
    }
    const std::optional<HexCoord::GridIndex> index = twin.indexOf(*c);
    if (!index.has_value()) {
      return std::nullopt;
    }
    return twin.idOf(*index).text;
  }

  void
  Document::growTowards(HexXml::SheetGridDoc& g, Pixel p, const HexCoord::Grid& twin)
  {
    // one column or row in the direction of the pixel; the origin and the numbering move with it so
    // every existing cell keeps its pixel and its id, and the stagger parity flips when the origin
    // moves along the staggered axis
    const bool flat = "flat" == g.orientation;
    const double s = g.size;
    const double colPitch = flat ? 1.5 * s : std::sqrt(3.0) * s;
    const double rowPitch = flat ? std::sqrt(3.0) * s : 1.5 * s;
    const Pixel first = twin.pixelOf(twin.centreOf(HexCoord::GridIndex{0, 0}));
    const Pixel last = twin.pixelOf(twin.centreOf(HexCoord::GridIndex{g.cols - 1, g.rows - 1}));
    const auto flip = [&g] { g.offset = "odd" == g.offset ? "even" : "odd"; };
    if (p.x < first.x - 0.5 * colPitch) {
      g.ox -= colPitch;
      g.colStart -= g.colStep;
      g.cols += 1;
      if (flat) {
        flip();
      }
      return;
    }
    if (p.x > last.x + 0.5 * colPitch) {
      g.cols += 1;
      return;
    }
    if (p.y < first.y - 0.5 * rowPitch) {
      g.oy -= rowPitch;
      g.rowStart -= g.rowStep;
      g.rows += 1;
      if (!flat) {
        flip();
      }
      return;
    }
    if (p.y > last.y + 0.5 * rowPitch) {
      g.rows += 1;
      return;
    }
    throw std::invalid_argument("addHexAt: the point is inside the grid's rectangle but on no cell");
  }

  std::string
  Document::addHexAt(Pixel p)
  {
    const std::optional<std::string> printed = frame_.hexAt(p);
    if (printed.has_value()) {
      return *printed;  // already printed: no edit
    }
    std::string added;
    edit([&](HexXml::SheetDoc& s) {
      HexXml::SheetGridDoc& g = s.grids.front();
      std::vector<std::string> clipped = clippedIds(g);
      HexCoord::Grid twin = unclippedTwin(frame_.grids().front());
      std::optional<std::string> id = cellAt(p);
      for (int attempt = 0; !id.has_value() && attempt < 4; ++attempt) {
        const std::vector<HexCoord::HexId> before = twin.ids();
        growTowards(g, p, twin);
        HexCoord::GridSpec spec = twin.spec();
        spec.cols = g.cols;
        spec.rows = g.rows;
        spec.ox = g.ox;
        spec.oy = g.oy;
        spec.colStart = g.colStart;
        spec.rowStart = g.rowStart;
        spec.offset = "odd" == g.offset ? HexCoord::Parity::Odd : HexCoord::Parity::Even;
        twin = HexCoord::Grid(spec);
        std::set<std::string> old;
        for (const HexCoord::HexId& h : before) {
          old.insert(h.text);
        }
        for (const HexCoord::HexId& h : twin.ids()) {
          if (0 == old.count(h.text)) {
            clipped.push_back(h.text);  // the new column or row starts clipped
          }
        }
        const std::optional<HexCoord::HexCentre> c = twin.hexAt(p);
        if (c.has_value()) {
          const std::optional<HexCoord::GridIndex> index = twin.indexOf(*c);
          if (index.has_value()) {
            id = twin.idOf(*index).text;
          }
        }
      }
      if (!id.has_value()) {
        throw std::invalid_argument("addHexAt: no cell of the grid is at that point");
      }
      clipped.erase(std::remove(clipped.begin(), clipped.end(), *id), clipped.end());
      g.clip = clipped.empty() ? std::nullopt : std::optional<std::string>(joined(clipped));
      added = *id;
    });
    return added;
  }

  void
  Document::toggleClip(std::string_view hex)
  {
    Schema::requireHexId(hex, "toggleClip");
    const std::string id(hex);
    edit([&](HexXml::SheetDoc& s) {
      HexXml::SheetGridDoc& g = s.grids.front();
      std::vector<std::string> clipped = clippedIds(g);
      const auto it = std::find(clipped.begin(), clipped.end(), id);
      if (it != clipped.end()) {
        clipped.erase(it);
      } else {
        if (!frame_.printsP(id)) {
          throw std::invalid_argument("toggleClip: hex '" + id + "' is neither printed nor clipped");
        }
        clipped.push_back(id);
        for (HexXml::SheetHexesDoc& bulk : s.hexesBulk) {
          eraseId(bulk.ids, id);
        }
      }
      g.clip = clipped.empty() ? std::nullopt : std::optional<std::string>(joined(clipped));
    });
    return;
  }

  void
  Document::undo()
  {
    if (undo_.empty()) {
      return;
    }
    redo_.push_back(std::move(sheet_));
    sheet_ = std::move(undo_.back());
    undo_.pop_back();
    frame_ = SheetFrame::of(sheet_);
    dirtyP_ = true;
    return;
  }

  void
  Document::redo()
  {
    if (redo_.empty()) {
      return;
    }
    undo_.push_back(std::move(sheet_));
    sheet_ = std::move(redo_.back());
    redo_.pop_back();
    frame_ = SheetFrame::of(sheet_);
    dirtyP_ = true;
    return;
  }

}  // namespace HexMapEd
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
