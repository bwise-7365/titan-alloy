// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmaped/SheetWriter.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string_view>

namespace HexMapEd {

  namespace {

    std::string
    escaped(std::string_view text)
    {
      std::string out;
      out.reserve(text.size());
      for (const char c : text) {
        switch (c) {
          case '&': out += "&amp;"; break;
          case '<': out += "&lt;"; break;
          case '>': out += "&gt;"; break;
          case '"': out += "&quot;"; break;
          default: out += c; break;
        }
      }
      return out;
    }

    std::string
    num(double v)
    {
      if (std::floor(v) == v && std::fabs(v) < 1e9) {
        return std::to_string(static_cast<long long>(v));
      }
      char buf[32];
      std::snprintf(buf, sizeof buf, "%.3f", v);
      std::string s(buf);
      while (!s.empty() && '0' == s.back()) {
        s.pop_back();
      }
      if (!s.empty() && '.' == s.back()) {
        s.pop_back();
      }
      return s;
    }

    std::string
    join(const std::vector<std::string>& items)
    {
      std::string out;
      for (const std::string& item : items) {
        if (!out.empty()) {
          out += ' ';
        }
        out += item;
      }
      return out;
    }

    // One element's attributes, in the order they are added; an absent optional adds nothing.
    class Attrs {
    public:
      Attrs& add(std::string_view name, std::string_view value)
      {
        text_ += ' ';
        text_ += name;
        text_ += "=\"";
        text_ += escaped(value);
        text_ += '"';
        return *this;
      }
      Attrs& add(std::string_view name, const std::string& value) { return add(name, std::string_view(value)); }
      Attrs& add(std::string_view name, const char* value) { return add(name, std::string_view(value)); }
      Attrs& add(std::string_view name, const std::optional<std::string>& value)
      {
        if (value.has_value()) {
          add(name, *value);
        }
        return *this;
      }
      Attrs& add(std::string_view name, double value) { return add(name, num(value)); }
      Attrs& add(std::string_view name, int value) { return add(name, std::to_string(value)); }
      Attrs& add(std::string_view name, const std::optional<double>& value)
      {
        if (value.has_value()) {
          add(name, *value);
        }
        return *this;
      }
      Attrs& add(std::string_view name, const std::optional<int>& value)
      {
        if (value.has_value()) {
          add(name, *value);
        }
        return *this;
      }
      Attrs& flag(std::string_view name, bool value, bool schemaDefault)
      {
        if (value != schemaDefault) {
          add(name, value ? "true" : "false");
        }
        return *this;
      }
      Attrs& unlessDefault(std::string_view name, const std::string& value, std::string_view schemaDefault)
      {
        if (value != schemaDefault) {
          add(name, value);
        }
        return *this;
      }
      Attrs& unlessDefault(std::string_view name, double value, double schemaDefault)
      {
        if (value != schemaDefault) {
          add(name, value);
        }
        return *this;
      }
      Attrs& unlessDefault(std::string_view name, int value, int schemaDefault)
      {
        if (value != schemaDefault) {
          add(name, value);
        }
        return *this;
      }
      const std::string& text() const { return text_; }

    private:
      std::string text_;
    };

    void
    line(std::string& out, int depth, std::string_view text)
    {
      out.append(static_cast<std::size_t>(2 * depth), ' ');
      out += text;
      out += '\n';
      return;
    }

    void
    emptyElement(std::string& out, int depth, std::string_view name, const Attrs& a)
    {
      line(out, depth, "<" + std::string(name) + a.text() + "/>");
      return;
    }

    void
    writeGrid(std::string& out, const HexXml::SheetGridDoc& g)
    {
      Attrs a;
      a.add("id", g.id)
        .add("orientation", g.orientation)
        .add("offset", g.offset)
        .add("cols", g.cols)
        .add("rows", g.rows)
        .add("size", g.size)
        .add("ox", g.ox)
        .add("oy", g.oy)
        .add("id-format", g.idFormat)
        .unlessDefault("col-start", g.colStart, 1)
        .unlessDefault("row-start", g.rowStart, 1)
        .unlessDefault("col-step", g.colStep, 1)
        .unlessDefault("row-step", g.rowStep, 1)
        .add("terrain", g.terrain)
        .flag("show-ids", g.showIds, true)
        .unlessDefault("id-side", g.idSide, "w")
        .add("clip", g.clip);
      emptyElement(out, 1, "grid", a);
      return;
    }

