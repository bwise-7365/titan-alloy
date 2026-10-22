// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Everything the PGG policies read off the immutable definition, looked up once: the two sides,
// what each counter is (arm, movement class, division, the counter that carries a German infantry
// division's last two steps, a Leader's rating), the map facts (terrain, rivers, roads, rail, the
// edges, the westernmost hex-columns, the Victory Point hexes and levels from the rules document's
// lists, the entrance areas) and the ids of the phases, modes, networks and spaces the rules name.
// Every id the rules or the prose names is resolved here and throws if absent. The entrance areas' hexes
// are the printed map's (PggFactsMap.cpp); data the rules need and the sheet does not carry is listed by
// dataGaps().
// ----------------------------------------------
#pragma once
#include "hexengine/Policies.h"
#include "hexrules/Package.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Pgg {

  using namespace HexModel;
  using HexEngine::Ctx;

  enum class Arm : std::uint8_t {
    SovietRifle, SovietArmour, SovietLeader, GermanInfantry, GermanPanzer, GermanMotorized, GermanCavalry, Marker
  };
  // What a unit pays on roads and in forest (6.7, 10.21): Foot pays 1 on a road and 1 in forest,
  // Motor 1/2 and 2, a Leader 1/2 on a road and 1 in forest.
  enum class MoveClass : std::uint8_t { Foot, Motor, Leader };

  enum class DivisionKind : std::uint8_t { Panzer, Motorized, DasReich, Infantry, Cavalry, Independent };
  struct Division {
    DivisionKind kind;
    int number = 0;
    auto operator<=>(const Division&) const = default;
  };

  enum class PhaseKind : std::uint8_t {
    SetUp, Move, MechanizedMove, Combat, DisruptionRemoval, SovietInterdiction, AirInterdiction, Container
  };
  struct PhaseInfo {
    PhaseKind kind;
    std::optional<SideId> side;
  };

  // German entrance areas A-H (16.2), Soviet entrance hexes V W X Z (16.1), provisional areas 1-6 (14.1).
  enum class Area : std::uint8_t { A, B, C, D, E, F, G, H, V, W, X, Z, P1, P2, P3, P4, P5, P6 };
  std::string_view areaName(Area);
  Area areaNamed(std::string_view);  // throws naming the text

  struct VictoryHex {
    HexIndex hex;
    int points = 0;
    std::string name;
  };
  struct VictoryLevel {
    std::optional<int> lowest;   // nullopt: no lower bound
    std::optional<int> highest;  // nullopt: no upper bound
    std::string code;            // the list item's code, e.g. "50-79"
    std::string name;            // "German Marginal"
    SideId winner;
  };

  class PggFacts {
  public:
    explicit PggFacts(const HexRules::GameDefinition&);

    // ---- sides ------------------------------------------------------------------------------------
    SideId german() const { return german_; }
    SideId soviet() const { return soviet_; }
    SideId enemy(SideId) const;

    // ---- counters ---------------------------------------------------------------------------------
    Arm arm(UnitId) const;
    bool markerP(UnitId unit) const { return Arm::Marker == arm(unit); }
    bool leaderP(UnitId unit) const { return Arm::SovietLeader == arm(unit); }
    bool sovietDivisionP(UnitId) const;  // a rifle or armoured division: untried, one step, from the pool
    MoveClass moveClass(UnitId) const;
    std::optional<Division> division(UnitId) const;
    const std::vector<UnitId>& members(Division) const;  // throws for a division no counter belongs to
    std::vector<Division> divisions() const;
    bool independentRegimentP(UnitId) const;             // GD and Lehr (amendments 7.3)
    std::optional<UnitId> successor(UnitId) const;       // a German infantry division's 9-7 -> its 2-7
    std::optional<UnitId> predecessor(UnitId) const;     // the inverse
    int leaderRating(UnitId) const;                      // throws for a unit that is not a Leader
    int railPoints(UnitId) const;                        // 6.31: rifle 1, armour 3, Leader 0
    UnitId counter(std::string_view id) const;           // throws for an unknown counter
    UnitTypeId unitType(std::string_view id) const;

    // ---- hexes ------------------------------------------------------------------------------------
    HexIndex hex(std::string_view id) const;  // throws for an unknown hex
    const std::string& terrainOf(HexIndex) const;
    bool lakeP(HexIndex) const;
    bool woodsP(HexIndex) const;
    bool swampP(HexIndex) const;
    bool majorCityP(HexIndex) const;
    bool minorCityP(HexIndex) const;
    bool riverP(HexIndex, Direction) const;
    bool lakeHexsideP(HexIndex, Direction) const;  // a hexside with a Lake hex on either side
    bool roadP(HexIndex, HexIndex) const;
    std::optional<std::size_t> railLink(HexIndex, HexIndex) const;
    bool railHexP(HexIndex) const;
    int column(HexIndex) const;
    bool westEdgeP(HexIndex hex) const { return 1 == column(hex); }
    bool eastEdgeP(HexIndex) const;
    bool southEdgeP(HexIndex) const;
    bool westmostColumnsP(HexIndex hex) const { return 2 >= column(hex); }  // hexrows 0100 and 0200 (6.4)
    std::optional<Direction> directionTo(HexIndex from, HexIndex to) const;
    HexIndex supplyRoadHex() const { return supplyRoad_; }  // 0120 (11.11)
    HexIndex smolensk() const { return smolensk_; }         // 2117
    const std::vector<VictoryHex>& victoryHexes() const { return victoryHexes_; }
    const std::vector<VictoryLevel>& victoryLevels() const { return victoryLevels_; }
    const std::vector<HexIndex>& area(Area) const;
    bool inAreaP(Area, HexIndex) const;
    int southWesternFrontColumn() const;  // 14.22: entrance hex Z's column

    // ---- ids the rules document names ---------------------------------------------------------------
    PhaseInfo phase(PhaseId) const;
    PhaseId phaseNamed(std::string_view id) const;
    ModeId normalMode() const { return normal_; }
    ModeId railMode() const { return rail_; }
    NetworkId roadNetwork() const { return roadNet_; }
    NetworkId railNetwork() const { return railNet_; }
    SpaceId deadPile() const { return deadPile_; }
    SpaceId sovietPool() const { return sovietPool_; }
    SpaceId sovietArrivals() const { return sovietArrivals_; }
    SpaceId germanArrivals() const { return germanArrivals_; }
    const HexRules::GameDefinition& definition() const { return definition_; }

    // The data the rules need and no input carries, in words (reported by the package test and in the
    // task file, never silently papered over).
    const std::vector<std::string>& dataGaps() const { return gaps_; }

  private:
    void readCounters();
    void readDivisions();
    void readHexes();
    void readLists();
    void readAreas();
    void readPhases();

    const HexRules::GameDefinition& definition_;
    SideId german_, soviet_;
    std::vector<Arm> arms_;
    std::vector<std::optional<Division>> divisionOf_;
    std::map<Division, std::vector<UnitId>> members_;
    std::map<std::uint32_t, UnitId> successor_, predecessor_;
    std::vector<std::uint8_t> lake_, woods_, swamp_, major_, minor_;
    int lastColumn_ = 0;
    int lastRow_ = 0;
    HexIndex supplyRoad_, smolensk_;
    EdgeTerrainId riverEdge_;
    std::vector<VictoryHex> victoryHexes_;
    std::vector<VictoryLevel> victoryLevels_;
    std::map<Area, std::vector<HexIndex>> areas_;
    std::map<std::uint32_t, PhaseInfo> phases_;
    ModeId normal_, rail_;
    NetworkId roadNet_, railNet_;
    SpaceId deadPile_, sovietPool_, sovietArrivals_, germanArrivals_;
    std::vector<std::string> gaps_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
