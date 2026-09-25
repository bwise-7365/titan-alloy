// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#pragma once
// SheetWriter -- a SheetDoc back to hexsheet XML: the schema's element order (grid, palette,
// terrains, lines, then the map content), one fixed attribute order per element, attributes at their
// schema default omitted, LF line ends, two-space indent. What SheetDoc::parse read, this writes, so
// load-write-load is the identity on the document model; the saved file carries the structure and
// the style declarations the renderers need, and validates against hexsheet.xsd.

#include "hexxml/SheetDoc.h"

#include <filesystem>
#include <string>
#include <string_view>

namespace HexMapEd {

  // schemaLocation is what xsi:noNamespaceSchemaLocation says: hexsheet.xsd relative to the file.
  std::string writeSheet(const HexXml::SheetDoc&, std::string_view schemaLocation = "hexsheet.xsd");
  void writeSheet(const std::filesystem::path&, const HexXml::SheetDoc&,
                  std::string_view schemaLocation = "hexsheet.xsd");  // throws on an unwritable path

}  // namespace HexMapEd
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
