// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/RulesDoc.h"

#include "hexxml/DocDetail.h"

namespace HexXml {

  using Detail::checkEnum;
  using Detail::idrefs;
  using Detail::splitTokens;

  namespace {

    constexpr std::initializer_list<std::string_view> kPurposes = {
        "movement", "zoc", "supply", "retreat", "network", "control", "grouping"};
    constexpr std::initializer_list<std::string_view> kTopics = {
        "sequence",  "geometry",  "counters", "stacking",      "zoc",       "control",  "movement",
        "supply",    "combat",    "retreat",  "reinforcement", "replacement", "weather", "cards",
        "politics",  "naval",     "leaders",  "markers",       "hidden",    "victory",  "other"};

    void
    checkPurposes(const XmlNode& node, std::string_view attr, const std::vector<std::string>& tokens)
    {
      for (const std::string& t : tokens) {
        checkEnum(node, attr, t, kPurposes);
      }
    }

    RuleAnnex
    parseRule(const XmlNode& node)
    {
      RuleAnnex r;
      r.id = node.required("id");
      r.ref = node.optional("ref");
      r.topic = node.optional("topic");
      if (r.topic) {
        checkEnum(node, "topic", *r.topic, kTopics);
      }
      r.phase = idrefs(node, "phase");
      r.sides = idrefs(node, "sides");
      r.units = idrefs(node, "units");
      r.turns = node.optional("turns");
      r.optionalFlag = node.optionalAs<bool>("optional").value_or(false);
      r.text = node.text();
      r.line = node.line();
      return r;
    }

    NoteAnnex
    parseNote(const XmlNode& node)
    {
      NoteAnnex n;
      n.ref = node.optional("ref");
      n.text = node.text();
      return n;
    }

    ListAnnex
    parseList(const XmlNode& node)
    {
      ListAnnex l;
      l.id = node.required("id");
      l.name = node.required("name");
      l.ordered = node.optionalAs<bool>("ordered").value_or(false);
      l.ref = node.optional("ref");
      l.line = node.line();
      for (const XmlNode& item : node.children("item")) {
        ListItemDoc d;
        d.code = item.optional("code");
        d.text = item.text();
        l.items.push_back(std::move(d));
      }
      return l;
    }

    TableAnnex
    parseTable(const XmlNode& node)
    {
      TableAnnex t;
      t.id = node.required("id");
      t.name = node.required("name");
      t.rows = node.required("rows");
      t.cols = node.required("cols");
      t.ref = node.optional("ref");
      t.line = node.line();
      for (const XmlNode& col : node.children("col")) {
        t.columns.push_back(TableColDoc{col.required("label"), col.text()});
      }
      for (const XmlNode& row : node.children("row")) {
        TableRowDoc r;
        r.label = row.required("label");
        for (const XmlNode& cell : row.children("cell")) {
          r.cells.push_back(cell.text());
        }
        Detail::requireExactly(row, r.cells.size(), t.columns.size(), r.label, t.id);
        t.tableRows.push_back(std::move(r));
      }
      return t;
    }

    // Every rule/note/list/table anywhere under `node`, in document order, regardless of nesting.
    void
    flattenAnnex(const XmlNode& node, RulesDoc& doc)
    {
      for (const XmlNode& child : node.children()) {
        const std::string n = child.name();
        if ("rule" == n) {
          doc.allRules.push_back(parseRule(child));
        } else if ("note" == n) {
          doc.allNotes.push_back(parseNote(child));
        } else if ("list" == n) {
          doc.allLists.push_back(parseList(child));
        } else if ("table" == n) {
          doc.allTables.push_back(parseTable(child));
        }
        flattenAnnex(child, doc);
      }
    }

    SideDoc
    parseSide(const XmlNode& node)
    {
      SideDoc s;
      s.id = node.required("id");
      s.name = node.required("name");
      s.control = node.required("control");
      checkEnum(node, "control", s.control, {"human", "automaton"});
      s.hostileTo = idrefs(node, "hostile-to");
      s.text = node.text();
      return s;
    }

    TerrainDoc
    parseTerrain(const XmlNode& node)
    {
      TerrainDoc t;
      t.id = node.required("id");
      t.name = node.required("name");
      t.moveCost = node.optional("move-cost");
      t.stop = node.optionalAs<bool>("stop").value_or(false);
      t.stopExcept = idrefs(node, "stop-except");
      t.enterOnly = idrefs(node, "enter-only");
      t.defenceMultiplier = node.optionalAs<double>("defence-multiplier");
      t.shift = node.optionalAs<int>("shift");
      t.blocks = idrefs(node, "blocks");
      checkPurposes(node, "blocks", t.blocks);
      t.ref = node.optional("ref");
      t.text = node.text();
      return t;
    }

