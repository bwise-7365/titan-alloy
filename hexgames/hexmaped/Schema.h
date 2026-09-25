// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#pragma once
// Schema -- the closed vocabularies of map_graphics/xml/hexsheet.xsd, as the editor offers them. The
// editor never writes a value outside these lists or an IDREF that the sheet does not declare, so
// every document it saves is one the schema accepts. (The names end in List because Qt defines
// `slots` as a macro.) (Ben, 2026-09-21: the editor allows only the
// structures the XSD defines). hexmaped_schema_test keeps these lists equal to the XSD's.

#include <span>
#include <string>
#include <string_view>

namespace HexMapEd::Schema {

  std::span<const std::string_view> symbolList();   // Symbol
  std::span<const std::string_view> slotList();     // Slot
  std::span<const std::string_view> dirList();      // Dir
  std::span<const std::string_view> patternList();  // Pattern
  std::span<const std::string_view> weightList();   // label weight
  std::span<const std::string_view> anchorList();   // label anchor
  std::span<const std::string_view> endReasonList();  // EndReason

  bool symbolP(std::string_view);
  bool slotP(std::string_view);
  bool patternP(std::string_view);

  // Throw std::invalid_argument naming the offending value.
  void requireSymbol(std::string_view value, std::string_view where);
  void requireSlot(std::string_view value, std::string_view where);
  void requireHexId(std::string_view value, std::string_view where);  // HexRef: [A-Za-z0-9]+
  void requireColourValue(std::string_view value, std::string_view where);  // RGB: #rrggbb or none

}  // namespace HexMapEd::Schema
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
