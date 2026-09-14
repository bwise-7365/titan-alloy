// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The map facts: terrain and drawn cities, rivers, roads and rail, the edges, the Victory Point
// hexes and levels (the rules document's lists german-city-vp and victory-levels) and the entrance
// areas.
// ----------------------------------------------
#include "PggFacts.h"

#include "hexxml/PackageDoc.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/XmlDocument.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <stdexcept>

namespace Pgg {

  namespace {

    constexpr std::array<std::string_view, 18> kAreaNames{"A", "B", "C", "D", "E", "F", "G", "H", "V",
                                                          "W", "X", "Z", "1", "2", "3", "4", "5", "6"};

    // TODO(decide): PROVISIONAL. The printed map marks the entrance areas; no input carries their
    // hexes except area B (14.3: 0101-0110 and 0112). Until the sheet or the rules name them, the
    // German areas run down the west edge, V and W sit on the north edge, X on the east edge toward
    // Moscow, Z on the south edge, and the provisional areas 1-6 round the north, east and south edges.
    struct AreaHexes {
      Area area;
      std::vector<std::string_view> hexes;
    };
    const std::vector<AreaHexes>&
    areaTable()
    {
      static const std::vector<AreaHexes> table{
          {Area::A, {"0301", "0401", "0501", "0601"}},
          {Area::B, {"0101", "0102", "0103", "0104", "0105", "0106", "0107", "0108", "0109", "0110", "0112"}},
          {Area::C, {"0113", "0114", "0115", "0116", "0117", "0118"}},
          {Area::D, {"0119", "0120", "0121"}},
          {Area::E, {"0122", "0123", "0124", "0125", "0126"}},
          {Area::F, {"0127", "0128", "0129"}},
          {Area::G, {"0130", "0131"}},
          {Area::H, {"0231", "0331", "0431", "0531"}},
          {Area::V, {"1001", "1101", "1201", "1301"}},
          {Area::W, {"2601", "2701", "2801", "2901"}},
          {Area::X, {"5612", "5613", "5614", "5615", "5616", "5617"}},
          {Area::Z, {"3031"}},
          {Area::P1, {"2001"}},
          {Area::P2, {"3601"}},
          {Area::P3, {"5601"}},
          {Area::P4, {"5625"}},
          {Area::P5, {"4531"}},
          {Area::P6, {"2531"}},
      };
      return table;
    }

    int
    twoDigits(std::string_view text, const std::string& id)
    {
      int value = 0;
      const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
      if (std::errc() != error || text.data() + text.size() != end) {
        throw std::invalid_argument("PggFacts: hex id '" + id + "' is not {col:02}{row:02}");
      }
      return value;
    }

    const HexXml::ListAnnex&
    listNamed(const HexXml::RulesDoc& doc, std::string_view id)
    {
      for (const HexXml::ListAnnex& list : doc.allLists) {
        if (id == list.id) {
          return list;
        }
      }
      throw std::invalid_argument("PggFacts: the rules document has no list '" + std::string(id) + "'");
    }

    // "Rzhev: 5", "Smolensk: 25 (reduced ...)": the name before the colon, the number after it.
    VictoryHex
    victoryHexOf(const HexXml::ListItemDoc& item, const PggFacts& facts)
    {
      const std::size_t colon = item.text.find(':');
      if (!item.code || std::string::npos == colon) {
        throw std::invalid_argument("PggFacts: german-city-vp item '" + item.text + "' is not '<hex>: <name>: <points>'");
      }
      std::size_t digits = colon + 1;
      while (digits < item.text.size() && ' ' == item.text[digits]) {
        ++digits;
      }
      int points = 0;
      const char* begin = item.text.data() + digits;
      const auto [end, error] = std::from_chars(begin, item.text.data() + item.text.size(), points);
      if (std::errc() != error || begin == end) {
        throw std::invalid_argument("PggFacts: german-city-vp item '" + item.text + "' names no points");
      }
      return VictoryHex{facts.hex(*item.code), points, item.text.substr(0, colon)};
    }

