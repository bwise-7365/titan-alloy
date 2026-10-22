// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/SaveDoc.h"

#include "hexxml/DocDetail.h"

namespace HexXml {

  using Detail::checkEnum;
  using Detail::requiredChild;
  using Detail::splitTokens;

  namespace {

    SaveFileDoc
    parseFile(const XmlNode& node)
    {
      SaveFileDoc f;
      f.role = node.required("role");
      checkEnum(node, "role", f.role, {"package", "rules", "sheet", "counters", "cards", "scenario"});
      f.path = node.required("path");
      f.sha256 = node.optional("sha256");
      return f;
    }

    SaveCursorDoc
    parseCursor(const XmlNode& node)
    {
      SaveCursorDoc c;
      c.turn = node.requiredAs<int>("turn");
      c.phase = node.required("phase");
      c.side = node.optional("side");
      c.moves = node.optionalAs<int>("moves").value_or(0);
      c.over = node.optionalAs<bool>("over").value_or(false);
      c.winner = node.optional("winner");
      return c;
    }

    SaveRegisterDoc
    parseRegister(const XmlNode& node)
    {
      return SaveRegisterDoc{node.required("track"), node.required("value")};
    }

    SaveFlagDoc
    parseFlag(const XmlNode& node)
    {
      return SaveFlagDoc{node.required("name"), node.required("value")};
    }

    SaveSideDoc
    parseSide(const XmlNode& node)
    {
      SaveSideDoc s;
      s.id = node.required("id");
      for (const XmlNode& r : node.children("register")) {
        s.registers.push_back(parseRegister(r));
      }
      for (const XmlNode& f : node.children("flag")) {
        s.flags.push_back(parseFlag(f));
      }
      return s;
    }

    SaveUnitDoc
    parseUnit(const XmlNode& node)
    {
      SaveUnitDoc u;
      u.id = node.required("id");
      u.counter = node.required("counter");
      u.type = node.required("type");
      u.owner = node.required("owner");
      u.hex = node.optional("hex");
      u.space = node.optional("space");
      u.face = node.optional("face").value_or("front");
      checkEnum(node, "face", u.face, {"front", "back"});
      u.steps = node.optionalAs<int>("steps");
      if (const std::optional<std::string> status = node.optional("status")) {
        u.status = splitTokens(*status);
      }
      u.attached = node.optional("attached");
      u.revealed = node.optionalAs<bool>("revealed");
      u.moved = node.optionalAs<bool>("moved").value_or(false);
      u.delay = node.optionalAs<int>("delay");
      u.text = node.text();
      return u;
    }

    SaveControlHexDoc
    parseControlHex(const XmlNode& node)
    {
      return SaveControlHexDoc{node.required("id"), node.required("side")};
    }

    SaveControlLinkDoc
    parseControlLink(const XmlNode& node)
    {
      SaveControlLinkDoc l;
      l.network = node.required("network");
      l.hexes = splitTokens(node.required("hexes"));
      l.side = node.required("side");
      return l;
    }

    SaveRegionDoc
    parseRegion(const XmlNode& node)
    {
      SaveRegionDoc r;
      r.layer = node.required("layer");
      r.id = node.required("id");
      r.status = node.optional("status");
      r.alignment = node.optional("alignment");
      r.posture = node.optional("posture");
      r.owner = node.optional("owner");
      return r;
    }

    SavePileDoc
    parsePile(const XmlNode& node)
    {
      SavePileDoc p;
      p.randomizer = node.required("randomizer");
      p.kind = node.required("kind");
      checkEnum(node, "kind", p.kind, {"draw", "hand", "pending", "current", "discard", "removed"});
      p.side = node.optional("side");
      p.cards = splitTokens(node.required("cards"));
      p.next = node.optionalAs<int>("next");
      return p;
    }

    SaveStreamDoc
    parseStream(const XmlNode& node)
    {
      SaveStreamDoc s;
      s.tag = node.required("tag");
      s.draws = node.requiredAs<int>("draws");
      s.check = node.optional("check");
      return s;
    }

    SaveArgDoc
    parseArg(const XmlNode& node)
    {
      return SaveArgDoc{node.required("name"), node.required("value")};
    }

    std::vector<std::string>
    tokensOf(const XmlNode& node, const char* attribute)
    {
      const std::optional<std::string> raw = node.optional(attribute);
      return raw ? splitTokens(*raw) : std::vector<std::string>{};
    }

    SaveAskDoc
    parseAsk(const XmlNode& node)
    {
      SaveAskDoc a;
      a.what = node.required("what");
      checkEnum(node, "what", a.what, {"loss", "retreat", "card", "choice"});
      a.side = node.optional("side");
      a.unit = node.optional("unit");
      a.candidates = tokensOf(node, "candidates");
      a.count = node.optionalAs<int>("count");
      a.mayStop = node.optionalAs<bool>("may-stop");
      a.deck = node.optional("deck");
      a.verb = node.optional("verb");
      a.options = tokensOf(node, "options");
      return a;
    }

    SaveOweDoc
    parseOwe(const XmlNode& node)
    {
      SaveOweDoc o;
      o.kind = node.required("kind");
      checkEnum(node, "kind", o.kind, {"loss", "retreat", "unit-retreat", "game"});
      o.side = node.optional("side");
      o.count = node.optionalAs<int>("count");
      o.fewest = node.optionalAs<int>("fewest");
      o.most = node.optionalAs<int>("most");
      o.unit = node.optional("unit");
      o.from = node.optional("from");
      o.path = tokensOf(node, "path");
      o.router = node.optional("router");
      o.involved = tokensOf(node, "involved");
      o.name = node.optional("name");
      for (const XmlNode& a : node.children("arg")) {
        o.args.push_back(SaveArgDoc{a.required("name"), a.required("value")});
      }
      if (const std::optional<XmlNode> ask = node.child("ask")) {
        o.ask = parseAsk(*ask);
      }
      return o;
    }

