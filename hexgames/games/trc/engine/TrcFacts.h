// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Everything the TRC policies read off the immutable definition, looked up once: the two sides,
// what each counter is (type, echelon, nation), which hexes are cities, rail, river, water or a
// board edge, the Kerch Strait, the sea areas, and the ids of the phases, modes, modifiers and
// spaces the rules name. Built from the GameDefinition plus the counters document (for echelons);
// every id the rules document or the prose names is resolved here and throws if absent.
// ----------------------------------------------
#pragma once
#include "hexengine/Policies.h"
#include "hexrules/Package.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Trc {

  using namespace HexModel;
  using HexEngine::Ctx;

  enum class Echelon : std::uint8_t { None, Corps, Army, ArmyGroup };
  enum class Nation : std::uint8_t { German, Finnish, Hungarian, Rumanian, Italian, Russian };
  enum class Impulse : std::uint8_t { First, Second };
  enum class PhaseKind : std::uint8_t { Weather, Move, Combat, End, SuddenDeath };
  enum class SeaArea : std::uint8_t { Baltic, BlackSea, Caspian };

  std::string_view nationName(Nation);  // the counter style: "finnish", "hungarian" ...

  struct PhaseInfo {
    PhaseKind kind;
    std::optional<SideId> side;        // the side the phase belongs to (nullopt: weather, sudden death)
    std::optional<Impulse> impulse;    // move and combat phases only
  };

  class TrcFacts {
  public:
    explicit TrcFacts(const HexRules::GameDefinition&);

    // ---- sides ------------------------------------------------------------------------------------
    SideId axis() const { return axis_; }
    SideId russian() const { return russian_; }
    SideId enemy(SideId) const;

    // ---- counters ---------------------------------------------------------------------------------
    const std::string& typeOf(UnitId) const;  // the rules unit-type id
    bool typeP(UnitId, std::string_view type) const;
    Echelon echelon(UnitId) const;
    Nation nation(UnitId) const;
    bool mobileP(UnitId) const;   // armour, motorized, panzer grenadier, cavalry
    UnitId counter(std::string_view id) const;  // throws for an unknown counter
    UnitId hitler() const { return hitler_; }
    UnitId stalin() const { return stalin_; }
    UnitId stavka() const { return stavka_; }
    bool noStackingValueP(UnitId) const;  // 6.3
    bool germanHqP(UnitId) const;

    // ---- hexes ------------------------------------------------------------------------------------
    bool waterP(HexIndex) const;
    bool majorCityP(HexIndex) const;
    bool cityP(HexIndex) const;
    bool oilP(HexIndex) const;
    bool railHexP(HexIndex) const;
    bool junctionP(HexIndex) const;        // a non-city rail hex where three or more links meet
    bool controlPointP(HexIndex) const;    // city, oil well or junction (17.2.1)
    bool riverHexP(HexIndex) const;
    std::optional<std::uint32_t> riverOf(HexIndex) const;  // connected river, for 14.1.1
    bool kerchP(HexIndex, HexIndex) const;  // the hexside between KK19 and KK20 (8.5)
    bool ownEdgeP(SideId, HexIndex) const;  // west for the Axis, east or south for the Russians
    bool southEntryP(HexIndex) const;       // QQ5 to QQ16 (20.5)
    HexIndex named(std::string_view name) const;  // a sheet hex name, case-insensitive; throws
    std::vector<HexIndex> oilWells() const;
    std::optional<SeaArea> seaAreaOfPort(HexIndex) const;  // a named port of the rules' sea areas
    std::vector<SeaArea> seasTouching(HexIndex) const;     // the sea areas a land hex borders
    bool inCountryP(HexIndex, std::string_view region) const;  // the countries layer
    bool blockedHexsideP(HexIndex, Direction) const;

    // The data the rules need and the documents do not carry yet, in words (reported by the
    // package test and in the task file, never silently papered over).
    const std::vector<std::string>& dataGaps() const { return gaps_; }

    // ---- ids the rules document names ---------------------------------------------------------------
    PhaseInfo phase(PhaseId) const;
    PhaseId phaseNamed(std::string_view id) const;
    ModeId normalMode() const { return normal_; }
    ModeId railMode() const { return railMode_; }
    // The rail link between two hexes, if the sheet draws one.
    std::optional<std::size_t> railLink(HexIndex, HexIndex) const;
    ModifierId stuka() const { return stuka_; }
    ModifierId sturmovik() const { return sturmovik_; }
    NetworkId railNetwork() const { return railNetwork_; }
    SpaceId omb() const { return omb_; }
    SpaceId pool(SideId) const;
    SpaceId surrendered(SideId) const;
    SpaceId stukasBox() const { return stukas_; }
    TrackId turnTrack() const { return turnTrack_; }
    UnitTypeId unitType(std::string_view) const;
    const HexRules::GameDefinition& definition() const { return definition_; }

  private:
    void readCounters();
    void readHexes();
    void readRivers();
    void readSeas();
    void readPhases();

    const HexRules::GameDefinition& definition_;
    SideId axis_, russian_;
    std::vector<Echelon> echelons_;
    std::vector<Nation> nations_;
    UnitId hitler_, stalin_, stavka_, ssReserve_;
    std::vector<bool> water_, major_, city_, oil_, rail_, junction_, river_, west_, east_, south_;
    std::vector<std::optional<std::uint32_t>> riverOf_;
    std::vector<std::optional<SeaArea>> seaOf_;  // per water hex
    std::map<std::string, HexIndex> named_;      // upper-cased sheet names
    HexIndex kerchA_, kerchB_;
    EdgeTerrainId riverEdge_, blockedEdge_;
    std::map<std::uint32_t, PhaseInfo> phases_;
    ModeId normal_, railMode_;
    ModifierId stuka_, sturmovik_;
    NetworkId railNetwork_;
    LayerId countries_;
    SpaceId omb_, axisPool_, russianPool_, axisSurrendered_, russianSurrendered_, stukas_;
    TrackId turnTrack_;
    std::vector<std::string> gaps_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
