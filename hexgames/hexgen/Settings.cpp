// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexgen/Settings.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <charconv>
#include <cmath>
#include <fstream>
#include <set>
#include <stdexcept>

namespace HexGen {

  namespace {

    constexpr std::array<Method, 2> kMethods{Method::Fractal, Method::Perlin};
    constexpr std::array<Role, kRoles> kAllRoles{Role::Low, Role::High, Role::Cover, Role::City,
                                                 Role::Rest};
    constexpr std::array<Side, 4> kSides{Side::Top, Side::Bottom, Side::Left, Side::Right};

    std::string
    at(const HexXml::XmlNode& node)
    {
      return node.file() + ":" + std::to_string(node.line()) + ": " + node.name() + ": ";
    }

    template <class E, std::size_t N>
    E
    enumOf(const HexXml::XmlNode& node, std::string_view attr, const std::array<E, N>& values,
           std::string_view (*nameOf)(E))
    {
      const std::string text = node.required(attr);
      for (const E value : values) {
        if (nameOf(value) == text) {
          return value;
        }
      }
      throw std::invalid_argument(at(node) + "'" + text + "' is not a value of " + std::string(attr));
    }

    // Only the children hexgen.xsd names may appear, so that a misspelt element is not ignored.
    void
    requireChildren(const HexXml::XmlNode& node, std::initializer_list<std::string_view> names)
    {
      for (const HexXml::XmlNode& child : node.children()) {
        if (names.end() == std::find(names.begin(), names.end(), child.name())) {
          throw std::invalid_argument(at(child) + "is not an element of " + node.name());
        }
      }
      return;
    }

    TerrainSetting
    terrainOf(const HexXml::XmlNode& node)
    {
      TerrainSetting t;
      t.id = node.required("id");
      t.name = node.optional("name").value_or(t.id);
      t.role = enumOf(node, "role", kAllRoles, roleName);
      t.percent = node.optionalAs<int>("percent").value_or(0);
      if (Role::Rest == t.role && node.optional("percent")) {
        throw std::invalid_argument(at(node) + "terrain " + t.id + " has role rest, which takes no percent");
      }
      if (Role::Rest != t.role && !node.optional("percent")) {
        throw std::invalid_argument(at(node) + "terrain " + t.id + " needs percent");
      }
      t.fill = node.required("fill");
      t.moveCost = node.optionalAs<double>("move-cost");
      t.roadCost = node.optionalAs<double>("road-cost");
      return t;
    }

    RoadSettings
    roadsOf(const HexXml::XmlNode& node)
    {
      requireChildren(node, {"side"});
      RoadSettings r;
      r.line = node.optional("line").value_or(r.line);
      r.stroke = node.optional("stroke").value_or(r.stroke);
      r.width = node.optionalAs<double>("width").value_or(r.width);
      for (const HexXml::XmlNode& side : node.children("side")) {
        SideSetting s;
        s.at = enumOf(side, "at", kSides, sideName);
        s.roads = side.requiredAs<int>("roads");
        s.cities = side.optionalAs<int>("cities").value_or(0);
        r.sides.push_back(s);
      }
      return r;
    }

    std::string
    num(double v)
    {
      std::array<char, 32> buf{};
      const auto [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), v);
      if (std::errc{} != ec) {
        throw std::invalid_argument("writeSettings: cannot format a number");
      }
      return std::string(buf.data(), end);
    }

    std::string
    escaped(std::string_view text)
    {
      std::string out;
      for (const char c : text) {
        if ('&' == c) {
          out += "&amp;";
        }
        else if ('<' == c) {
          out += "&lt;";
        }
        else if ('>' == c) {
          out += "&gt;";
        }
        else if ('"' == c) {
          out += "&quot;";
        }
        else {
          out += c;
        }
      }
      return out;
    }

    std::string
    attr(std::string_view name, std::string_view value)
    {
      return " " + std::string(name) + "=\"" + escaped(value) + "\"";
    }

    bool
    rgbP(std::string_view text)
    {
      return 7 == text.size() && '#' == text[0] &&
             std::all_of(text.begin() + 1, text.end(), [](char c) { return 0 != std::isxdigit(static_cast<unsigned char>(c)); });
    }