    SaveResultDoc
    parseResult(const XmlNode& node)
    {
      SaveResultDoc r;
      r.outcome = node.required("outcome");
      r.odds = node.optional("odds");
      r.column = node.optional("column");
      r.drm = node.optionalAs<int>("drm");
      r.text = node.text();
      return r;
    }

    SaveDrawDoc
    parseDraw(const XmlNode& node)
    {
      SaveDrawDoc d;
      d.stream = node.optional("stream");
      d.n = node.optionalAs<int>("n");
      d.value = node.optional("value");
      d.randomizer = node.optional("randomizer");
      d.card = node.optional("card");
      return d;
    }

    SaveEventDoc
    parseEvent(const XmlNode& node)
    {
      SaveEventDoc e;
      e.kind = node.required("kind");
      e.unit = node.optional("unit");
      e.hex = node.optional("hex");
      e.side = node.optional("side");
      e.value = node.optional("value");
      e.text = node.text();
      return e;
    }

    SaveMoveDoc
    parseMove(const XmlNode& node)
    {
      SaveMoveDoc m;
      m.n = node.requiredAs<int>("n");
      m.turn = node.requiredAs<int>("turn");
      m.phase = node.required("phase");
      m.side = node.required("side");
      m.by = node.optional("by").value_or("script");
      checkEnum(node, "by", m.by, {"human", "script", "ai"});
      m.cmd = node.required("cmd");
      if (const std::optional<std::string> units = node.optional("units")) {
        m.units = splitTokens(*units);
      }
      m.from = node.optional("from");
      m.to = node.optional("to");
      if (const std::optional<std::string> path = node.optional("path")) {
        m.path = splitTokens(*path);
      }
      m.target = node.optional("target");
      m.mode = node.optional("mode");
      if (const std::optional<std::string> modifiers = node.optional("modifiers")) {
        m.modifiers = splitTokens(*modifiers);
      }
      m.card = node.optional("card");
      m.value = node.optional("value");
      m.choice = node.optional("choice");

      for (const XmlNode& a : node.children("arg")) {
        m.args.push_back(parseArg(a));
      }
      if (const std::optional<XmlNode> result = node.child("result")) {
        m.result = parseResult(*result);
      }
      for (const XmlNode& d : node.children("draw")) {
        m.draws.push_back(parseDraw(d));
      }
      for (const XmlNode& e : node.children("event")) {
        m.events.push_back(parseEvent(e));
      }
      return m;
    }

    SaveNoteDoc
    parseNote(const XmlNode& node)
    {
      SaveNoteDoc n;
      n.n = node.optionalAs<int>("n");
      n.text = node.text();
      return n;
    }

  }  // namespace

  SaveDoc
  SaveDoc::parse(const XmlDocument& doc)
  {
    const XmlNode root = doc.root();
    if ("save" != root.name()) {
      throw std::invalid_argument(root.file() + ":" + std::to_string(root.line()) +
                                   ": expected root element 'save', found '" + root.name() + "'");
    }

    SaveDoc s;
    s.format = root.required("format");
    s.kind = root.required("kind");
    checkEnum(root, "kind", s.kind, {"scenario", "save", "script", "golden"});
    s.game = root.required("game");
    s.package = root.required("package");
    s.scenario = root.optional("scenario");
    s.seed = root.requiredAs<std::uint64_t>("seed");
    s.engine = root.optional("engine");
    s.created = root.optional("created");
    s.title = root.optional("title");

    if (const std::optional<XmlNode> package = root.child("package")) {
      for (const XmlNode& f : package->children("file")) {
        s.files.push_back(parseFile(f));
      }
    }

    s.cursor = parseCursor(requiredChild(root, "cursor"));

    if (const std::optional<XmlNode> sides = root.child("sides")) {
      for (const XmlNode& side : sides->children("side")) {
        s.sides.push_back(parseSide(side));
      }
    }

    if (const std::optional<XmlNode> units = root.child("units")) {
      for (const XmlNode& u : units->children("unit")) {
        s.units.push_back(parseUnit(u));
      }
    }

    if (const std::optional<XmlNode> control = root.child("control")) {
      for (const XmlNode& h : control->children("hex")) {
        s.controlHexes.push_back(parseControlHex(h));
      }
      for (const XmlNode& l : control->children("link")) {
        s.controlLinks.push_back(parseControlLink(l));
      }
    }

    if (const std::optional<XmlNode> regions = root.child("regions")) {
      for (const XmlNode& r : regions->children("region")) {
        s.regions.push_back(parseRegion(r));
      }
    }

    if (const std::optional<XmlNode> resolution = root.child("resolution")) {
      for (const XmlNode& o : resolution->children("owe")) {
        s.resolution.push_back(parseOwe(o));
      }
    }

    if (const std::optional<XmlNode> piles = root.child("piles")) {
      for (const XmlNode& p : piles->children("pile")) {
        s.piles.push_back(parsePile(p));
      }
    }

    if (const std::optional<XmlNode> streams = root.child("streams")) {
      for (const XmlNode& st : streams->children("stream")) {
        s.streams.push_back(parseStream(st));
      }
    }

    if (const std::optional<XmlNode> log = root.child("log")) {
      for (const XmlNode& m : log->children("move")) {
        s.log.push_back(parseMove(m));
      }
    }

    if (const std::optional<XmlNode> notes = root.child("notes")) {
      for (const XmlNode& n : notes->children("note")) {
        s.notes.push_back(parseNote(n));
      }
    }

    return s;
  }

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