    NetworkDoc
    parseNetwork(const XmlNode& node)
    {
      NetworkDoc n;
      n.id = node.required("id");
      n.name = node.required("name");
      n.carries = idrefs(node, "carries");
      checkPurposes(node, "carries", n.carries);
      n.mutableFlag = node.optionalAs<bool>("mutable").value_or(false);
      n.text = node.text();
      return n;
    }

    RegionDoc
    parseRegion(const XmlNode& node)
    {
      RegionDoc r;
      r.id = node.required("id");
      r.name = node.required("name");
      r.side = idrefs(node, "side");
      r.text = node.text();
      return r;
    }

    RegionLayerDoc
    parseRegionLayer(const XmlNode& node)
    {
      RegionLayerDoc l;
      l.id = node.required("id");
      l.name = node.required("name");
      l.partition = node.requiredAs<bool>("partition");
      l.boundedBy = node.optional("bounded-by");
      l.mutableFlag = node.optionalAs<bool>("mutable").value_or(false);
      for (const XmlNode& r : node.children("region")) {
        l.regions.push_back(parseRegion(r));
      }
      return l;
    }

    SpaceDoc
    parseSpace(const XmlNode& node)
    {
      SpaceDoc s;
      s.id = node.required("id");
      s.name = node.required("name");
      s.kind = node.required("kind");
      checkEnum(node, "kind", s.kind, {"box", "pool", "track", "display"});
      s.side = idrefs(node, "side");
      s.returnsFlag = node.optionalAs<bool>("returns");
      s.text = node.text();
      return s;
    }

    MapDoc
    parseMap(const XmlNode& node)
    {
      MapDoc m;
      const XmlNode hexIds = Detail::requiredChild(node, "hex-ids");
      m.hexIdPattern = hexIds.required("pattern");
      m.hexIdExample = hexIds.required("example");
      m.hexIdText = hexIds.text();
      for (const XmlNode& t : node.children("hex-terrain")) {
        m.hexTerrain.push_back(parseTerrain(t));
      }
      for (const XmlNode& t : node.children("hexside-terrain")) {
        m.hexsideTerrain.push_back(parseTerrain(t));
      }
      for (const XmlNode& n : node.children("network")) {
        m.networks.push_back(parseNetwork(n));
      }
      for (const XmlNode& l : node.children("region-layer")) {
        m.regionLayers.push_back(parseRegionLayer(l));
      }
      for (const XmlNode& s : node.children("space")) {
        m.spaces.push_back(parseSpace(s));
      }
      return m;
    }

    UnitTypeDoc
    parseUnitType(const XmlNode& node)
    {
      UnitTypeDoc u;
      u.id = node.required("id");
      u.name = node.required("name");
      u.side = idrefs(node, "side");
      u.kind = node.required("kind");
      checkEnum(node, "kind", u.kind, {"ground", "air", "naval", "hq", "leader", "marker"});
      u.steps = node.optional("steps");
      u.zoc = node.optional("zoc");
      if (u.zoc) {
        checkEnum(node, "zoc", *u.zoc, {"full", "own-hex", "none"});
      }
      u.stacking = node.optional("stacking");
      u.hidden = node.optionalAs<bool>("hidden").value_or(false);
      u.text = node.text();
      return u;
    }

    PhaseDoc
    parsePhase(const XmlNode& node)
    {
      PhaseDoc p;
      p.id = node.required("id");
      p.name = node.required("name");
      p.side = idrefs(node, "side");
      p.turns = node.optional("turns");
      p.condition = node.optional("condition");
      p.optionalFlag = node.optionalAs<bool>("optional").value_or(false);
      p.line = node.line();
      for (const XmlNode& c : node.children("phase")) {
        p.children.push_back(parsePhase(c));
      }
      return p;
    }

    StackingDoc
    parseStacking(const XmlNode& node)
    {
      StackingDoc s;
      s.units = node.optionalAs<int>("units");
      s.steps = node.optionalAs<int>("steps");
      s.enforced = node.required("enforced");
      s.repair = node.required("repair");
      checkEnum(node, "repair", s.repair, {"eliminate-excess", "none"});
      s.exempt = idrefs(node, "exempt");
      s.ref = node.optional("ref");
      return s;
    }

