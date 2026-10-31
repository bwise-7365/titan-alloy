// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The settings of one generated map: the hexgen.xsd language (HexRules xsd/hexgen.xsd, whose
// documentation states what each setting does), read from a file or built by the GUI, and written
// back. validate() is the one boundary check; everything after it trusts the settings.
// ----------------------------------------------
#pragma once
#include "hexcoord/Direction.h"
#include "hexcoord/Grid.h"
#include "hexxml/XmlDocument.h"

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace HexGen {

  enum class Method : std::uint8_t { Fractal, Perlin };

  // The five terrain roles, in the order the terrain step applies them.
  enum class Role : std::uint8_t { Low, High, Cover, City, Rest };
  inline constexpr std::size_t kRoles = 5;

  // The four map edges, in the order the edge roads are laid (HexKrieg's order).
  enum class Side : std::uint8_t { Top, Bottom, Left, Right };

  std::string_view methodName(Method);
  std::string_view roleName(Role);
  std::string_view sideName(Side);

  struct TerrainSetting {
    std::string id;
    std::string name;  // the sheet's terrain name; the reader fills it with the id when absent
    Role role = Role::Rest;
    int percent = 0;  // of the hexes; always 0 for Role::Rest, which takes the remainder
    std::string fill;  // #rrggbb
    std::optional<double> moveCost;
    std::optional<double> roadCost;  // absent: no road enters this terrain
  };

  struct SideSetting {
    Side at = Side::Top;
    int roads = 0;
    int cities = 0;  // at most roads
  };

  struct RoadSettings {
    std::string line = "road";
    std::string stroke = "#666692";
    double width = 3.0;
    std::vector<SideSetting> sides;  // at most one per side
  };

  struct AltitudeSettings {
    double maxLand = 2000.0;
    double maxDepth = 200.0;
  };

  // The largest road-cost hexgen accepts: path costs are summed as integers in hundredths of a step,
  // and below this bound no path on a 300 x 300 map can overflow them.
  inline constexpr double kMaxRoadCost = 100.0;

  struct Settings {
    std::string id;
    std::string title;
    std::int64_t seed = 0;

    HexCoord::Orientation orientation = HexCoord::Orientation::Flat;
    HexCoord::Parity offset = HexCoord::Parity::Odd;
    int cols = 0;
    int rows = 0;
    double size = 30.0;
    std::optional<std::string> idFormat;  // absent: idFormatOrDefault()

    Method method = Method::Perlin;
    double roughness = 0.5;

    std::vector<TerrainSetting> terrains;  // five, one per role, in document order
    std::optional<RoadSettings> roads;     // absent: no roads
    std::optional<AltitudeSettings> altitude;  // absent: no alt written

    // Throws std::invalid_argument naming the file and line of what is wrong.
    static Settings load(const std::filesystem::path&);
    static Settings parse(const HexXml::XmlDocument&);
    // HexKrieg's Map tab: 18 x 30, Perlin 0.5, its default percentages, two roads and two cities on
    // the left and right edges (HexRules corpus/valid/generator-hexkrieg.xml).
    static Settings hexkrieg();

    // {col:02}{row:02}, or {col:03}{row:03} once either extent reaches 100 (as HexKrieg numbers).
    std::string idFormatOrDefault() const;
    std::size_t terrainIndex(Role) const;  // throws if no terrain has the role
    const TerrainSetting& terrain(Role role) const { return terrains[terrainIndex(role)]; }
  };

  // The sheet's one grid: id "main", numbered from 0, with a 40-pixel margin round the hexes.
  HexCoord::GridSpec gridSpecOf(const Settings&);
  inline constexpr double kMargin = 40.0;

  // The checks of hexgen.xsd and of validate.py, plus hexgen's own (kMaxRoadCost, distinct printed
  // ids); throws std::invalid_argument beginning with `where`.
  void validate(const Settings&, std::string_view where);

  // A settings document in the hexgen.xsd language; reading it back gives the same Settings.
  std::string writeSettings(const Settings&);
  void writeSettings(const std::filesystem::path&, const Settings&);  // throws on an unwritable path

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
