// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmaped/Schema.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <stdexcept>

namespace HexMapEd::Schema {

  namespace {

    constexpr std::array<std::string_view, 29> kSymbols{
      "position-badge", "fire-intense", "fire-steady", "fire-square", "lvt-wreck", "arrival-box",
      "artillery",      "tank",         "pier-head",   "city-major",  "city-minor", "city",
      "capital",        "town",         "port",        "port-multi",  "port-key",   "oil",
      "range-dot",      "stacking",     "star",        "aid",         "ice",        "strait",
      "strait-broken",  "arrow",        "junction",    "dot",        "text"};
    constexpr std::array<std::string_view, 9> kSlots{"c", "n", "ne", "e", "se", "s", "sw", "w", "nw"};
    constexpr std::array<std::string_view, 8> kDirs{"n", "ne", "e", "se", "s", "sw", "w", "nw"};
    constexpr std::array<std::string_view, 5> kPatterns{"none", "dots", "hatch", "mottle", "palms"};
    constexpr std::array<std::string_view, 2> kWeights{"normal", "bold"};
    constexpr std::array<std::string_view, 3> kAnchors{"start", "middle", "end"};
    constexpr std::array<std::string_view, 7> kEndReasons{"edge",  "place", "junction", "dot",
                                                          "bank",  "sea",   "unexplained"};

    bool
    inP(std::span<const std::string_view> list, std::string_view value)
    {
      return std::find(list.begin(), list.end(), value) != list.end();
    }

  }  // namespace

  std::span<const std::string_view> symbolList() { return kSymbols; }
  std::span<const std::string_view> slotList() { return kSlots; }
  std::span<const std::string_view> dirList() { return kDirs; }
  std::span<const std::string_view> patternList() { return kPatterns; }
  std::span<const std::string_view> weightList() { return kWeights; }
  std::span<const std::string_view> anchorList() { return kAnchors; }
  std::span<const std::string_view> endReasonList() { return kEndReasons; }

  bool symbolP(std::string_view v) { return inP(kSymbols, v); }
  bool slotP(std::string_view v) { return inP(kSlots, v); }
  bool patternP(std::string_view v) { return inP(kPatterns, v); }

  void
  requireSymbol(std::string_view value, std::string_view where)
  {
    if (!symbolP(value)) {
      throw std::invalid_argument(std::string(where) + ": '" + std::string(value) + "' is not a Symbol of hexsheet.xsd");
    }
    return;
  }

  void
  requireSlot(std::string_view value, std::string_view where)
  {
    if (!slotP(value)) {
      throw std::invalid_argument(std::string(where) + ": '" + std::string(value) + "' is not a Slot of hexsheet.xsd");
    }
    return;
  }

  void
  requireHexId(std::string_view value, std::string_view where)
  {
    const bool okP = !value.empty() && std::all_of(value.begin(), value.end(), [](unsigned char c) {
      return 0 != std::isalnum(c);
    });
    if (!okP) {
      throw std::invalid_argument(std::string(where) + ": '" + std::string(value) + "' is not a HexRef ([A-Za-z0-9]+)");
    }
    return;
  }

  void
  requireColourValue(std::string_view value, std::string_view where)
  {
    if ("none" == value) {
      return;
    }
    const bool okP = 7 == value.size() && '#' == value.front() &&
                     std::all_of(value.begin() + 1, value.end(), [](unsigned char c) { return 0 != std::isxdigit(c); });
    if (!okP) {
      throw std::invalid_argument(std::string(where) + ": '" + std::string(value) + "' is not an RGB value (#rrggbb or none)");
    }
    return;
  }

}  // namespace HexMapEd::Schema
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
