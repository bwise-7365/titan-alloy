// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A hexrules document (game_rules/xml/hexrules.xsd), mirrored one to one: one struct per complex
// type, attributes as typed fields, element children as vectors. parse() re-checks everything the
// XSD promised (required attributes, enumerations, one cell per table column) so a C++ caller never
// has to trust the file alone; IDREFS attributes come back as token vectors, left unresolved -- that
// is RuleSetBuilder's job, in hexrules, which alone knows the dense id spaces. The annex (note, rule,
// list, table) is collected twice: once nested where it was written, and once flattened over the
// whole document, because a RuleSet's prose is scoped only by its own attributes, never by position.
// ----------------------------------------------
#pragma once
#include "hexxml/XmlDocument.h"

#include <optional>
#include <string>
#include <vector>

namespace HexXml {

  struct RuleAnnex {
    std::string id;
    std::optional<std::string> ref;
    std::optional<std::string> topic;
    std::vector<std::string> phase;
    std::vector<std::string> sides;
    std::vector<std::string> units;
    std::optional<std::string> turns;
    bool optionalFlag = false;
    std::string text;
    int line = 0;
  };

  struct ListItemDoc {
    std::optional<std::string> code;
    std::string text;
  };

  struct ListAnnex {
    std::string id;
    std::string name;
    bool ordered = false;
    std::optional<std::string> ref;
    std::vector<ListItemDoc> items;
    int line = 0;
  };

  struct TableColDoc {
    std::string label;
    std::string text;
  };

  struct TableRowDoc {
    std::string label;
    std::vector<std::string> cells;
  };

  // cells is checked here: every row has exactly cols.size() cells, or parse() throws naming the
  // table id and the offending row.
  struct TableAnnex {
    std::string id;
    std::string name;
    std::string rows;
    std::string cols;
    std::optional<std::string> ref;
    std::vector<TableColDoc> columns;
    std::vector<TableRowDoc> tableRows;
    int line = 0;
  };

  struct NoteAnnex {
    std::optional<std::string> ref;
    std::string text;
  };

  struct SideDoc {
    std::string id;
    std::string name;
    std::string control;  // "human" | "automaton"
    std::vector<std::string> hostileTo;
    std::string text;
  };

  struct TerrainDoc {  // hex-terrain and hexside-terrain: same shape
    std::string id;
    std::string name;
    std::optional<std::string> moveCost;  // a number, or prohibited/entire/other-terrain/none
    bool stop = false;
    std::vector<std::string> stopExcept;
    std::vector<std::string> enterOnly;
    std::optional<double> defenceMultiplier;
    std::optional<int> shift;
    std::vector<std::string> blocks;  // Purposes
    std::optional<std::string> ref;
    std::string text;
  };

  struct NetworkDoc {
    std::string id;
    std::string name;
    std::vector<std::string> carries;
    bool mutableFlag = false;
    std::string text;
  };

  struct RegionDoc {
    std::string id;
    std::string name;
    std::vector<std::string> side;
    std::string text;
  };

  struct RegionLayerDoc {
    std::string id;
    std::string name;
    bool partition = false;
    std::optional<std::string> boundedBy;
    bool mutableFlag = false;
    std::vector<RegionDoc> regions;
  };

  struct SpaceDoc {
    std::string id;
    std::string name;
    std::string kind;  // box | pool | track | display
    std::vector<std::string> side;
    std::optional<bool> returnsFlag;
    std::string text;
  };

  struct MapDoc {
    std::string hexIdPattern;
    std::string hexIdExample;
    std::string hexIdText;
    std::vector<TerrainDoc> hexTerrain;
    std::vector<TerrainDoc> hexsideTerrain;
    std::vector<NetworkDoc> networks;
    std::vector<RegionLayerDoc> regionLayers;
    std::vector<SpaceDoc> spaces;
  };

  struct UnitTypeDoc {
    std::string id;
    std::string name;
    std::vector<std::string> side;
    std::string kind;  // ground|air|naval|hq|leader|marker
    std::optional<std::string> steps;
    std::optional<std::string> zoc;  // full|own-hex|none
    std::optional<std::string> stacking;
    bool hidden = false;
    std::optional<std::string> hiddenFrom;  // enemy|all
    std::optional<std::string> conceals;    // values|identity
    std::vector<std::string> reveal;        // attacked|attacking|combat|adjacent|rule|owner
    std::optional<std::string> rehide;      // never|rule
    std::string text;
  };

  struct StepDoc {  // added in M6b: phase/step
    std::string id;
    std::string does;
    std::string at;  // enter|before-command|after-command|end
    std::vector<std::string> commands;
    std::vector<std::string> rules;
    std::optional<std::string> turns;
    std::string text;
    int line = 0;
  };

  struct PhaseDoc {
    std::string id;
    std::string name;
    std::vector<std::string> side;
    std::optional<std::string> turns;
    std::optional<std::string> condition;
    bool optionalFlag = false;
    std::vector<StepDoc> steps;  // document order
    std::vector<PhaseDoc> children;
    int line = 0;
  };