    void
    writeStyle(std::string& out, const HexXml::SheetDoc& s)
    {
      line(out, 1, "<palette>");
      for (const HexXml::SheetColorDoc& c : s.palette) {
        emptyElement(out, 2, "color", Attrs().add("id", c.id).add("value", c.value).add("name", c.name));
      }
      line(out, 1, "</palette>");
      line(out, 1, "<terrains>");
      for (const HexXml::SheetTerrainDoc& t : s.terrains) {
        emptyElement(out, 2, "terrain",
              Attrs().add("id", t.id).add("name", t.name).add("fill", t.fill).add("stroke", t.stroke).unlessDefault("pattern", t.pattern, "none"));
      }
      line(out, 1, "</terrains>");
      if (!s.lines.empty()) {
        line(out, 1, "<lines>");
        for (const HexXml::SheetLineDoc& l : s.lines) {
          emptyElement(out, 2, "line",
                Attrs()
                  .add("id", l.id)
                  .add("stroke", l.stroke)
                  .add("width", l.width)
                  .add("dash", l.dash)
                  .add("casing", l.casing)
                  .add("casing-width", l.casingWidth)
                  .flag("ticks", l.ticks, false)
                  .add("opacity", l.opacity));
        }
        line(out, 1, "</lines>");
      }
      if (!s.legend.empty()) {
        line(out, 1, "<legend>");
        for (const HexXml::SheetMarkDoc& m : s.legend) {
          emptyElement(out, 2, "mark",
                       Attrs().add("id", m.id).add("name", m.name).add("shape", m.shape).add("color", m.color).add("size", m.size).add("pictogram", m.pictogram).add("across", m.across).add("from", m.from));
        }
        line(out, 1, "</legend>");
      }
      return;
    }

    void
    writeHex(std::string& out, const HexXml::SheetHexDoc& h)
    {
      Attrs a;
      a.add("id", h.id).add("terrain", h.terrain).add("ring", h.ring).add("ring-width", h.ringWidth).add("name", h.name);
      if (h.glyphs.empty() && h.sides.empty()) {
        emptyElement(out, 1, "hex", a);
        return;
      }
      std::string inner;
      for (const HexXml::SheetGlyphDoc& g : h.glyphs) {
        inner += "<glyph" +
                 Attrs()
                   .add("symbol", g.symbol)
                   .add("mark", g.mark)
                   .unlessDefault("slot", g.slot, "c")
                   .add("color", g.color)
                   .add("text", g.text)
                   .add("dir", g.dir)
                   .unlessDefault("scale", g.scale, 1.0)
                   .text() +
                 "/>";
      }
      for (const HexXml::SheetSideGlyphDoc& sg : h.sides) {
        inner += "<side" + Attrs().add("dir", sg.dir).add("symbol", sg.symbol).add("color", sg.color).text() + "/>";
      }
      line(out, 1, "<hex" + a.text() + ">" + inner + "</hex>");
      return;
    }

    void
    writeLabel(std::string& out, int depth, const HexXml::SheetLabelDoc& l)
    {
      emptyElement(out, depth, "label",
            Attrs()
              .add("text", l.text)
              .add("at", l.at)
              .unlessDefault("slot", l.slot, "c")
              .add("x", l.x)
              .add("y", l.y)
              .add("path", l.path)
              .unlessDefault("angle", l.angle, 0.0)
              .add("size", l.size)
              .add("color", l.color)
              .unlessDefault("weight", l.weight, "normal")
              .flag("italic", l.italic, false)
              .unlessDefault("spacing", l.spacing, 0.0)
              .flag("halo", l.halo, false)
              .unlessDefault("anchor", l.anchor, "middle"));
      return;
    }

    void
    writePanel(std::string& out, const HexXml::SheetPanelDoc& p)
    {
      Attrs a;
      a.add("id", p.id)
        .add("title", p.title)
        .add("x", p.x)
        .add("y", p.y)
        .add("w", p.w)
        .add("h", p.h)
        .unlessDefault("rotate", p.rotate, 0.0)
        .add("fill", p.fill)
        .add("stroke", p.stroke);
      line(out, 1, "<panel" + a.text() + ">");
      for (const HexXml::SheetPanelTextDoc& t : p.texts) {
        line(out, 2,
             "<text" + Attrs().add("x", t.x).add("y", t.y).add("size", t.size).add("weight", t.weight).add("color", t.color).text() +
               ">" + escaped(t.text) + "</text>");
      }
      for (const HexXml::SheetBoxDoc& b : p.boxes) {
        emptyElement(out, 2, "box", Attrs().add("label", b.label).add("x", b.x).add("y", b.y).add("w", b.w).add("h", b.h).add("fill", b.fill));
      }
      for (const HexXml::SheetTrackDoc& t : p.tracks) {
        emptyElement(out, 2, "track",
              Attrs()
                .add("x", t.x)
                .add("y", t.y)
                .add("cell-w", t.cellW)
                .add("cell-h", t.cellH)
                .add("cells", t.cells)
                .unlessDefault("direction", t.direction, "h")
                .add("wrap", t.wrap)
                .add("fill", t.fill));
      }
      for (const HexXml::SheetPanelTableDoc& t : p.tables) {
        line(out, 2, "<table" + Attrs().add("x", t.x).add("y", t.y).add("cell-w", t.cellW).add("cell-h", t.cellH).add("size", t.size).text() + ">");
        for (const HexXml::SheetPanelTableRowDoc& r : t.rows) {
          std::string cells;
          for (const std::string& c : r.cells) {
            cells += "<cell>" + escaped(c) + "</cell>";
          }
          line(out, 3, "<row>" + cells + "</row>");
        }
        line(out, 2, "</table>");
      }
      line(out, 1, "</panel>");
      return;
    }

  }  // namespace