    // Road costs are summed in hundredths of a step (CostGraph), so they must be whole hundredths.
    bool
    wholeHundredthsP(double v)
    {
      return std::abs(v * 100.0 - std::round(v * 100.0)) < 1e-9;
    }

  }  // namespace

  std::string_view
  methodName(Method m)
  {
    switch (m) {
      case Method::Fractal:
        return "fractal";
      case Method::Perlin:
        return "perlin";
    }
    throw std::invalid_argument("methodName: not a Method");
  }

  std::string_view
  roleName(Role r)
  {
    switch (r) {
      case Role::Low:
        return "low";
      case Role::High:
        return "high";
      case Role::Cover:
        return "cover";
      case Role::City:
        return "city";
      case Role::Rest:
        return "rest";
    }
    throw std::invalid_argument("roleName: not a Role");
  }

  std::string_view
  sideName(Side s)
  {
    switch (s) {
      case Side::Top:
        return "top";
      case Side::Bottom:
        return "bottom";
      case Side::Left:
        return "left";
      case Side::Right:
        return "right";
    }
    throw std::invalid_argument("sideName: not a Side");
  }

  Settings
  Settings::load(const std::filesystem::path& path)
  {
    return parse(HexXml::XmlDocument::load(path));
  }

  Settings
  Settings::parse(const HexXml::XmlDocument& doc)
  {
    const HexXml::XmlNode root = doc.root();
    if ("generator" != root.name()) {
      throw std::invalid_argument(at(root) + "the root element is not generator");
    }
    requireChildren(root, {"grid", "height", "terrain", "roads", "altitude"});
    Settings s;
    s.id = root.required("id");
    s.title = root.required("title");
    s.seed = root.requiredAs<std::int64_t>("seed");

    const std::optional<HexXml::XmlNode> grid = root.child("grid");
    if (!grid) {
      throw std::invalid_argument(at(root) + "has no grid");
    }
    const std::string orientation = grid->optional("orientation").value_or("flat");
    if ("flat" != orientation && "pointy" != orientation) {
      throw std::invalid_argument(at(*grid) + "orientation '" + orientation + "' is neither flat nor pointy");
    }
    s.orientation = "flat" == orientation ? HexCoord::Orientation::Flat : HexCoord::Orientation::Pointy;
    const std::string offset = grid->optional("offset").value_or("odd");
    if ("odd" != offset && "even" != offset) {
      throw std::invalid_argument(at(*grid) + "offset '" + offset + "' is neither odd nor even");
    }
    s.offset = "odd" == offset ? HexCoord::Parity::Odd : HexCoord::Parity::Even;
    s.cols = grid->requiredAs<int>("cols");
    s.rows = grid->requiredAs<int>("rows");
    s.size = grid->optionalAs<double>("size").value_or(s.size);
    s.idFormat = grid->optional("id-format");

    const std::optional<HexXml::XmlNode> height = root.child("height");
    if (!height) {
      throw std::invalid_argument(at(root) + "has no height");
    }
    s.method = enumOf(*height, "method", kMethods, methodName);
    s.roughness = height->optionalAs<double>("roughness").value_or(s.roughness);

    for (const HexXml::XmlNode& t : root.children("terrain")) {
      s.terrains.push_back(terrainOf(t));
    }
    if (const std::optional<HexXml::XmlNode> roads = root.child("roads")) {
      s.roads = roadsOf(*roads);
    }
    if (const std::optional<HexXml::XmlNode> alt = root.child("altitude")) {
      AltitudeSettings a;
      a.maxLand = alt->optionalAs<double>("max-land").value_or(a.maxLand);
      a.maxDepth = alt->optionalAs<double>("max-depth").value_or(a.maxDepth);
      s.altitude = a;
    }
    validate(s, root.file());
    return s;
  }

  Settings
  Settings::hexkrieg()
  {
    Settings s;
    s.id = "hexkrieg-map";
    s.title = "HexKrieg map, 18 x 30";
    s.seed = 60834130;
    s.cols = 30;
    s.rows = 18;
    s.method = Method::Perlin;
    s.roughness = 0.5;
    s.terrains = {
        {"water", "WATER", Role::Low, 10, "#69B9ED", 100.0, std::nullopt},
        {"mountain", "MOUNTAIN", Role::High, 20, "#B79F31", 4.0, 2.0},
        {"forest", "FOREST", Role::Cover, 25, "#6A9E8D", 2.0, 1.25},
        {"urban", "URBAN", Role::City, 3, "#DDDDDD", 1.0, 1.0},
        {"open", "OPEN", Role::Rest, 0, "#EEE5C1", 1.0, 1.0},
    };
    RoadSettings roads;
    roads.sides = {{Side::Left, 2, 2}, {Side::Right, 2, 2}};
    s.roads = roads;
    return s;
  }

  std::string
  Settings::idFormatOrDefault() const
  {
    if (idFormat) {
      return *idFormat;
    }
    return 100 <= std::max(cols, rows) ? "{col:03}{row:03}" : "{col:02}{row:02}";
  }

  std::size_t
  Settings::terrainIndex(Role role) const
  {
    for (std::size_t i = 0; i < terrains.size(); ++i) {
      if (role == terrains[i].role) {
        return i;
      }
    }
    throw std::invalid_argument("hexgen settings " + id + ": no terrain has role " + std::string(roleName(role)));
  }

  HexCoord::GridSpec
  gridSpecOf(const Settings& s)
  {
    const double half = s.size * std::sqrt(3.0) / 2.0;
    HexCoord::GridSpec g;
    g.id = "main";
    g.orientation = s.orientation;
    g.offset = s.offset;
    g.cols = s.cols;
    g.rows = s.rows;
    g.size = s.size;
    const bool flat = HexCoord::Orientation::Flat == s.orientation;
    g.ox = kMargin + (flat ? s.size : half);
    g.oy = kMargin + (flat ? half : s.size);
    g.idFormat = s.idFormatOrDefault();
    g.colStart = 0;
    g.rowStart = 0;
    return g;
  }

  void
  validate(const Settings& s, std::string_view where)
  {
    const std::string w = std::string(where) + ": ";
    const auto fail = [&](const std::string& what) { throw std::invalid_argument(w + what); };
    if (s.id.empty()) {
      fail("the generator has no id");
    }
    if (5 > s.cols || 300 < s.cols || 5 > s.rows || 300 < s.rows) {
      fail("cols and rows must each be 5 to 300, not " + std::to_string(s.cols) + " x " + std::to_string(s.rows));
    }
    if (!(0.0 < s.size)) {
      fail("grid size must be positive");
    }
    if (!(0.0 <= s.roughness)) {
      fail("roughness must not be negative");
    }
    if (kRoles != s.terrains.size()) {
      fail("there must be five terrains, one per role, not " + std::to_string(s.terrains.size()));
    }
    std::set<std::string> ids;
    std::set<Role> roles;
    int total = 0;
    for (const TerrainSetting& t : s.terrains) {
      const std::string name = "terrain " + t.id + ": ";
      if (!ids.insert(t.id).second) {
        fail(name + "the id is used twice");
      }
      if (!roles.insert(t.role).second) {
        fail(name + "role " + std::string(roleName(t.role)) + " is held by two terrains");
      }
      if (0 > t.percent || 100 < t.percent || (Role::Rest == t.role && 0 != t.percent)) {
        fail(name + "percent " + std::to_string(t.percent) + " is not allowed for role " + std::string(roleName(t.role)));
      }
      total += t.percent;
      if (!rgbP(t.fill)) {
        fail(name + "fill '" + t.fill + "' is not #rrggbb");
      }
      if (t.moveCost && !(0.0 < *t.moveCost)) {
        fail(name + "move-cost must be positive");
      }
      if (t.roadCost && !(0.0 < *t.roadCost && *t.roadCost <= kMaxRoadCost && wholeHundredthsP(*t.roadCost))) {
        fail(name + "road-cost must be a positive number of hundredths, at most 100");
      }
      if (Role::City == t.role && !t.roadCost) {
        fail(name + "has role city and needs road-cost");
      }
    }
    if (100 < total) {
      fail("terrain percentages sum to " + std::to_string(total) + ", over 100");
    }
    if (s.roads) {
      if (!rgbP(s.roads->stroke) || s.roads->line.empty() || !(0.0 < s.roads->width)) {
        fail("roads: line, stroke (#rrggbb) and a positive width are required");
      }
      std::set<Side> sides;
      for (const SideSetting& side : s.roads->sides) {
        const std::string name = "side " + std::string(sideName(side.at)) + ": ";
        if (!sides.insert(side.at).second) {
          fail(name + "given twice");
        }
        if (0 > side.roads || 0 > side.cities) {
          fail(name + "roads and cities must not be negative");
        }
        if (side.roads < side.cities) {
          fail(name + std::to_string(side.cities) + " cities but " + std::to_string(side.roads) + " roads");
        }
      }
    }
    if (s.altitude && !(0.0 < s.altitude->maxLand && 0.0 < s.altitude->maxDepth)) {
      fail("altitude: max-land and max-depth must be positive");
    }
    std::set<std::string> printed;
    try {
      const HexCoord::Grid grid(gridSpecOf(s));
      for (const HexCoord::HexId& hex : grid.ids()) {
        printed.insert(hex.text);
      }
    }
    catch (const std::invalid_argument& e) {
      fail(std::string("grid: ") + e.what());
    }
    if (static_cast<std::size_t>(s.cols) * static_cast<std::size_t>(s.rows) != printed.size()) {
      fail("id-format '" + s.idFormatOrDefault() + "' gives two hexes the same printed id");
    }
    return;
  }

  std::string
  writeSettings(const Settings& s)
  {
    std::string out = "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out += "<generator xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" "
           "xsi:noNamespaceSchemaLocation=\"hexgen.xsd\"" +
           attr("id", s.id) + attr("title", s.title) + attr("seed", std::to_string(s.seed)) + ">\n";
    out += "  <grid" +
           attr("orientation", HexCoord::Orientation::Flat == s.orientation ? "flat" : "pointy") +
           attr("offset", HexCoord::Parity::Odd == s.offset ? "odd" : "even") +
           attr("cols", std::to_string(s.cols)) + attr("rows", std::to_string(s.rows)) +
           attr("size", num(s.size)) + (s.idFormat ? attr("id-format", *s.idFormat) : "") + "/>\n";
    out += "  <height" + attr("method", methodName(s.method)) + attr("roughness", num(s.roughness)) + "/>\n";
    for (const TerrainSetting& t : s.terrains) {
      out += "  <terrain" + attr("id", t.id) + attr("name", t.name) + attr("role", roleName(t.role));
      if (Role::Rest != t.role) {
        out += attr("percent", std::to_string(t.percent));
      }
      out += attr("fill", t.fill);
      if (t.moveCost) {
        out += attr("move-cost", num(*t.moveCost));
      }
      if (t.roadCost) {
        out += attr("road-cost", num(*t.roadCost));
      }
      out += "/>\n";
    }
    if (s.roads) {
      out += "  <roads" + attr("line", s.roads->line) + attr("stroke", s.roads->stroke) +
             attr("width", num(s.roads->width)) + ">\n";
      for (const SideSetting& side : s.roads->sides) {
        out += "    <side" + attr("at", sideName(side.at)) + attr("roads", std::to_string(side.roads)) +
               attr("cities", std::to_string(side.cities)) + "/>\n";
      }
      out += "  </roads>\n";
    }
    if (s.altitude) {
      out += "  <altitude" + attr("max-land", num(s.altitude->maxLand)) +
             attr("max-depth", num(s.altitude->maxDepth)) + "/>\n";
    }
    out += "</generator>\n";
    return out;
  }

  void
  writeSettings(const std::filesystem::path& path, const Settings& s)
  {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
      throw std::invalid_argument("writeSettings: cannot write " + path.string());
    }
    file << writeSettings(s);
    if (!file) {
      throw std::invalid_argument("writeSettings: writing " + path.string() + " failed");
    }
    return;
  }

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