    ZocDoc
    parseZoc(const XmlNode& node)
    {
      ZocDoc z;
      z.id = node.required("id");
      z.name = node.required("name");
      z.side = idrefs(node, "side");
      z.range = node.required("range");
      z.projectedBy = idrefs(node, "projected-by");
      z.blockedBy = idrefs(node, "blocked-by");
      z.negatedByFriendly = node.optionalAs<bool>("negated-by-friendly").value_or(false);
      z.stopsMovement = node.optionalAs<bool>("stops-movement").value_or(false);
      z.zocToZocForbidden = node.optionalAs<bool>("zoc-to-zoc-forbidden").value_or(false);
      z.mandatoryAttack = node.optionalAs<bool>("mandatory-attack").value_or(false);
      z.blocks = idrefs(node, "blocks");
      checkPurposes(node, "blocks", z.blocks);
      z.ref = node.optional("ref");
      return z;
    }

    ModeDoc
    parseMode(const XmlNode& node)
    {
      ModeDoc m;
      m.id = node.required("id");
      m.name = node.required("name");
      m.side = idrefs(node, "side");
      m.units = idrefs(node, "units");
      m.phase = idrefs(node, "phase");
      m.network = node.optional("network");
      m.randomizer = node.optional("randomizer");
      m.ref = node.optional("ref");
      m.text = node.text();
      return m;
    }

    MovementDoc
    parseMovement(const XmlNode& node)
    {
      MovementDoc mv;
      mv.budget = node.required("budget");
      checkEnum(node, "budget", mv.budget, {"actions", "hexes", "points"});
      for (const XmlNode& m : node.children("mode")) {
        mv.modes.push_back(parseMode(m));
      }
      return mv;
    }

    SegmentDoc
    parseSegment(const XmlNode& node)
    {
      SegmentDoc s;
      s.order = node.requiredAs<int>("order");
      s.kind = node.required("kind");
      checkEnum(node, "kind", s.kind, {"free", "network", "region-gated"});
      s.length = node.optional("length");
      s.network = node.optional("network");
      s.layer = node.optional("layer");
      s.text = node.text();
      return s;
    }

    TraceDoc
    parseTrace(const XmlNode& node)
    {
      TraceDoc t;
      t.id = node.required("id");
      t.name = node.required("name");
      t.side = idrefs(node, "side");
      t.sources = node.required("sources");
      t.maxLength = node.required("max-length");
      t.blockedByZoc = node.requiredAs<bool>("blocked-by-zoc");
      t.threshold = node.optionalAs<int>("threshold").value_or(1);
      t.fatal = node.requiredAs<bool>("fatal");
      t.checked = node.required("checked");
      t.ref = node.optional("ref");
      for (const XmlNode& s : node.children("segment")) {
        t.segments.push_back(parseSegment(s));
      }
      return t;
    }

    ResolverDoc
    parseResolver(const XmlNode& node)
    {
      ResolverDoc r;
      r.id = node.required("id");
      r.name = node.required("name");
      r.kind = node.required("kind");
      checkEnum(node, "kind", r.kind, {"odds-table", "band-table", "fire-table", "card-race", "automatic"});
      r.randomizer = node.optional("randomizer");
      r.minOdds = node.optional("min-odds");
      r.maxOdds = node.optional("max-odds");
      r.rounding = node.optional("rounding");
      if (r.rounding) {
        checkEnum(node, "rounding", *r.rounding, {"defender", "attacker", "none"});
      }
      r.ref = node.optional("ref");
      for (const XmlNode& t : node.children("table")) {
        r.tables.push_back(parseTable(t));
      }
      return r;
    }

    ModifierDoc
    parseModifier(const XmlNode& node)
    {
      ModifierDoc m;
      m.id = node.required("id");
      m.name = node.required("name");
      m.appliesTo = node.required("applies-to");
      checkEnum(node, "applies-to", m.appliesTo, {"attacker", "defender", "either"});
      m.kind = node.required("kind");
      checkEnum(node, "kind", m.kind, {"shift", "multiplier", "drm", "hit-limit", "other"});
      m.value = node.optionalAs<double>("value");
      m.cap = node.optionalAs<double>("cap");
      m.side = idrefs(node, "side");
      m.phase = idrefs(node, "phase");
      m.ref = node.optional("ref");
      return m;
    }

    CombatDoc
    parseCombat(const XmlNode& node)
    {
      CombatDoc c;
      c.mandatory = node.requiredAs<bool>("mandatory");
      for (const XmlNode& r : node.children("resolver")) {
        c.resolvers.push_back(parseResolver(r));
      }
      for (const XmlNode& m : node.children("modifier")) {
        c.modifiers.push_back(parseModifier(m));
      }
      return c;
    }