    // "<=0", "1-25", "125+".
    VictoryLevel
    victoryLevelOf(const HexXml::ListItemDoc& item, SideId german, SideId soviet)
    {
      if (!item.code) {
        throw std::invalid_argument("PggFacts: victory-levels item '" + item.text + "' has no code");
      }
      const std::string& code = *item.code;
      VictoryLevel level;
      level.code = code;
      level.name = item.text;
      if (code.starts_with("<=")) {
        level.highest = std::stoi(code.substr(2));
      } else if (code.ends_with("+")) {
        level.lowest = std::stoi(code.substr(0, code.size() - 1));
      } else {
        const std::size_t dash = code.find('-');
        if (std::string::npos == dash) {
          throw std::invalid_argument("PggFacts: victory-levels code '" + code + "' is not a range");
        }
        level.lowest = std::stoi(code.substr(0, dash));
        level.highest = std::stoi(code.substr(dash + 1));
      }
      if (item.text.starts_with("German")) {
        level.winner = german;
      } else if (item.text.starts_with("Soviet")) {
        level.winner = soviet;
      } else {
        throw std::invalid_argument("PggFacts: victory level '" + item.text + "' names neither side");
      }
      return level;
    }

  }  // namespace

  std::string_view
  areaName(Area area)
  {
    return kAreaNames[static_cast<std::size_t>(area)];
  }

  Area
  areaNamed(std::string_view text)
  {
    for (std::size_t i = 0; i < kAreaNames.size(); ++i) {
      if (text == kAreaNames[i]) {
        return static_cast<Area>(i);
      }
    }
    throw std::invalid_argument("Pgg::areaNamed: '" + std::string(text) + "' is not an entrance area");
  }

  void
  PggFacts::readHexes()
  {
    const HexModel::Board& board = *definition_.board;
    const HexRules::RuleSet& rules = *definition_.rules;
    const std::size_t count = board.hexCount();
    lake_.assign(count, 0);
    woods_.assign(count, 0);
    swamp_.assign(count, 0);
    major_.assign(count, 0);
    minor_.assign(count, 0);
    const TerrainId majorCity = rules.terrain("major-city");
    const TerrainId minorCity = rules.terrain("minor-city");
    for (std::size_t h = 0; h < count; ++h) {
      const HexIndex hex{static_cast<std::uint32_t>(h)};
      const std::string& terrain = terrainOf(hex);
      lake_[h] = "lake" == terrain;
      woods_[h] = "woods" == terrain;
      swamp_[h] = "swamp" == terrain;
      for (const Feature& feature : board.features(hex)) {
        major_[h] = major_[h] || majorCity == feature.terrain;
        minor_[h] = minor_[h] || minorCity == feature.terrain;
      }
      lastColumn_ = std::max(lastColumn_, column(hex));
      lastRow_ = std::max(lastRow_, twoDigits(std::string_view(board.id(hex).text).substr(2), board.id(hex).text));
    }
    riverEdge_ = rules.edgeTerrain("river");
    supplyRoad_ = hex("0120");
    smolensk_ = hex("2117");
    gaps_.push_back("entrance areas A and C-H, V, W, X, Z and 1-6: the printed map marks them and no input carries "
                    "their hexes, so they are provisional (PggFactsMap.cpp)");
    gaps_.push_back("no Railroad hex lies on the south edge, so South-Western Front divisions (14.22) enter on any "
                    "south-edge hex at or east of entrance hex Z");
    if (board.network(board.networkId("road-net")).linksAt(supplyRoad_).empty()) {
      gaps_.push_back("the sheet draws no road into hex 0120, so no road leads there and German supply comes only "
                      "from the twenty-point trace to the west edge (11.11, 11.12)");
    }
    return;
  }

  void
  PggFacts::readLists()
  {
    const HexXml::PackageDoc package = HexXml::PackageDoc::parse(HexXml::XmlDocument::load(definition_.packagePath));
    const HexXml::RulesDoc doc =
        HexXml::RulesDoc::parse(HexXml::XmlDocument::load(definition_.packagePath.parent_path() / package.rules.path));
    for (const HexXml::ListItemDoc& item : listNamed(doc, "german-city-vp").items) {
      if (item.code && !definition_.board->find(HexCoord::HexId{*item.code})) {
        gaps_.push_back("Victory Point hex " + *item.code + " (" + item.text +
                        ") lies outside the sheet's 56-column grid, so it scores nothing");
        continue;
      }
      victoryHexes_.push_back(victoryHexOf(item, *this));
    }
    for (const HexXml::ListItemDoc& item : listNamed(doc, "victory-levels").items) {
      victoryLevels_.push_back(victoryLevelOf(item, german_, soviet_));
    }
    return;
  }