  std::string
  writeSheet(const HexXml::SheetDoc& s, std::string_view schemaLocation)
  {
    std::string out;
    out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    Attrs root;
    root.add("xmlns:xsi", "http://www.w3.org/2001/XMLSchema-instance")
      .add("xsi:noNamespaceSchemaLocation", schemaLocation)
      .add("id", s.id)
      .add("title", s.title)
      .add("source", s.source)
      .add("width", s.width)
      .add("height", s.height)
      .add("background", s.background)
      .unlessDefault("font", s.font, "Arial, Helvetica, sans-serif")
      .add("urban", s.urban)
      .add("junctions", s.junctions);
    line(out, 0, "<sheet" + root.text() + ">");
    line(out, 1, "<!-- written by HexMapEd; the structure and its style, validated against hexsheet.xsd -->");
    for (const HexXml::SheetGridDoc& g : s.grids) {
      writeGrid(out, g);
    }
    writeStyle(out, s);
    for (const HexXml::SheetHexesDoc& h : s.hexesBulk) {
      if (!h.ids.empty()) {
        emptyElement(out, 1, "hexes", Attrs().add("terrain", h.terrain).add("ids", join(h.ids)));
      }
    }
    for (const HexXml::SheetEdgeDoc& e : s.edges) {
      emptyElement(out, 1, "edge", Attrs().add("at", e.at).add("line", e.line).add("symbol", e.symbol).add("mark", e.mark).add("color", e.color).add("label", e.label));
    }
    for (const HexXml::SheetPathDoc& p : s.paths) {
      emptyElement(out, 1, "path",
            Attrs().add("id", p.id).add("kind", p.kind).add("name", p.name).add("line", p.line).add("edges", join(p.edges)).unlessDefault("offset", p.offset, 0.0).add("ends", p.ends));
    }
    for (const HexXml::SheetLinkDoc& l : s.links) {
      if (l.hexes.size() >= 2) {
        emptyElement(out, 1, "link", Attrs().add("id", l.id).add("kind", l.kind).add("name", l.name).add("line", l.line).add("hexes", join(l.hexes)).add("owner", l.owner).add("ends", l.ends));
      }
    }
    for (const HexXml::SheetJunctionDoc& j : s.junctionElements) {
      Attrs a;
      a.add("at", j.at);
      if (!j.links.empty()) {
        a.add("links", join(j.links));
      }
      if (!j.paths.empty()) {
        a.add("paths", join(j.paths));
      }
      a.add("name", j.name);
      emptyElement(out, 1, "junction", a);
    }
    for (const HexXml::SheetRegionDoc& r : s.regions) {
      emptyElement(out, 1, "region",
            Attrs()
              .add("layer", r.layer)
              .add("name", r.name)
              .add("hexes", join(r.hexes))
              .add("tint", r.tint)
              .unlessDefault("opacity", r.opacity, 0.35)
              .add("outline", r.outline)
              .add("label", r.label)
              .add("label-at", r.labelAt));
    }
    for (const HexXml::SheetHexDoc& h : s.hexes) {
      writeHex(out, h);
    }
    for (const HexXml::SheetLabelDoc& l : s.labels) {
      writeLabel(out, 1, l);
    }
    for (const HexXml::SheetPanelDoc& p : s.panels) {
      writePanel(out, p);
    }
    line(out, 0, "</sheet>");
    return out;
  }

  void
  writeSheet(const std::filesystem::path& path, const HexXml::SheetDoc& sheet, std::string_view schemaLocation)
  {
    const std::string text = writeSheet(sheet, schemaLocation);
    std::ofstream fh(path, std::ios::binary);
    if (!fh) {
      throw std::invalid_argument("cannot write " + path.string());
    }
    fh << text;
    if (!fh) {
      throw std::invalid_argument("write failed: " + path.string());
    }
    return;
  }

}  // namespace HexMapEd
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
