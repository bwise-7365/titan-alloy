// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmaped/Document.h"

#include "hexmaped/Schema.h"
#include "hexmaped/SheetWriter.h"
#include "hexxml/XmlDocument.h"

#include <algorithm>
#include <cctype>
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

    bool
    chainIdTakenP(const HexXml::SheetDoc& s, const std::string& id)
    {
      const bool linkP = std::any_of(s.links.begin(), s.links.end(), [&](const HexXml::SheetLinkDoc& l) { return l.id == id; });
      const bool pathP = std::any_of(s.paths.begin(), s.paths.end(), [&](const HexXml::SheetPathDoc& p) { return p.id == id; });
      return linkP || pathP;
    }

    // "<id>-2", "<id>-3", ...: the first not already a link or path id
    std::string
    freshChainId(const HexXml::SheetDoc& s, const std::string& base)
    {
      int n = 2;
      while (chainIdTakenP(s, base + "-" + std::to_string(n))) {
        ++n;
      }
      return base + "-" + std::to_string(n);
    }

    // an ASCII xs:NCName: a letter or '_' first, then letters, digits, '-', '_', '.'
    bool
    idPrefixP(std::string_view text)
    {
      const auto startP = [](char ch) { return std::isalpha(static_cast<unsigned char>(ch)) || '_' == ch; };
      const auto restP = [](char ch) { return std::isalnum(static_cast<unsigned char>(ch)) || '-' == ch || '_' == ch || '.' == ch; };
      return !text.empty() && startP(text.front()) && std::all_of(text.begin(), text.end(), restP);
    }

    // "<kind>-new-1", "<kind>-new-2", ...: the first not already a link or path id
    std::string
    newChainId(const HexXml::SheetDoc& s, const std::string& kind)
    {
      int n = 1;
      while (chainIdTakenP(s, kind + "-new-" + std::to_string(n))) {
        ++n;
      }
      return kind + "-new-" + std::to_string(n);
    }

    // A cut chain's pieces keep their outer end reasons; the ends at the cut are "unexplained".
    void
    splitEnds(const HexXml::SheetLinkDoc& whole, HexXml::SheetLinkDoc& head, HexXml::SheetLinkDoc& tail)
    {
      if (!whole.ends.has_value()) {
        return;
      }
      const std::vector<std::string> pair = tokens(*whole.ends);
      if (2 != pair.size()) {
        throw std::invalid_argument("removeLinkStep: link ends \"" + *whole.ends + "\" is not a pair");
      }
      head.ends = pair[0] + " unexplained";
      tail.ends = "unexplained " + pair[1];
      return;
    }

    // A one-hex piece left by a cut: it has an "unexplained" end, or no ends. A one-hex link with explained
    // ends (a railway leaving the map, "place edge") is a real link and is never absorbed.
    bool
    remnantP(const HexXml::SheetLinkDoc& l)
    {
      return 1 == l.hexes.size() && (!l.ends.has_value() || std::string::npos != l.ends->find("unexplained"));
    }

    // After a step joins `chain` to `ends`, a remnant of the same kind and line on either hex is folded
    // into the chain: the junctions that named it name the chain, and a junction left joining the chain
    // only to itself goes.
    void
    absorbRemnants(HexXml::SheetDoc& s, std::size_t chain, const std::vector<std::string>& ends)
    {
      if (!s.links[chain].id.has_value()) {
        s.links[chain].id = newChainId(s, s.links[chain].kind);
      }
      const HexXml::SheetLinkDoc target = s.links[chain];  // kind, line and id; ends may change below
      std::vector<std::string> gone;
      for (std::size_t n = 0; n < s.links.size(); ++n) {
        const HexXml::SheetLinkDoc& l = s.links[n];
        const bool absorbP = n != chain && remnantP(l) && l.kind == target.kind && l.line == target.line &&
                             std::find(ends.begin(), ends.end(), l.hexes.front()) != ends.end();
        if (!absorbP) {
          continue;
        }
        if (l.id.has_value()) {
          gone.push_back(*l.id);
        }
        // the chain's end on the remnant's hex takes back the reason the cut left on the remnant
        HexXml::SheetLinkDoc& c = s.links[chain];
        if (l.ends.has_value() && c.ends.has_value()) {
          const std::vector<std::string> theirs = tokens(*l.ends);
          std::vector<std::string> mine = tokens(*c.ends);
          const std::string reason = "unexplained" != theirs.front() ? theirs.front() : theirs.back();
          if (2 == mine.size() && "unexplained" != reason) {
            if (c.hexes.front() == l.hexes.front()) {
              mine[0] = reason;
            }
            if (c.hexes.back() == l.hexes.front()) {
              mine[1] = reason;
            }
            c.ends = mine[0] + " " + mine[1];
          }
        }
      }
      for (HexXml::SheetJunctionDoc& j : s.junctionElements) {
        for (std::string& m : j.links) {
          if (std::find(gone.begin(), gone.end(), m) != gone.end()) {
            m = *target.id;
          }
        }
        std::vector<std::string> unique;
        for (const std::string& m : j.links) {
          if (std::find(unique.begin(), unique.end(), m) == unique.end()) {
            unique.push_back(m);
          }
        }
        j.links = unique;
      }
      s.junctionElements.erase(std::remove_if(s.junctionElements.begin(), s.junctionElements.end(),
                                              [](const HexXml::SheetJunctionDoc& j) { return j.paths.empty() && j.links.size() < 2; }),
                               s.junctionElements.end());
      s.links.erase(std::remove_if(s.links.begin(), s.links.end(),
                                   [&](const HexXml::SheetLinkDoc& l) {
                                     return remnantP(l) && l.kind == target.kind && l.line == target.line &&
                                            l.id != target.id &&
                                            std::find(ends.begin(), ends.end(), l.hexes.front()) != ends.end();
                                   }),
                    s.links.end());
      return;
    }

    // Junctions standing on the tail's hexes now name the tail's id; none is removed.
    void
    repointJunctions(HexXml::SheetDoc& s, const std::string& oldId, const HexXml::SheetLinkDoc& tail)
    {
      for (HexXml::SheetJunctionDoc& j : s.junctionElements) {
        const bool onTailP = std::find(tail.hexes.begin(), tail.hexes.end(), j.at) != tail.hexes.end();
        if (onTailP) {
          std::replace(j.links.begin(), j.links.end(), oldId, *tail.id);
        }
      }
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
    if (!idPrefixP(kind)) {
      throw std::invalid_argument("addLinkStep: link kind \"" + std::string(kind) + "\" cannot start an XML id (letters, digits, '-', '_', '.'; not starting with a digit)");
    }
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
      for (const HexXml::SheetLinkDoc& l : s.links) {
        if (l.kind != k || l.line != line) {
          continue;
        }
        for (std::size_t i = 0; i + 1 < l.hexes.size(); ++i) {
          if ((l.hexes[i] == ha && l.hexes[i + 1] == hb) || (l.hexes[i] == hb && l.hexes[i + 1] == ha)) {
            return;  // already a step of this chain
          }
        }
      }
      // extend a chain that ends at either hex (never a one-hex piece: that is absorbed below), or start one
      std::size_t chain = s.links.size();
      for (std::size_t n = 0; n < s.links.size() && chain == s.links.size(); ++n) {
        HexXml::SheetLinkDoc& l = s.links[n];
        if (l.kind != k || l.line != line || l.hexes.size() < 2) {
          continue;
        }
        if (l.hexes.back() == ha || l.hexes.back() == hb) {
          l.hexes.push_back(l.hexes.back() == ha ? hb : ha);
          chain = n;
        }
        else if (l.hexes.front() == ha || l.hexes.front() == hb) {
          l.hexes.insert(l.hexes.begin(), l.hexes.front() == ha ? hb : ha);
          chain = n;
        }
      }
      if (chain == s.links.size()) {
        HexXml::SheetLinkDoc l;
        l.id = newChainId(s, k);  // explicit-junction sheets require every link to have an id
        l.kind = k;
        l.line = line;
        l.hexes = {ha, hb};
        s.links.push_back(l);
      }
      absorbRemnants(s, chain, {ha, hb});
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
        splitEnds(l, head, tail);
        if (l.id.has_value()) {
          // the head keeps the id; a one-hex remnant is kept too (a map exit is a one-hex link)
          tail.id = freshChainId(s, *l.id);
          repointJunctions(s, *l.id, tail);
        }
        out.push_back(head);
        out.push_back(tail);
      }
      s.links = std::move(out);
    });
    return;
  }

  void
  Document::removeOneHexLinks(std::string_view hex)
  {
    requireHex(hex, "removeOneHexLinks");
    const std::string h(hex);
    const auto oneHexP = [&](const HexXml::SheetLinkDoc& l) { return 1 == l.hexes.size() && h == l.hexes.front(); };
    if (std::none_of(sheet_.links.begin(), sheet_.links.end(), oneHexP)) {
      throw std::invalid_argument("removeOneHexLinks: no one-hex link at " + h);
    }
    for (const HexXml::SheetLinkDoc& l : sheet_.links) {
      if (!oneHexP(l) || !l.id.has_value()) {
        continue;
      }
      for (const HexXml::SheetJunctionDoc& j : sheet_.junctionElements) {
        if (std::find(j.links.begin(), j.links.end(), *l.id) != j.links.end()) {
          throw std::invalid_argument("removeOneHexLinks: link '" + *l.id + "' at " + h + " is named by the junction at " +
                                      j.at + "; edit the junction first");
        }
      }
    }
    edit([&](HexXml::SheetDoc& s) {
      s.links.erase(std::remove_if(s.links.begin(), s.links.end(), oneHexP), s.links.end());
    });
    return;
  }

  namespace {

    // The link's end reason at `hex` (first or last hex) becomes `reason`, when the link has ends.
    void
    setEndAt(HexXml::SheetLinkDoc& l, const std::string& hex, const std::string& reason)
    {
      if (!l.ends.has_value() || l.hexes.empty()) {
        return;
      }
      std::vector<std::string> pair = tokens(*l.ends);
      if (2 != pair.size()) {
        throw std::invalid_argument("toggleJunction: link ends \"" + *l.ends + "\" is not a pair");
      }
      if (l.hexes.front() == hex) {
        pair[0] = reason;
      }
      if (l.hexes.back() == hex) {
        pair[1] = reason;
      }
      l.ends = pair[0] + " " + pair[1];
      return;
    }

  }  // namespace

  void
  Document::toggleJunction(std::string_view hex, std::string_view kind)
  {
    requireHex(hex, "toggleJunction");
    if ("explicit" != sheet_.junctions) {
      throw std::invalid_argument("toggleJunction: this sheet's junctions are implicit (every shared hex joins)");
    }
    const std::string h(hex);
    const std::string k(kind);
    const auto kindAt = [&](const HexXml::SheetLinkDoc& l) {
      return l.kind == k && std::find(l.hexes.begin(), l.hexes.end(), h) != l.hexes.end();
    };
    const auto existing = std::find_if(sheet_.junctionElements.begin(), sheet_.junctionElements.end(),
                                       [&](const HexXml::SheetJunctionDoc& j) {
                                         return j.at == h && std::any_of(sheet_.links.begin(), sheet_.links.end(), [&](const HexXml::SheetLinkDoc& l) {
                                                  return kindAt(l) && l.id.has_value() &&
                                                         std::find(j.links.begin(), j.links.end(), *l.id) != j.links.end();
                                                });
                                       });
    if (existing != sheet_.junctionElements.end()) {
      const std::vector<std::string> members = existing->links;
      edit([&](HexXml::SheetDoc& s) {
        s.junctionElements.erase(std::remove_if(s.junctionElements.begin(), s.junctionElements.end(),
                                                [&](const HexXml::SheetJunctionDoc& j) { return j.at == h && j.links == members; }),
                                 s.junctionElements.end());
        for (HexXml::SheetLinkDoc& l : s.links) {
          if (l.id.has_value() && std::find(members.begin(), members.end(), *l.id) != members.end()) {
            setEndAt(l, h, "unexplained");
          }
        }
      });
      return;
    }
    std::vector<std::string> members;
    for (const HexXml::SheetLinkDoc& l : sheet_.links) {
      if (!kindAt(l)) {
        continue;
      }
      if (!l.id.has_value()) {
        throw std::invalid_argument("toggleJunction: a " + k + " link through " + h + " has no id");
      }
      members.push_back(*l.id);
    }
    if (members.size() < 2) {
      throw std::invalid_argument("toggleJunction: fewer than two " + k + " links pass through " + h);
    }
    edit([&](HexXml::SheetDoc& s) {
      HexXml::SheetJunctionDoc j;
      j.at = h;
      j.links = members;
      for (const HexXml::SheetHexDoc& e : s.hexes) {
        if (e.id == h && e.name.has_value()) {
          j.name = e.name;
        }
      }
      s.junctionElements.push_back(j);
      for (HexXml::SheetLinkDoc& l : s.links) {
        if (l.id.has_value() && std::find(members.begin(), members.end(), *l.id) != members.end()) {
          setEndAt(l, h, "junction");
        }
      }
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