  struct StackingDoc {
    std::optional<int> units;
    std::optional<int> steps;
    std::string enforced;
    std::string repair;  // eliminate-excess | none
    std::vector<std::string> exempt;
    std::optional<std::string> ref;
  };

  struct ZocDoc {
    std::string id;
    std::string name;
    std::vector<std::string> side;
    std::string range;  // Extent
    std::vector<std::string> projectedBy;
    std::vector<std::string> blockedBy;
    bool negatedByFriendly = false;
    bool stopsMovement = false;
    bool zocToZocForbidden = false;
    bool mandatoryAttack = false;
    std::vector<std::string> blocks;
    std::optional<std::string> ref;
  };

  struct ModeDoc {
    std::string id;
    std::string name;
    std::vector<std::string> side;
    std::vector<std::string> units;
    std::vector<std::string> phase;
    std::optional<std::string> network;
    std::optional<std::string> randomizer;
    std::optional<std::string> ref;
    std::string text;
  };

  struct MovementDoc {
    std::string budget;  // actions|hexes|points
    std::vector<ModeDoc> modes;
  };

  struct SegmentDoc {
    int order = 0;
    std::string kind;  // free|network|region-gated
    std::optional<std::string> length;
    std::optional<std::string> network;
    std::optional<std::string> layer;
    std::string text;
  };

  struct TraceDoc {
    std::string id;
    std::string name;
    std::vector<std::string> side;
    std::string sources;
    std::string maxLength;
    bool blockedByZoc = false;
    int threshold = 1;
    bool fatal = false;
    std::string checked;
    std::optional<std::string> ref;
    std::vector<SegmentDoc> segments;
  };

  struct SupplyDoc {
    std::vector<TraceDoc> traces;
  };

  struct ResolverDoc {
    std::string id;
    std::string name;
    std::string kind;  // odds-table|band-table|fire-table|card-race|automatic
    std::optional<std::string> randomizer;
    std::optional<std::string> minOdds;
    std::optional<std::string> maxOdds;
    std::optional<std::string> rounding;  // defender|attacker|none
    std::optional<std::string> ref;
    std::vector<TableAnnex> tables;  // this resolver's own annex tables
  };

  struct ModifierDoc {
    std::string id;
    std::string name;
    std::string appliesTo;  // attacker|defender|either
    std::string kind;       // shift|multiplier|drm|hit-limit|other
    std::optional<double> value;
    std::optional<double> cap;
    std::vector<std::string> side;
    std::vector<std::string> phase;
    std::optional<std::string> ref;
  };

  struct CombatDoc {
    bool mandatory = false;
    std::vector<ResolverDoc> resolvers;
    std::vector<ModifierDoc> modifiers;
  };

  struct RetreatDoc {
    std::string routedBy;  // attacker|owner|none
    std::string distance;
    bool monotone = false;
    bool advanceAfterCombat = false;
    std::string unsatisfiable;  // eliminate|convert-to-step-loss|not-applicable
    std::vector<std::string> blockedBy;
    std::optional<std::string> ref;
  };

  struct RandomizerDoc {
    std::string id;
    std::string name;
    std::string kind;  // die|deck|hand
    std::optional<int> size;
    std::vector<std::string> side;
    std::optional<std::string> fields;
  };

  struct WeatherDoc {
    std::string source;  // none|rolled|printed
    std::optional<std::string> randomizer;
    std::optional<std::string> layer;
    std::optional<std::string> ref;
  };

  struct ConditionDoc {
    std::string id;
    std::vector<std::string> side;
    std::string kind;  // immediate|scheduled|final
    std::optional<std::string> turns;
    std::optional<std::string> ref;
    std::string text;
  };

  struct VictoryDoc {
    std::vector<ConditionDoc> conditions;
  };

  struct RulesDoc {
    std::string id;
    std::string title;
    std::optional<std::string> publisher;
    std::optional<std::string> year;
    int players = 0;
    std::optional<std::string> source;
    std::optional<std::string> hexScale;
    std::optional<std::string> turnScale;

    std::vector<SideDoc> sides;
    MapDoc map;
    std::vector<UnitTypeDoc> unitTypes;
    std::optional<int> sequenceTurns;
    std::vector<PhaseDoc> phases;  // top-level phase(s) of <sequence>
    StackingDoc stacking;
    std::vector<ZocDoc> zocs;
    MovementDoc movement;
    SupplyDoc supply;
    CombatDoc combat;
    std::optional<RetreatDoc> retreat;
    std::vector<RandomizerDoc> randomizers;
    std::optional<WeatherDoc> weather;
    VictoryDoc victory;

    // The whole document's annex, flattened in document order, wherever it was written.
    std::vector<RuleAnnex> allRules;
    std::vector<ListAnnex> allLists;
    std::vector<TableAnnex> allTables;
    std::vector<NoteAnnex> allNotes;

    static RulesDoc parse(const XmlDocument&);
  };

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