    RetreatDoc
    parseRetreat(const XmlNode& node)
    {
      RetreatDoc r;
      r.routedBy = node.required("routed-by");
      checkEnum(node, "routed-by", r.routedBy, {"attacker", "owner", "none"});
      r.distance = node.required("distance");
      r.monotone = node.optionalAs<bool>("monotone").value_or(false);
      r.advanceAfterCombat = node.requiredAs<bool>("advance-after-combat");
      r.unsatisfiable = node.required("unsatisfiable");
      checkEnum(node, "unsatisfiable", r.unsatisfiable,
                {"eliminate", "convert-to-step-loss", "not-applicable"});
      r.blockedBy = idrefs(node, "blocked-by");
      r.ref = node.optional("ref");
      return r;
    }

    RandomizerDoc
    parseRandomizer(const XmlNode& node)
    {
      RandomizerDoc r;
      r.id = node.required("id");
      r.name = node.required("name");
      r.kind = node.required("kind");
      checkEnum(node, "kind", r.kind, {"die", "deck", "hand"});
      r.size = node.optionalAs<int>("size");
      r.side = idrefs(node, "side");
      r.fields = node.optional("fields");
      return r;
    }

    WeatherDoc
    parseWeather(const XmlNode& node)
    {
      WeatherDoc w;
      w.source = node.required("source");
      checkEnum(node, "source", w.source, {"none", "rolled", "printed"});
      w.randomizer = node.optional("randomizer");
      w.layer = node.optional("layer");
      w.ref = node.optional("ref");
      return w;
    }

    ConditionDoc
    parseCondition(const XmlNode& node)
    {
      ConditionDoc c;
      c.id = node.required("id");
      c.side = idrefs(node, "side");
      c.kind = node.required("kind");
      checkEnum(node, "kind", c.kind, {"immediate", "scheduled", "final"});
      c.turns = node.optional("turns");
      c.ref = node.optional("ref");
      c.text = node.text();
      return c;
    }

  }  // namespace

  RulesDoc
  RulesDoc::parse(const XmlDocument& doc)
  {
    const XmlNode root = doc.root();
    if ("game" != root.name()) {
      throw std::invalid_argument(root.file() + ":" + std::to_string(root.line()) +
                                   ": expected root element 'game', found '" + root.name() + "'");
    }

    RulesDoc r;
    r.id = root.required("id");
    r.title = root.required("title");
    r.publisher = root.optional("publisher");
    r.year = root.optional("year");
    r.players = root.requiredAs<int>("players");
    r.source = root.optional("source");
    r.hexScale = root.optional("hex-scale");
    r.turnScale = root.optional("turn-scale");

    const XmlNode sides = Detail::requiredChild(root, "sides");
    for (const XmlNode& s : sides.children("side")) {
      r.sides.push_back(parseSide(s));
    }

    r.map = parseMap(Detail::requiredChild(root, "map"));

    const XmlNode counters = Detail::requiredChild(root, "counters");
    for (const XmlNode& u : counters.children("unit-type")) {
      r.unitTypes.push_back(parseUnitType(u));
    }

    const XmlNode sequence = Detail::requiredChild(root, "sequence");
    r.sequenceTurns = sequence.optionalAs<int>("turns");
    for (const XmlNode& p : sequence.children("phase")) {
      r.phases.push_back(parsePhase(p));
    }

    r.stacking = parseStacking(Detail::requiredChild(root, "stacking"));

    for (const XmlNode& z : root.children("zoc")) {
      r.zocs.push_back(parseZoc(z));
    }

    r.movement = parseMovement(Detail::requiredChild(root, "movement"));
    r.supply = SupplyDoc{};
    for (const XmlNode& t : Detail::requiredChild(root, "supply").children("trace")) {
      r.supply.traces.push_back(parseTrace(t));
    }

    r.combat = parseCombat(Detail::requiredChild(root, "combat"));

    if (const std::optional<XmlNode> retreat = root.child("retreat")) {
      r.retreat = parseRetreat(*retreat);
    }

    for (const XmlNode& rz : root.children("randomizer")) {
      r.randomizers.push_back(parseRandomizer(rz));
    }

    if (const std::optional<XmlNode> weather = root.child("weather")) {
      r.weather = parseWeather(*weather);
    }

    const XmlNode victory = Detail::requiredChild(root, "victory");
    for (const XmlNode& c : victory.children("condition")) {
      r.victory.conditions.push_back(parseCondition(c));
    }

    flattenAnnex(root, r);

    return r;
  }

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
