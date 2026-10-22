// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// SaveModel::read, over the hexxml facade. Checks what hexsave.xsd cannot: exactly one of unit/@hex
// and unit/@space; move numbers contiguous from 1; a "#k" copy suffix with k >= 1. Cross-document
// checks (does the hex or counter exist) need a Board and Roster and belong to the M4 glue instead.
// ----------------------------------------------
#include "hexrecord/SaveModel.h"
#include "hexxml/XmlDocument.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace HexRecord {

  namespace {

    std::vector<std::string>
    splitTokens(const std::string& s)
    {
      std::vector<std::string> tokens;
      std::size_t i = 0;
      while (i < s.size()) {
        while (i < s.size() && 0 != std::isspace(static_cast<unsigned char>(s[i]))) {
          ++i;
        }
        const std::size_t start = i;
        while (i < s.size() && 0 == std::isspace(static_cast<unsigned char>(s[i]))) {
          ++i;
        }
        if (i > start) {
          tokens.push_back(s.substr(start, i - start));
        }
      }
      return tokens;
    }

    // Reading checks a unit has exactly one of @hex and @space, and that a "#k" copy suffix on its id
    // has k >= 1; both are beyond what hexsave.xsd's UnitRef pattern can express.
    void
    checkUnit(const SaveUnit& unit, const HexXml::XmlNode& node)
    {
      if (unit.hex.has_value() == unit.space.has_value()) {
        const std::string which = unit.hex.has_value() ? "both hex and space" : "neither hex nor space";
        throw std::invalid_argument(node.file() + ":" + std::to_string(node.line()) + ": unit id='" + unit.id +
                                    "': " + which);
      }
      const std::size_t hash = unit.id.find('#');
      if (std::string::npos != hash) {
        const std::string digits = unit.id.substr(hash + 1);
        const int k = std::stoi(digits);
        if (k < 1) {
          throw std::invalid_argument(node.file() + ":" + std::to_string(node.line()) + ": unit id='" + unit.id +
                                      "': copy suffix '#" + digits + "' must be >= 1");
        }
      }
      return;
    }

    // Move numbers must be exactly 1..count, in some order; hexsave.xsd's xs:unique only rules out
    // duplicates, not gaps.
    void
    checkMoveNumbering(const std::vector<SaveMove>& log, const HexXml::XmlNode& logNode)
    {
      std::vector<int> ns;
      ns.reserve(log.size());
      for (const SaveMove& move : log) {
        ns.push_back(move.n);
      }
      std::sort(ns.begin(), ns.end());
      for (std::size_t i = 0; i < ns.size(); ++i) {
        const int expected = static_cast<int>(i) + 1;
        if (expected != ns[i]) {
          throw std::invalid_argument(logNode.file() + ":" + std::to_string(logNode.line()) +
                                      ": log: move numbers are not contiguous from 1 (expected " +
                                      std::to_string(expected) + ", found " + std::to_string(ns[i]) + ")");
        }
      }
      return;
    }

    std::vector<std::string>
    tokensOf(const HexXml::XmlNode& node, const char* attribute)
    {
      const std::optional<std::string> raw = node.optional(attribute);
      return raw.has_value() ? splitTokens(*raw) : std::vector<std::string>{};
    }

    SaveAsk
    readAsk(const HexXml::XmlNode& node)
    {
      SaveAsk ask;
      ask.what = node.required("what");
      ask.side = node.optional("side");
      ask.unit = node.optional("unit");
      ask.candidates = tokensOf(node, "candidates");
      ask.count = node.optionalAs<int>("count");
      ask.mayStopP = node.optionalAs<bool>("may-stop");
      ask.deck = node.optional("deck");
      ask.verb = node.optional("verb");
      ask.options = tokensOf(node, "options");
      return ask;
    }

    SaveOwe
    readOwe(const HexXml::XmlNode& node)
    {
      SaveOwe owe;
      owe.kind = node.required("kind");
      owe.side = node.optional("side");
      owe.count = node.optionalAs<int>("count");
      owe.fewest = node.optionalAs<int>("fewest");
      owe.most = node.optionalAs<int>("most");
      owe.unit = node.optional("unit");
      owe.from = node.optional("from");
      owe.path = tokensOf(node, "path");
      owe.router = node.optional("router");
      owe.involved = tokensOf(node, "involved");
      owe.name = node.optional("name");
      for (const HexXml::XmlNode& arg : node.children("arg")) {
        owe.args.push_back(SaveArg{arg.required("name"), arg.required("value")});
      }
      if (const std::optional<HexXml::XmlNode> ask = node.child("ask"); ask.has_value()) {
        owe.ask = readAsk(*ask);
      }
      return owe;
    }

  }  // namespace

  SaveModel
  SaveModel::read(const std::filesystem::path& path)
  {
    const HexXml::XmlDocument doc = HexXml::XmlDocument::load(path);
    const HexXml::XmlNode root = doc.root();

    SaveModel model;
    model.kind = root.required("kind");
    model.game = root.required("game");
    model.package = root.required("package");
    model.scenario = root.optional("scenario");
    model.seed = root.requiredAs<std::uint64_t>("seed");
    model.engine = root.optional("engine");
    model.created = root.optional("created");
    model.title = root.optional("title");
    if (const std::optional<std::string> loc = root.optional("xsi:noNamespaceSchemaLocation"); loc.has_value()) {
      model.schemaLocation = *loc;
    }

    if (const std::optional<HexXml::XmlNode> pkg = root.child("package"); pkg.has_value()) {
      for (const HexXml::XmlNode& f : pkg->children("file")) {
        SaveFile file;
        file.role = f.required("role");
        file.path = f.required("path");
        file.sha256 = f.optional("sha256");
        model.files.push_back(std::move(file));
      }
    }

    if (const std::optional<HexXml::XmlNode> cursor = root.child("cursor"); cursor.has_value()) {
      model.cursor.turn = cursor->requiredAs<int>("turn");
      model.cursor.phase = cursor->required("phase");
      model.cursor.side = cursor->optional("side");
      model.cursor.moves = cursor->optionalAs<std::uint64_t>("moves").value_or(0);
      model.cursor.overP = cursor->optionalAs<bool>("over").value_or(false);
      model.cursor.winner = cursor->optional("winner");
    }

    if (const std::optional<HexXml::XmlNode> sides = root.child("sides"); sides.has_value()) {
      for (const HexXml::XmlNode& s : sides->children("side")) {
        SaveSide side;
        side.id = s.required("id");
        for (const HexXml::XmlNode& r : s.children("register")) {
          side.registers.push_back(SaveRegister{r.required("track"), r.required("value")});
        }
        for (const HexXml::XmlNode& f : s.children("flag")) {
          side.flags.push_back(SaveFlag{f.required("name"), f.required("value")});
        }
        model.sides.push_back(std::move(side));
      }
    }

    if (const std::optional<HexXml::XmlNode> units = root.child("units"); units.has_value()) {
      for (const HexXml::XmlNode& u : units->children("unit")) {
        SaveUnit unit;
        unit.id = u.required("id");
        unit.counter = u.required("counter");
        unit.type = u.required("type");
        unit.owner = u.required("owner");
        unit.hex = u.optional("hex");
        unit.space = u.optional("space");
        unit.face = u.optional("face").value_or("front");
        unit.steps = u.optionalAs<int>("steps");
        if (const std::optional<std::string> status = u.optional("status"); status.has_value()) {
          unit.status = splitTokens(*status);
        }
        unit.attached = u.optional("attached");
        unit.revealedP = u.optionalAs<bool>("revealed");
        unit.movedP = u.optionalAs<bool>("moved").value_or(false);
        unit.delay = u.optionalAs<int>("delay");
        unit.text = u.text();

        checkUnit(unit, u);
        model.units.push_back(std::move(unit));
      }
    }

    if (const std::optional<HexXml::XmlNode> control = root.child("control"); control.has_value()) {
      for (const HexXml::XmlNode& h : control->children("hex")) {
        model.controlHexes.push_back(SaveControlHex{h.required("id"), h.required("side")});
      }
      for (const HexXml::XmlNode& l : control->children("link")) {
        SaveControlLink link;
        link.network = l.required("network");
        link.hexes = splitTokens(l.required("hexes"));
        link.side = l.required("side");
        model.controlLinks.push_back(std::move(link));
      }
    }

    if (const std::optional<HexXml::XmlNode> regions = root.child("regions"); regions.has_value()) {
      for (const HexXml::XmlNode& r : regions->children("region")) {
        SaveRegion region;
        region.layer = r.required("layer");
        region.id = r.required("id");
        region.status = r.optional("status");
        region.alignment = r.optional("alignment");
        region.posture = r.optional("posture");
        region.owner = r.optional("owner");
        model.regions.push_back(std::move(region));
      }
    }

    if (const std::optional<HexXml::XmlNode> resolution = root.child("resolution"); resolution.has_value()) {
      for (const HexXml::XmlNode& o : resolution->children("owe")) {
        model.resolution.push_back(readOwe(o));
      }
    }

    if (const std::optional<HexXml::XmlNode> piles = root.child("piles"); piles.has_value()) {
      for (const HexXml::XmlNode& p : piles->children("pile")) {
        SavePile pile;
        pile.randomizer = p.required("randomizer");
        pile.kind = p.required("kind");
        pile.side = p.optional("side");
        pile.cards = splitTokens(p.required("cards"));
        pile.next = p.optionalAs<int>("next");
        model.piles.push_back(std::move(pile));
      }
    }

    if (const std::optional<HexXml::XmlNode> streams = root.child("streams"); streams.has_value()) {
      for (const HexXml::XmlNode& s : streams->children("stream")) {
        SaveStream stream;
        stream.tag = s.required("tag");
        stream.draws = s.requiredAs<std::uint64_t>("draws");
        stream.check = s.optional("check");
        model.streams.push_back(std::move(stream));
      }
    }

    if (const std::optional<HexXml::XmlNode> log = root.child("log"); log.has_value()) {
      for (const HexXml::XmlNode& mv : log->children("move")) {
        SaveMove move;
        move.n = mv.requiredAs<int>("n");
        move.turn = mv.requiredAs<int>("turn");
        move.phase = mv.required("phase");
        move.side = mv.required("side");
        move.by = mv.optional("by").value_or("script");
        move.cmd = mv.required("cmd");
        if (const std::optional<std::string> units = mv.optional("units"); units.has_value()) {
          move.units = splitTokens(*units);
        }
        move.from = mv.optional("from");
        move.to = mv.optional("to");
        if (const std::optional<std::string> pathTokens = mv.optional("path"); pathTokens.has_value()) {
          move.path = splitTokens(*pathTokens);
        }
        move.target = mv.optional("target");
        move.mode = mv.optional("mode");
        if (const std::optional<std::string> modifiers = mv.optional("modifiers"); modifiers.has_value()) {
          move.modifiers = splitTokens(*modifiers);
        }
        move.card = mv.optional("card");
        move.value = mv.optional("value");
        move.choice = mv.optional("choice");

        for (const HexXml::XmlNode& arg : mv.children("arg")) {
          move.args.push_back(SaveArg{arg.required("name"), arg.required("value")});
        }
        if (const std::optional<HexXml::XmlNode> result = mv.child("result"); result.has_value()) {
          SaveResult r;
          r.outcome = result->required("outcome");
          r.odds = result->optional("odds");
          r.column = result->optional("column");
          r.drm = result->optionalAs<int>("drm");
          r.text = result->text();
          move.result = std::move(r);
        }
        for (const HexXml::XmlNode& draw : mv.children("draw")) {
          SaveDraw d;
          d.stream = draw.optional("stream");
          d.n = draw.optionalAs<int>("n");
          d.value = draw.optional("value");
          d.randomizer = draw.optional("randomizer");
          d.card = draw.optional("card");
          move.draws.push_back(std::move(d));
        }
        for (const HexXml::XmlNode& event : mv.children("event")) {
          SaveEvent e;
          e.kind = event.required("kind");
          e.unit = event.optional("unit");
          e.hex = event.optional("hex");
          e.side = event.optional("side");
          e.value = event.optional("value");
          e.text = event.text();
          move.events.push_back(std::move(e));
        }

        model.log.push_back(std::move(move));
      }
      checkMoveNumbering(model.log, *log);
    }

    if (const std::optional<HexXml::XmlNode> notes = root.child("notes"); notes.has_value()) {
      for (const HexXml::XmlNode& n : notes->children("note")) {
        SaveNote note;
        note.n = n.optionalAs<int>("n");
        note.text = n.text();
        model.notes.push_back(std::move(note));
      }
    }

    return model;
  }

}  // namespace HexRecord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
