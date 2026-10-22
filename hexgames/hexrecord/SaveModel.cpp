// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The canonical writer: fixed attribute order (as hexsave.xsd declares each element), units and
// control hexes sorted by @id, moves sorted by @n, two-space indent, LF endings, no trailing
// whitespace. A pure function of the model, so two equal models write identical bytes.
// ----------------------------------------------
#include "hexrecord/SaveModel.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace HexRecord {

  namespace {

    std::string
    escapeText(const std::string& s)
    {
      std::string out;
      out.reserve(s.size());
      for (char c : s) {
        switch (c) {
          case '&':
            out += "&amp;";
            break;
          case '<':
            out += "&lt;";
            break;
          case '>':
            out += "&gt;";
            break;
          default:
            out += c;
            break;
        }
      }
      return out;
    }

    std::string
    escapeAttr(const std::string& s)
    {
      const std::string escaped = escapeText(s);
      std::string quoted;
      quoted.reserve(escaped.size());
      for (char c : escaped) {
        if ('"' == c) {
          quoted += "&quot;";
        }
        else {
          quoted += c;
        }
      }
      return quoted;
    }

    std::string
    joinTokens(const std::vector<std::string>& tokens)
    {
      std::string out;
      for (std::size_t i = 0; i < tokens.size(); ++i) {
        if (0 != i) {
          out += ' ';
        }
        out += tokens[i];
      }
      return out;
    }

    std::string
    indentOf(int level)
    {
      return std::string(static_cast<std::size_t>(level) * 2, ' ');
    }

    // Accumulates one tag's attribute text, in the caller's declared order.
    class AttrList {
    public:
      void req(const char* name, const std::string& value)
      {
        text_ += ' ';
        text_ += name;
        text_ += "=\"";
        text_ += escapeAttr(value);
        text_ += '"';
        return;
      }
      void req(const char* name, std::uint64_t value) { req(name, std::to_string(value)); return; }
      void req(const char* name, int value) { req(name, std::to_string(value)); return; }
      void req(const char* name, bool value)
      {
        req(name, value ? std::string("true") : std::string("false"));
        return;
      }
      void opt(const char* name, const std::optional<std::string>& value)
      {
        if (value.has_value()) {
          req(name, *value);
        }
        return;
      }
      void opt(const char* name, const std::optional<int>& value)
      {
        if (value.has_value()) {
          req(name, *value);
        }
        return;
      }
      void opt(const char* name, const std::optional<bool>& value)
      {
        if (value.has_value()) {
          req(name, *value);
        }
        return;
      }
      void optTokens(const char* name, const std::vector<std::string>& tokens)
      {
        if (!tokens.empty()) {
          req(name, joinTokens(tokens));
        }
        return;
      }
      const std::string& text() const { return text_; }

    private:
      std::string text_;
    };

    void
    emitLeaf(std::string& out, int level, const char* name, const AttrList& attrs, const std::string& text)
    {
      out += indentOf(level);
      out += '<';
      out += name;
      out += attrs.text();
      if (text.empty()) {
        out += "/>\n";
      }
      else {
        out += '>';
        out += escapeText(text);
        out += "</";
        out += name;
        out += ">\n";
      }
      return;
    }

    void
    emitFile(std::string& out, int level, const SaveFile& f)
    {
      AttrList a;
      a.req("role", f.role);
      a.req("path", f.path);
      a.opt("sha256", f.sha256);
      emitLeaf(out, level, "file", a, "");
      return;
    }

    void
    emitSide(std::string& out, int level, const SaveSide& s)
    {
      AttrList a;
      a.req("id", s.id);
      if (s.registers.empty() && s.flags.empty()) {
        emitLeaf(out, level, "side", a, "");
        return;
      }
      out += indentOf(level);
      out += "<side";
      out += a.text();
      out += ">\n";
      for (const SaveRegister& r : s.registers) {
        AttrList ra;
        ra.req("track", r.track);
        ra.req("value", r.value);
        emitLeaf(out, level + 1, "register", ra, "");
      }
      for (const SaveFlag& f : s.flags) {
        AttrList fa;
        fa.req("name", f.name);
        fa.req("value", f.value);
        emitLeaf(out, level + 1, "flag", fa, "");
      }
      out += indentOf(level);
      out += "</side>\n";
      return;
    }

    void
    emitUnit(std::string& out, int level, const SaveUnit& u)
    {
      AttrList a;
      a.req("id", u.id);
      a.req("counter", u.counter);
      a.req("type", u.type);
      a.req("owner", u.owner);
      a.opt("hex", u.hex);
      a.opt("space", u.space);
      a.req("face", u.face);
      a.opt("steps", u.steps);
      a.optTokens("status", u.status);
      a.opt("attached", u.attached);
      a.opt("revealed", u.revealedP);
      a.req("moved", u.movedP);
      a.opt("delay", u.delay);
      emitLeaf(out, level, "unit", a, u.text);
      return;
    }

    void
    emitControlHex(std::string& out, int level, const SaveControlHex& h)
    {
      AttrList a;
      a.req("id", h.id);
      a.req("side", h.side);
      emitLeaf(out, level, "hex", a, "");
      return;
    }

    void
    emitControlLink(std::string& out, int level, const SaveControlLink& l)
    {
      AttrList a;
      a.req("network", l.network);
      a.req("hexes", joinTokens(l.hexes));
      a.req("side", l.side);
      emitLeaf(out, level, "link", a, "");
      return;
    }

    void
    emitRegion(std::string& out, int level, const SaveRegion& r)
    {
      AttrList a;
      a.req("layer", r.layer);
      a.req("id", r.id);
      a.opt("status", r.status);
      a.opt("alignment", r.alignment);
      a.opt("posture", r.posture);
      a.opt("owner", r.owner);
      emitLeaf(out, level, "region", a, "");
      return;
    }

    void
    emitAsk(std::string& out, int level, const SaveAsk& k)
    {
      AttrList a;
      a.req("what", k.what);
      a.opt("side", k.side);
      a.opt("unit", k.unit);
      a.optTokens("candidates", k.candidates);
      a.opt("count", k.count);
      a.opt("may-stop", k.mayStopP);
      a.opt("deck", k.deck);
      a.opt("verb", k.verb);
      a.optTokens("options", k.options);
      emitLeaf(out, level, "ask", a, "");
      return;
    }

    void
    emitOwe(std::string& out, int level, const SaveOwe& o)
    {
      AttrList a;
      a.req("kind", o.kind);
      a.opt("side", o.side);
      a.opt("count", o.count);
      a.opt("fewest", o.fewest);
      a.opt("most", o.most);
      a.opt("unit", o.unit);
      a.opt("from", o.from);
      a.optTokens("path", o.path);
      a.opt("router", o.router);
      a.optTokens("involved", o.involved);
      a.opt("name", o.name);
      if (o.args.empty() && !o.ask.has_value()) {
        emitLeaf(out, level, "owe", a, "");
        return;
      }
      out += indentOf(level);
      out += "<owe";
      out += a.text();
      out += ">\n";
      for (const SaveArg& arg : o.args) {
        AttrList aa;
        aa.req("name", arg.name);
        aa.req("value", arg.value);
        emitLeaf(out, level + 1, "arg", aa, "");
      }
      if (o.ask.has_value()) {
        emitAsk(out, level + 1, *o.ask);
      }
      out += indentOf(level);
      out += "</owe>\n";
      return;
    }

    void
    emitPile(std::string& out, int level, const SavePile& p)
    {
      AttrList a;
      a.req("randomizer", p.randomizer);
      a.req("kind", p.kind);
      a.opt("side", p.side);
      a.req("cards", joinTokens(p.cards));
      a.opt("next", p.next);
      emitLeaf(out, level, "pile", a, "");
      return;
    }

    void
    emitStream(std::string& out, int level, const SaveStream& s)
    {
      AttrList a;
      a.req("tag", s.tag);
      a.req("draws", s.draws);
      a.opt("check", s.check);
      emitLeaf(out, level, "stream", a, "");
      return;
    }

    void
    emitMove(std::string& out, int level, const SaveMove& m)
    {
      AttrList a;
      a.req("n", m.n);
      a.req("turn", m.turn);
      a.req("phase", m.phase);
      a.req("side", m.side);
      a.req("by", m.by);
      a.req("cmd", m.cmd);
      a.optTokens("units", m.units);
      a.opt("from", m.from);
      a.opt("to", m.to);
      a.optTokens("path", m.path);
      a.opt("target", m.target);
      a.opt("mode", m.mode);
      a.optTokens("modifiers", m.modifiers);
      a.opt("card", m.card);
      a.opt("value", m.value);
      a.opt("choice", m.choice);

      const bool anyChildrenP = !m.args.empty() || m.result.has_value() || !m.draws.empty() || !m.events.empty();
      if (!anyChildrenP) {
        emitLeaf(out, level, "move", a, "");
        return;
      }
      out += indentOf(level);
      out += "<move";
      out += a.text();
      out += ">\n";
      for (const SaveArg& arg : m.args) {
        AttrList aa;
        aa.req("name", arg.name);
        aa.req("value", arg.value);
        emitLeaf(out, level + 1, "arg", aa, "");
      }
      if (m.result.has_value()) {
        AttrList ra;
        ra.req("outcome", m.result->outcome);
        ra.opt("odds", m.result->odds);
        ra.opt("column", m.result->column);
        ra.opt("drm", m.result->drm);
        emitLeaf(out, level + 1, "result", ra, m.result->text);
      }
      for (const SaveDraw& d : m.draws) {
        AttrList da;
        da.opt("stream", d.stream);
        da.opt("n", d.n);
        da.opt("value", d.value);
        da.opt("randomizer", d.randomizer);
        da.opt("card", d.card);
        emitLeaf(out, level + 1, "draw", da, "");
      }
      for (const SaveEvent& e : m.events) {
        AttrList ea;
        ea.req("kind", e.kind);
        ea.opt("unit", e.unit);
        ea.opt("hex", e.hex);
        ea.opt("side", e.side);
        ea.opt("value", e.value);
        emitLeaf(out, level + 1, "event", ea, e.text);
      }
      out += indentOf(level);
      out += "</move>\n";
      return;
    }

    void
    emitNote(std::string& out, int level, const SaveNote& n)
    {
      AttrList a;
      a.opt("n", n.n);
      emitLeaf(out, level, "note", a, n.text);
      return;
    }

  }  // namespace

  std::string
  canonicalText(const SaveModel& m)
  {
    std::string out;
    out += "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";

    AttrList root;
    root.req("xmlns:xsi", std::string("http://www.w3.org/2001/XMLSchema-instance"));
    root.req("xsi:noNamespaceSchemaLocation", m.schemaLocation);
    root.req("format", std::string("hexsave-1.0"));
    root.req("kind", m.kind);
    root.req("game", m.game);
    root.req("package", m.package);
    root.opt("scenario", m.scenario);
    root.req("seed", m.seed);
    root.opt("engine", m.engine);
    root.opt("created", m.created);
    root.opt("title", m.title);
    out += "<save";
    out += root.text();
    out += ">\n";

    if (!m.files.empty()) {
      out += indentOf(1);
      out += "<package>\n";
      for (const SaveFile& f : m.files) {
        emitFile(out, 2, f);
      }
      out += indentOf(1);
      out += "</package>\n";
    }

    {
      AttrList c;
      c.req("turn", m.cursor.turn);
      c.req("phase", m.cursor.phase);
      c.opt("side", m.cursor.side);
      c.req("moves", m.cursor.moves);
      c.req("over", m.cursor.overP);
      c.opt("winner", m.cursor.winner);
      emitLeaf(out, 1, "cursor", c, "");
    }

    if (!m.sides.empty()) {
      out += indentOf(1);
      out += "<sides>\n";
      for (const SaveSide& s : m.sides) {
        emitSide(out, 2, s);
      }
      out += indentOf(1);
      out += "</sides>\n";
    }

    if (!m.units.empty()) {
      std::vector<SaveUnit> sorted = m.units;
      std::sort(sorted.begin(), sorted.end(), [](const SaveUnit& a, const SaveUnit& b) { return a.id < b.id; });
      out += indentOf(1);
      out += "<units>\n";
      for (const SaveUnit& u : sorted) {
        emitUnit(out, 2, u);
      }
      out += indentOf(1);
      out += "</units>\n";
    }

    if (!m.controlHexes.empty() || !m.controlLinks.empty()) {
      std::vector<SaveControlHex> sortedHexes = m.controlHexes;
      std::sort(sortedHexes.begin(), sortedHexes.end(),
                [](const SaveControlHex& a, const SaveControlHex& b) { return a.id < b.id; });
      out += indentOf(1);
      out += "<control>\n";
      for (const SaveControlHex& h : sortedHexes) {
        emitControlHex(out, 2, h);
      }
      for (const SaveControlLink& l : m.controlLinks) {
        emitControlLink(out, 2, l);
      }
      out += indentOf(1);
      out += "</control>\n";
    }

    if (!m.regions.empty()) {
      out += indentOf(1);
      out += "<regions>\n";
      for (const SaveRegion& r : m.regions) {
        emitRegion(out, 2, r);
      }
      out += indentOf(1);
      out += "</regions>\n";
    }

    if (!m.resolution.empty()) {
      out += indentOf(1);
      out += "<resolution>\n";
      for (const SaveOwe& o : m.resolution) {
        emitOwe(out, 2, o);
      }
      out += indentOf(1);
      out += "</resolution>\n";
    }

    if (!m.piles.empty()) {
      out += indentOf(1);
      out += "<piles>\n";
      for (const SavePile& p : m.piles) {
        emitPile(out, 2, p);
      }
      out += indentOf(1);
      out += "</piles>\n";
    }

    if (!m.streams.empty()) {
      out += indentOf(1);
      out += "<streams>\n";
      for (const SaveStream& s : m.streams) {
        emitStream(out, 2, s);
      }
      out += indentOf(1);
      out += "</streams>\n";
    }

    if (!m.log.empty()) {
      std::vector<SaveMove> sorted = m.log;
      std::sort(sorted.begin(), sorted.end(), [](const SaveMove& a, const SaveMove& b) { return a.n < b.n; });
      out += indentOf(1);
      out += "<log>\n";
      for (const SaveMove& mv : sorted) {
        emitMove(out, 2, mv);
      }
      out += indentOf(1);
      out += "</log>\n";
    }

    if (!m.notes.empty()) {
      out += indentOf(1);
      out += "<notes>\n";
      for (const SaveNote& n : m.notes) {
        emitNote(out, 2, n);
      }
      out += indentOf(1);
      out += "</notes>\n";
    }

    out += "</save>\n";
    return out;
  }

  void
  writeCanonical(const SaveModel& m, const std::filesystem::path& path)
  {
    const std::string text = canonicalText(m);
    std::ofstream out(path, std::ios::binary | std::ios::trunc);
    if (!out) {
      throw std::invalid_argument(path.string() + ": could not open for writing");
    }
    out.write(text.data(), static_cast<std::streamsize>(text.size()));
    return;
  }

  std::string
  renderDraw(const SaveDraw& d)
  {
    std::string out;
    if (d.stream.has_value()) {
      out += *d.stream;
      if (d.n.has_value()) {
        out += "#" + std::to_string(*d.n);
      }
      if (d.value.has_value()) {
        out += "=" + *d.value;
      }
    }
    else if (d.randomizer.has_value()) {
      out += *d.randomizer;
      if (d.card.has_value()) {
        out += "=" + *d.card;
      }
    }
    return out;
  }

  std::string
  renderEvent(const SaveEvent& e)
  {
    std::string out = e.kind;
    if (e.unit.has_value()) {
      out += " unit=" + *e.unit;
    }
    if (e.hex.has_value()) {
      out += " hex=" + *e.hex;
    }
    if (e.side.has_value()) {
      out += " side=" + *e.side;
    }
    if (e.value.has_value()) {
      out += " value=" + *e.value;
    }
    if (!e.text.empty()) {
      out += " " + e.text;
    }
    return out;
  }

}  // namespace HexRecord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
