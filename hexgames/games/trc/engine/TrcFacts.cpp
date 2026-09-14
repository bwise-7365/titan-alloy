// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcFacts.h"

#include "hexxml/CounterSetDoc.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/XmlDocument.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace Trc {

  namespace {

    std::string
    upper(std::string_view text)
    {
      std::string out(text);
      for (char& c : out) {
        if ('a' <= c && 'z' >= c) {
          c = static_cast<char>(c - 'a' + 'A');
        }
      }
      return out;
    }

    Echelon
    echelonNamed(const std::string& level, const std::string& counter)
    {
      if ("corps" == level) {
        return Echelon::Corps;
      }
      if ("army" == level) {
        return Echelon::Army;
      }
      if ("army-group" == level) {
        return Echelon::ArmyGroup;
      }
      throw std::invalid_argument("TrcFacts: counter '" + counter + "' prints echelon '" + level +
                                   "', which TRC's stacking rule (6.1) does not know");
    }

    Nation
    nationOfStyle(const std::string& style, const std::string& counter)
    {
      if ("german" == style || "ss" == style || "luftwaffe" == style) {
        return Nation::German;
      }
      if ("finnish" == style) {
        return Nation::Finnish;
      }
      if ("hungarian" == style) {
        return Nation::Hungarian;
      }
      if ("rumanian" == style) {
        return Nation::Rumanian;
      }
      if ("italian" == style) {
        return Nation::Italian;
      }
      if ("russian" == style || "guards" == style || "worker" == style) {
        return Nation::Russian;
      }
      throw std::invalid_argument("TrcFacts: counter '" + counter + "' has style '" + style +
                                   "', which names no TRC nation");
    }

    std::string
    rowLetters(const std::string& id)
    {
      std::string out;
      for (char c : id) {
        if (0 == std::isalpha(static_cast<unsigned char>(c))) {
          break;
        }
        out += c;
      }
      return out;
    }

    int
    columnNumber(const std::string& id)
    {
      return std::stoi(id.substr(rowLetters(id).size()));
    }

    bool
    edgeNamedP(const HexRules::RuleSet& rules, const std::vector<EdgeTerrainId>& edges, const char* id)
    {
      for (EdgeTerrainId edge : edges) {
        if (id == rules.hexsideTerrain()[edge.value].id) {
          return true;
        }
      }
      return false;
    }

    ModeId
    modeNamed(const HexRules::RuleSet& rules, const char* id)
    {
      for (std::size_t i = 0; i < rules.movement().modes.size(); ++i) {
        if (id == rules.movement().modes[i].id) {
          return ModeId{static_cast<std::uint32_t>(i)};
        }
      }
      throw std::invalid_argument(std::string("TrcFacts: the rules document has no movement mode '") + id + "'");
    }

    ModifierId
    modifierNamed(const HexRules::RuleSet& rules, const char* id)
    {
      for (std::size_t i = 0; i < rules.combat().modifiers.size(); ++i) {
        if (id == rules.combat().modifiers[i].id) {
          return ModifierId{static_cast<std::uint32_t>(i)};
        }
      }
      throw std::invalid_argument(std::string("TrcFacts: the rules document has no modifier '") + id + "'");
    }

  }  // namespace

  std::string_view
  nationName(Nation nation)
  {
    switch (nation) {
      case Nation::German:
        return "german";
      case Nation::Finnish:
        return "finnish";
      case Nation::Hungarian:
        return "hungarian";
      case Nation::Rumanian:
        return "rumanian";
      case Nation::Italian:
        return "italian";
      case Nation::Russian:
        return "russian";
    }
    throw std::invalid_argument("Trc::nationName: nation outside the six");
  }

  TrcFacts::TrcFacts(const HexRules::GameDefinition& definition)
    : definition_(definition),
      axis_(definition.rules->side("axis")),
      russian_(definition.rules->side("russian")),
      hitler_(counter("g-ge-hitler-hitler")),
      stalin_(counter("r-ru-stalin-stalin")),
      stavka_(counter("r-ru-stavka-hq")),
      ssReserve_(counter("g-ss-res-infantry")),
      kerchA_(definition.board->indexOf(HexCoord::HexId{"KK19"})),
      kerchB_(definition.board->indexOf(HexCoord::HexId{"KK20"})),
      riverEdge_(definition.rules->edgeTerrain("river")),
      blockedEdge_(definition.rules->edgeTerrain("blocked")),
      normal_(modeNamed(*definition.rules, "normal")),
      railMode_(modeNamed(*definition.rules, "rail-move")),
      stuka_(modifierNamed(*definition.rules, "stuka-shift")),
      sturmovik_(modifierNamed(*definition.rules, "sturmovik-shift")),
      railNetwork_(definition.board->networkId("rail")),
      countries_(definition.board->layerId("countries")),
      omb_(definition.board->spaceId("omb")),
      axisPool_(definition.board->spaceId("axis-pool")),
      russianPool_(definition.board->spaceId("russian-pool")),
      axisSurrendered_(definition.board->spaceId("axis-surrendered")),
      russianSurrendered_(definition.board->spaceId("russian-surrendered")),
      stukas_(definition.board->spaceId("stukas")),
      turnTrack_(definition.board->trackId("turn-track"))
  {
    readCounters();
    readHexes();
    readRivers();
    readSeas();
    readPhases();
  }

  void
  TrcFacts::readCounters()
  {
    const HexXml::PackageDoc package =
        HexXml::PackageDoc::parse(HexXml::XmlDocument::load(definition_.packagePath));
    if (package.counters.empty()) {
      throw std::invalid_argument("TrcFacts: package '" + package.id + "' names no counters document");
    }
    const HexXml::CounterSetDoc counters = HexXml::CounterSetDoc::parse(
        HexXml::XmlDocument::load(definition_.packagePath.parent_path() / package.counters.front().path));
    std::map<std::string, Echelon> byCounter;
    for (const HexXml::CounterDoc& doc : counters.counters) {
      byCounter[doc.id] =
          doc.front.echelons.empty() ? Echelon::None : echelonNamed(doc.front.echelons.front().level, doc.id);
    }
    for (const UnitSpec& spec : definition_.roster->units()) {
      const std::string base = spec.counter.text.substr(0, spec.counter.text.find('#'));
      const auto found = byCounter.find(base);
      if (byCounter.end() == found) {
        throw std::invalid_argument("TrcFacts: roster counter '" + spec.counter.text +
                                     "' is not in the counters document");
      }
      echelons_.push_back(found->second);
      nations_.push_back(nationOfStyle(spec.nationality, spec.counter.text));
    }
    return;
  }

  void
  TrcFacts::readHexes()
  {
    const Board& board = *definition_.board;
    const HexRules::RuleSet& rules = *definition_.rules;
    const std::size_t n = board.hexCount();
    water_.assign(n, false);
    major_.assign(n, false);
    city_.assign(n, false);
    oil_.assign(n, false);
    rail_.assign(n, false);
    junction_.assign(n, false);
    west_.assign(n, false);
    east_.assign(n, false);
    south_.assign(n, false);
    const Direction westward = HexCoord::fromCompass("w", board.orientation());
    const Direction eastward = HexCoord::fromCompass("e", board.orientation());
    const LinkNetwork& network = board.network(railNetwork_);

    for (std::size_t h = 0; h < n; ++h) {
      const HexIndex hex{static_cast<std::uint32_t>(h)};
      const std::string& terrain = rules.hexTerrain()[board.terrain(hex).value].id;
      water_[h] = "sea" == terrain || "lake" == terrain;
      for (const Feature& feature : board.features(hex)) {
        const std::string& drawn = rules.hexTerrain()[feature.terrain.value].id;
        major_[h] = major_[h] || "major-city" == drawn;
        city_[h] = city_[h] || "major-city" == drawn || "minor-city" == drawn;
        oil_[h] = oil_[h] || "oil-field" == drawn;
        if (feature.name) {
          named_[upper(*feature.name)] = hex;
        }
      }
      rail_[h] = !network.linksAt(hex).empty();
      junction_[h] = rail_[h] && !city_[h] && 3 <= network.linksAt(hex).size();
      west_[h] = !board.neighbour(hex, westward).has_value();
      east_[h] = !board.neighbour(hex, eastward).has_value();
      south_[h] = "QQ" == rowLetters(board.id(hex).text);
    }

    std::string wet;
    for (const auto& [name, hex] : named_) {
      if (water_[hex.value] && city_[hex.value]) {
        wet += (wet.empty() ? "" : ", ") + board.id(hex).text + " " + name;
      }
    }
    if (!wet.empty()) {
      gaps_.push_back("the sheet draws cities on sea or lake hexes, which no unit can enter: " + wet);
    }

    bool membersP = false;
    const RegionLayer& countries = board.layer(countries_);
    for (std::size_t r = 0; r < countries.regionCount(); ++r) {
      membersP = membersP || !countries.members(RegionId{static_cast<std::uint32_t>(r)}).empty();
    }
    if (!membersP) {
      gaps_.push_back("the sheet has no <region> membership for layer 'countries', so no hex is in Russia, "
                      "Hungary, Finland or Germany (24.0, 17.3, 19.1, 25.0 1945 objectives)");
    }
    return;
  }

  void
  TrcFacts::readRivers()
  {
    const Board& board = *definition_.board;
    river_.assign(board.hexCount(), false);
    riverOf_.assign(board.hexCount(), std::nullopt);
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const std::vector<EdgeTerrainId>& edges = board.edge(HexIndex{static_cast<std::uint32_t>(h)}, static_cast<Direction>(d));
        river_[h] = river_[h] || edges.end() != std::find(edges.begin(), edges.end(), riverEdge_);
      }
    }
    // One river is the river hexes joined through river hexsides (14.1.1: "a river crossing the
    // hexside between two river hexes connects them").
    std::uint32_t next = 0;
    for (std::size_t h = 0; h < board.hexCount(); ++h) {
      if (!river_[h] || riverOf_[h]) {
        continue;
      }
      const HexIndex seed{static_cast<std::uint32_t>(h)};
      for (HexIndex member : HexSearch::regionFlood(board, seed, [&](HexIndex from, Direction d) {
             const std::vector<EdgeTerrainId>& edges = board.edge(from, d);
             return edges.end() == std::find(edges.begin(), edges.end(), riverEdge_);
           })) {
        riverOf_[member.value] = next;
      }
      ++next;
    }
    return;
  }

  void
  TrcFacts::readSeas()
  {
    const Board& board = *definition_.board;
    const HexRules::RuleSet& rules = *definition_.rules;
    seaOf_.assign(board.hexCount(), std::nullopt);
    const auto seaP = [&](HexIndex hex) {
      return "sea" == rules.hexTerrain()[board.terrain(hex).value].id;
    };
    const auto label = [&](std::string_view town, SeaArea area) {
      const HexIndex port = named(town);
      for (int d = 0; d < HexCoord::kDirections; ++d) {
        const std::optional<HexIndex> water = board.neighbour(port, static_cast<Direction>(d));
        if (!water || !seaP(*water) || seaOf_[water->value]) {
          continue;
        }
        for (HexIndex member : HexSearch::regionFlood(board, *water, [&](HexIndex from, Direction step) {
               const std::optional<HexIndex> to = board.neighbour(from, step);
               return !to || !seaP(*to);
             })) {
          seaOf_[member.value] = area;
        }
      }
      return;
    };
    for (const char* port : {"RIGA", "TALLINN", "HELSINKI", "LENINGRAD"}) {
      label(port, SeaArea::Baltic);
    }
    for (const char* port : {"ODESSA", "SEVASTOPOL", "ROSTOV"}) {
      label(port, SeaArea::BlackSea);
    }
    label("ASTRAKHAN", SeaArea::Caspian);
    return;
  }

  void
  TrcFacts::readPhases()
  {
    const HexRules::RuleSet& rules = *definition_.rules;
    const auto add = [&](const std::string& id, PhaseInfo info) {
      phases_[rules.phase(id).value] = info;
      return;
    };
    add("weather-phase", PhaseInfo{PhaseKind::Weather, std::nullopt, std::nullopt});
    add("sudden-death", PhaseInfo{PhaseKind::SuddenDeath, std::nullopt, std::nullopt});
    for (SideId side : {axis_, russian_}) {
      const std::string prefix = rules.sides()[side.value].id;
      add(prefix + "-i1-move", PhaseInfo{PhaseKind::Move, side, Impulse::First});
      add(prefix + "-i1-combat", PhaseInfo{PhaseKind::Combat, side, Impulse::First});
      add(prefix + "-i2-move", PhaseInfo{PhaseKind::Move, side, Impulse::Second});
      add(prefix + "-i2-combat", PhaseInfo{PhaseKind::Combat, side, Impulse::Second});
      add(prefix + "-end", PhaseInfo{PhaseKind::End, side, std::nullopt});
    }
    return;
  }

  SideId
  TrcFacts::enemy(SideId side) const
  {
    return axis_ == side ? russian_ : axis_;
  }

  const std::string&
  TrcFacts::typeOf(UnitId unit) const
  {
    return definition_.rules->unitTypes()[definition_.roster->unit(unit).type.value].id;
  }

  bool
  TrcFacts::typeP(UnitId unit, std::string_view type) const
  {
    return type == typeOf(unit);
  }

  Echelon
  TrcFacts::echelon(UnitId unit) const
  {
    return echelons_.at(unit.value);
  }

  Nation
  TrcFacts::nation(UnitId unit) const
  {
    return nations_.at(unit.value);
  }

  bool
  TrcFacts::mobileP(UnitId unit) const
  {
    return typeP(unit, "armour") || typeP(unit, "motorized") || typeP(unit, "panzer-grenadier") ||
           typeP(unit, "cavalry");
  }

  UnitId
  TrcFacts::counter(std::string_view id) const
  {
    const std::optional<UnitId> found = definition_.roster->find(CounterId{std::string(id)});
    if (!found) {
      throw std::invalid_argument("TrcFacts: the roster has no counter '" + std::string(id) + "'");
    }
    return *found;
  }

  bool
  TrcFacts::noStackingValueP(UnitId unit) const
  {
    if (ssReserve_ == unit) {
      return true;
    }
    const HexRules::StackingSpec& spec = definition_.rules->stacking();
    const UnitTypeId type = definition_.roster->unit(unit).type;
    if (spec.exempt.end() != std::find(spec.exempt.begin(), spec.exempt.end(), type)) {
      return true;
    }
    return "none" == definition_.rules->unitTypes()[type.value].stacking.value_or("");
  }

  bool
  TrcFacts::germanHqP(UnitId unit) const
  {
    return typeP(unit, "hq") && Nation::German == nation(unit);
  }

  bool
  TrcFacts::waterP(HexIndex hex) const
  {
    return water_.at(hex.value);
  }

  bool
  TrcFacts::majorCityP(HexIndex hex) const
  {
    return major_.at(hex.value);
  }

  bool
  TrcFacts::cityP(HexIndex hex) const
  {
    return city_.at(hex.value);
  }

  bool
  TrcFacts::oilP(HexIndex hex) const
  {
    return oil_.at(hex.value);
  }

  bool
  TrcFacts::railHexP(HexIndex hex) const
  {
    return rail_.at(hex.value);
  }

  bool
  TrcFacts::junctionP(HexIndex hex) const
  {
    return junction_.at(hex.value);
  }

  bool
  TrcFacts::controlPointP(HexIndex hex) const
  {
    return cityP(hex) || oilP(hex) || junctionP(hex);
  }

  bool
  TrcFacts::riverHexP(HexIndex hex) const
  {
    return river_.at(hex.value);
  }

  std::optional<std::uint32_t>
  TrcFacts::riverOf(HexIndex hex) const
  {
    return riverOf_.at(hex.value);
  }

  bool
  TrcFacts::kerchP(HexIndex a, HexIndex b) const
  {
    return (kerchA_ == a && kerchB_ == b) || (kerchA_ == b && kerchB_ == a);
  }

  bool
  TrcFacts::ownEdgeP(SideId side, HexIndex hex) const
  {
    if (axis_ == side) {
      return west_.at(hex.value);
    }
    return east_.at(hex.value) || south_.at(hex.value);
  }

  bool
  TrcFacts::southEntryP(HexIndex hex) const
  {
    const std::string& id = definition_.board->id(hex).text;
    if (!south_.at(hex.value)) {
      return false;
    }
    const int column = columnNumber(id);
    return 5 <= column && 16 >= column;
  }

  HexIndex
  TrcFacts::named(std::string_view name) const
  {
    const auto found = named_.find(upper(name));
    if (named_.end() == found) {
      throw std::invalid_argument("TrcFacts: the sheet names no hex '" + std::string(name) + "'");
    }
    return found->second;
  }

  std::vector<HexIndex>
  TrcFacts::oilWells() const
  {
    std::vector<HexIndex> out;
    for (std::size_t h = 0; h < oil_.size(); ++h) {
      if (oil_[h]) {
        out.push_back(HexIndex{static_cast<std::uint32_t>(h)});
      }
    }
    return out;
  }

  std::optional<SeaArea>
  TrcFacts::seaAreaOfPort(HexIndex hex) const
  {
    for (const char* port : {"RIGA", "TALLINN", "HELSINKI", "LENINGRAD"}) {
      if (named(port) == hex) {
        return SeaArea::Baltic;
      }
    }
    for (const char* port : {"ODESSA", "SEVASTOPOL", "ROSTOV"}) {
      if (named(port) == hex) {
        return SeaArea::BlackSea;
      }
    }
    return std::nullopt;
  }

  std::vector<SeaArea>
  TrcFacts::seasTouching(HexIndex hex) const
  {
    std::vector<SeaArea> out;
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const std::optional<HexIndex> water = definition_.board->neighbour(hex, static_cast<Direction>(d));
      if (!water || !seaOf_[water->value]) {
        continue;
      }
      if (out.end() == std::find(out.begin(), out.end(), *seaOf_[water->value])) {
        out.push_back(*seaOf_[water->value]);
      }
    }
    return out;
  }

  bool
  TrcFacts::inCountryP(HexIndex hex, std::string_view region) const
  {
    const Board& board = *definition_.board;
    const RegionId wanted = board.regionId(countries_, std::string(region));
    const std::vector<RegionId>& regions = board.layer(countries_).regionsOf(hex);
    return regions.end() != std::find(regions.begin(), regions.end(), wanted);
  }

  bool
  TrcFacts::blockedHexsideP(HexIndex hex, Direction direction) const
  {
    const std::vector<EdgeTerrainId>& edges = definition_.board->edge(hex, direction);
    return edges.end() != std::find(edges.begin(), edges.end(), blockedEdge_);
  }

  PhaseInfo
  TrcFacts::phase(PhaseId id) const
  {
    const auto found = phases_.find(id.value);
    if (phases_.end() == found) {
      throw std::invalid_argument("TrcFacts: phase index " + std::to_string(id.value) +
                                   " is not a stop of TRC's sequence of play");
    }
    return found->second;
  }

  PhaseId
  TrcFacts::phaseNamed(std::string_view id) const
  {
    return definition_.rules->phase(std::string(id));
  }

  std::optional<std::size_t>
  TrcFacts::railLink(HexIndex a, HexIndex b) const
  {
    const LinkNetwork& network = definition_.board->network(railNetwork_);
    for (std::size_t link : network.linksAt(a)) {
      const LinkNetwork::Link& arc = network.links()[link];
      if ((arc.a == a && arc.b == b) || (arc.a == b && arc.b == a)) {
        return link;
      }
    }
    return std::nullopt;
  }

  SpaceId
  TrcFacts::pool(SideId side) const
  {
    return axis_ == side ? axisPool_ : russianPool_;
  }

  SpaceId
  TrcFacts::surrendered(SideId side) const
  {
    return axis_ == side ? axisSurrendered_ : russianSurrendered_;
  }

  UnitTypeId
  TrcFacts::unitType(std::string_view id) const
  {
    return definition_.rules->unitType(std::string(id));
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