  void
  PggFacts::readAreas()
  {
    for (const AreaHexes& entry : areaTable()) {
      for (std::string_view id : entry.hexes) {
        areas_[entry.area].push_back(hex(id));
      }
    }
    return;
  }

  HexIndex
  PggFacts::hex(std::string_view id) const
  {
    return definition_.board->indexOf(HexCoord::HexId{std::string(id)});
  }

  const std::string&
  PggFacts::terrainOf(HexIndex hex) const
  {
    return definition_.rules->hexTerrain()[definition_.board->terrain(hex).value].id;
  }

  bool
  PggFacts::lakeP(HexIndex hex) const
  {
    return 0 != lake_.at(hex.value);
  }

  bool
  PggFacts::woodsP(HexIndex hex) const
  {
    return 0 != woods_.at(hex.value);
  }

  bool
  PggFacts::swampP(HexIndex hex) const
  {
    return 0 != swamp_.at(hex.value);
  }

  bool
  PggFacts::majorCityP(HexIndex hex) const
  {
    return 0 != major_.at(hex.value);
  }

  bool
  PggFacts::minorCityP(HexIndex hex) const
  {
    return 0 != minor_.at(hex.value);
  }

  bool
  PggFacts::riverP(HexIndex hex, Direction direction) const
  {
    const std::vector<EdgeTerrainId>& edge = definition_.board->edge(hex, direction);
    return edge.end() != std::find(edge.begin(), edge.end(), riverEdge_);
  }

  bool
  PggFacts::lakeHexsideP(HexIndex hex, Direction direction) const
  {
    const std::optional<HexIndex> across = definition_.board->neighbour(hex, direction);
    return lakeP(hex) || (across && lakeP(*across));
  }

  bool
  PggFacts::roadP(HexIndex a, HexIndex b) const
  {
    const HexModel::LinkNetwork& roads = definition_.board->network(roadNet_);
    for (std::size_t link : roads.linksAt(a)) {
      const HexModel::LinkNetwork::Link& l = roads.links()[link];
      if ((l.a == a && l.b == b) || (l.a == b && l.b == a)) {
        return true;
      }
    }
    return false;
  }

  std::optional<std::size_t>
  PggFacts::railLink(HexIndex a, HexIndex b) const
  {
    const HexModel::LinkNetwork& rail = definition_.board->network(railNet_);
    for (std::size_t link : rail.linksAt(a)) {
      const HexModel::LinkNetwork::Link& l = rail.links()[link];
      if ((l.a == a && l.b == b) || (l.a == b && l.b == a)) {
        return link;
      }
    }
    return std::nullopt;
  }

  bool
  PggFacts::railHexP(HexIndex hex) const
  {
    return !definition_.board->network(railNet_).linksAt(hex).empty();
  }

  int
  PggFacts::column(HexIndex hex) const
  {
    const std::string& id = definition_.board->id(hex).text;
    if (4 != id.size()) {
      throw std::invalid_argument("PggFacts: hex id '" + id + "' is not {col:02}{row:02}");
    }
    return twoDigits(std::string_view(id).substr(0, 2), id);
  }

  bool
  PggFacts::eastEdgeP(HexIndex hex) const
  {
    return lastColumn_ == column(hex);
  }

  bool
  PggFacts::southEdgeP(HexIndex hex) const
  {
    const std::string& id = definition_.board->id(hex).text;
    return lastRow_ == twoDigits(std::string_view(id).substr(2), id);
  }

  std::optional<Direction>
  PggFacts::directionTo(HexIndex from, HexIndex to) const
  {
    for (int d = 0; d < HexCoord::kDirections; ++d) {
      const Direction direction = static_cast<Direction>(d);
      if (definition_.board->neighbour(from, direction) == to) {
        return direction;
      }
    }
    return std::nullopt;
  }

  const std::vector<HexIndex>&
  PggFacts::area(Area which) const
  {
    const auto found = areas_.find(which);
    if (areas_.end() == found) {
      throw std::invalid_argument("PggFacts: entrance area '" + std::string(areaName(which)) + "' has no hexes");
    }
    return found->second;
  }

  bool
  PggFacts::inAreaP(Area which, HexIndex hex) const
  {
    const std::vector<HexIndex>& hexes = area(which);
    return hexes.end() != std::find(hexes.begin(), hexes.end(), hex);
  }

  int
  PggFacts::southWesternFrontColumn() const
  {
    return column(area(Area::Z).front());
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
