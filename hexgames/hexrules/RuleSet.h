// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The RuleSet: the typed content of a hexrules document, resolved and immutable. Every IDREF is a
// dense id; every prose <rule> is kept with its scope so a game's ledger can account for it.
// ----------------------------------------------
#pragma once
#include "hexmodel/Ids.h"
#include "hexmodel/Quantities.h"

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace HexRules {

  using namespace HexModel;

  enum class Purpose : std::uint8_t { Movement, Zoc, Supply, Retreat, Network, Control, Grouping };
  using PurposeMask = std::bitset<8>;

  struct Side {
    std::string id;
    std::string name;
    bool automatonP = false;
  };

  // Symmetric by construction: the loader throws on an asymmetric @hostile-to.
  class HostilityMatrix {
  public:
    SideMask enemiesOf(SideId) const;
    bool hostileP(SideId, SideId) const;
  private:
    friend class RuleSetBuilder;
    std::vector<SideMask> rows_;
  };

  struct Terrain {  // hex-terrain and hexside-terrain share one shape
    std::string id;
    std::string name;
    MoveCost moveCost = NoCost{};
    bool stopP = false;
    std::vector<UnitTypeId> stopExcept;
    std::vector<UnitTypeId> enterOnly;
    std::optional<double> defenceMultiplier;
    std::optional<int> shift;  // CRT columns in the defender's favour
    PurposeMask blocks;
  };

  struct Network { std::string id; PurposeMask carries; bool mutableP = false; };
  struct RegionLayerSpec {
    std::string id;
    bool partitionP = true;
    std::optional<EdgeTerrainId> boundedBy;
    bool mutableP = false;
    std::vector<std::string> regionIds;
    std::vector<std::string> regionNames;   // parallel to regionIds; BoardBuilder's display names
    std::vector<SideMask> regionSides;      // parallel to regionIds; a region's own @side, if any
  };
  struct SpaceSpec { std::string id; std::string name; std::string kind; SideMask sides; bool returnsP = false; };

  // A hidden unit type: who may not look, what stays secret, what turns it face up, and whether it
  // can be hidden again (hexrules unit-type @hidden-from @conceals @reveal @rehide).
  enum class HiddenFrom : std::uint8_t { Enemy, All };
  enum class Conceals : std::uint8_t { Values, Identity };
  enum class RevealTrigger : std::uint8_t { Attacked, Attacking, Combat, Adjacent, Rule, Owner };
  enum class Rehide : std::uint8_t { Never, Rule };
  struct Concealment {
    HiddenFrom from;
    Conceals conceals;
    std::vector<RevealTrigger> reveal;  // at least one, in document order
    Rehide rehide;
  };

  struct UnitType {
    std::string id;
    std::string name;
    SideMask sides;  // empty: any side
    std::string kind;
    std::optional<std::string> steps;
    std::optional<std::string> zoc;   // full | own-hex | none
    std::optional<std::string> stacking;
    std::optional<Concealment> concealment;  // set: the type is placed face down
  };

  // Added in M6b: a step of the sequence of play (hexrules phase/step), what the rules make happen
  // in a phase. The engine runs, by `at`, the behaviour a registry holds under `does`.
  enum class StepAt : std::uint8_t { Enter, BeforeCommand, AfterCommand, End };
  struct Step {
    std::string id;
    std::string does;
    StepAt at = StepAt::Enter;
    std::vector<std::string> commands;  // command verbs; empty: every verb
    std::vector<RuleId> rules;          // each checked at load to be a prose <rule>
    TurnSelector turns;
    std::string text;
    int line = 0;
  };

  struct PhaseNode {
    PhaseId id;
    std::string name;
    SideMask sides;             // acting sides; several: repeated once per side, in order
    TurnSelector turns;
    std::optional<std::string> condition;
    bool optionalP = false;
    std::vector<Step> steps;    // document order
    std::vector<PhaseNode> children;
  };

  struct StackingSpec { std::optional<int> units; std::optional<int> steps; std::string enforced; std::string repair; std::vector<UnitTypeId> exempt; };

  struct ZocSpec {
    std::string id;
    SideMask sides;
    Extent range = HexCount{1};
    std::vector<UnitTypeId> projectedBy;
    std::vector<EdgeTerrainId> blockedBy;
    bool negatedByFriendlyP = false;
    bool stopsMovementP = false;
    bool zocToZocForbiddenP = false;
    bool mandatoryAttackP = false;
    PurposeMask blocks;
  };

  struct Mode { std::string id; std::string name; SideMask sides; std::vector<UnitTypeId> units; std::vector<PhaseId> phases; std::optional<NetworkId> network; std::optional<RandomizerId> randomizer; };
  struct MovementSpec { std::string budget; std::vector<Mode> modes; };  // actions | hexes | points

  struct Segment { int order; std::string kind; Extent length = Unlimited{}; std::optional<NetworkId> network; std::optional<LayerId> layer; };
  struct Trace { std::string id; SideMask sides; std::string sources; Extent maxLength = Unlimited{}; bool blockedByZocP = true; int threshold = 1; bool fatalP = false; std::string checked; std::vector<Segment> segments; };

  struct Table {
    std::string id;
    std::vector<std::string> cols;
    std::vector<std::string> rowLabels;
    std::vector<std::vector<std::string>> cells;  // rowLabels.size() x cols.size(), checked at load
    const std::string& cell(std::size_t row, std::size_t col) const;
  };
  struct Resolver { std::string id; std::string kind; std::optional<RandomizerId> randomizer; std::optional<std::string> minOdds, maxOdds; Rounding rounding = Rounding::Defender; std::vector<Table> tables; };
  struct Modifier { std::string id; std::string appliesTo; std::string kind; std::optional<double> value; std::optional<double> cap; SideMask sides; std::vector<PhaseId> phases; };
  struct CombatSpec { bool mandatoryP = false; std::vector<Resolver> resolvers; std::vector<Modifier> modifiers; };

  struct RetreatSpec { std::string routedBy; std::string distance; bool monotoneP = false; bool advanceAfterCombatP = false; std::string unsatisfiable; std::vector<EdgeTerrainId> blockedBy; };
  struct RandomizerSpec { std::string id; std::string kind; std::optional<int> size; SideMask sides; std::optional<std::string> fields; };  // die | deck | hand
  struct WeatherSpec { std::string source; std::optional<RandomizerId> randomizer; std::optional<LayerId> layer; };
  struct Condition { std::string id; SideMask sides; std::string kind; TurnSelector turns; std::string text; };

  // A prose rule, with the scope its attributes give it.
  struct ProseRule {
    RuleId id;
    std::optional<std::string> ref;
    std::optional<std::string> topic;
    std::vector<PhaseId> phases;
    SideMask sides;
    std::vector<UnitTypeId> units;
    TurnSelector turns;
    bool optionalP = false;
    std::string text;
    int line = 0;
  };

  class RuleSet {
  public:
    const std::string& gameId() const { return gameId_; }
    const std::vector<Side>& sides() const { return sides_; }
    const HostilityMatrix& hostility() const { return hostility_; }
    const std::vector<Terrain>& hexTerrain() const { return hexTerrain_; }
    const std::vector<Terrain>& hexsideTerrain() const { return hexsideTerrain_; }
    const std::vector<Network>& networks() const { return networks_; }
    const std::vector<RegionLayerSpec>& layers() const { return layers_; }
    const std::vector<SpaceSpec>& spaces() const { return spaces_; }
    const std::vector<UnitType>& unitTypes() const { return unitTypes_; }
    const std::vector<PhaseNode>& phases() const { return phases_; }
    const StackingSpec& stacking() const { return stacking_; }
    const std::vector<ZocSpec>& zocs() const { return zocs_; }
    const MovementSpec& movement() const { return movement_; }
    const std::vector<Trace>& traces() const { return traces_; }
    const CombatSpec& combat() const { return combat_; }
    const std::optional<RetreatSpec>& retreat() const { return retreat_; }
    const std::vector<RandomizerSpec>& randomizers() const { return randomizers_; }
    const std::optional<WeatherSpec>& weather() const { return weather_; }
    const std::vector<Condition>& victory() const { return victory_; }
    const std::vector<ProseRule>& proseRules() const { return prose_; }

    // Name lookups for loaders and tests; each throws std::invalid_argument naming the id.
    SideId side(const std::string& id) const;
    UnitTypeId unitType(const std::string& id) const;
    TerrainId terrain(const std::string& id) const;
    EdgeTerrainId edgeTerrain(const std::string& id) const;
    PhaseId phase(const std::string& id) const;
    // Added in M4: the same map, whole, so that a PhaseId can be written back as the document's own
    // phase id (a golden's cursor/@phase and every move/@phase). PhaseNode keeps only the display
    // name, so this is the only route from a dense PhaseId to the token a document holds.
    const std::map<std::string, PhaseId>& phaseIds() const { return phaseByName_; }

  private:
    friend class RuleSetBuilder;
    std::string gameId_;
    std::vector<Side> sides_;
    HostilityMatrix hostility_;
    std::vector<Terrain> hexTerrain_, hexsideTerrain_;
    std::vector<Network> networks_;
    std::vector<RegionLayerSpec> layers_;
    std::vector<SpaceSpec> spaces_;
    std::vector<UnitType> unitTypes_;
    std::vector<PhaseNode> phases_;
    StackingSpec stacking_;
    std::vector<ZocSpec> zocs_;
    MovementSpec movement_;
    std::vector<Trace> traces_;
    CombatSpec combat_;
    std::optional<RetreatSpec> retreat_;
    std::vector<RandomizerSpec> randomizers_;
    std::optional<WeatherSpec> weather_;
    std::vector<Condition> victory_;
    std::vector<ProseRule> prose_;

    // PhaseNode keeps only its display name, not the document's own id string (phase ids are
    // referenced from many places -- mode/@phase, rule/@phase, modifier/@phase -- but never
    // re-displayed), so phase() needs this map rather than a linear search over the tree.
    std::map<std::string, PhaseId> phaseByName_;
  };

}  // namespace HexRules
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
