// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Builds a Board from a hexsheet document, a loaded RuleSet, and a package's bindings between them.
// The header only forward-declares HexRules::RuleSet (hexmodel must not depend on hexrules, which
// depends on hexmodel): the .cpp that defines BoardBuilder::build is compiled into the hexrules
// static library, which already links hexmodel and can see both id spaces. See hexrules/CMakeLists.txt.
// ----------------------------------------------
#pragma once
#include "hexmodel/Board.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/SheetDoc.h"

#include <string>
#include <vector>

namespace HexRules {
  class RuleSet;
}

namespace HexModel {

  class BoardBuilder {
  public:
    // Throws std::invalid_argument naming file:line and the offending id when a sheet terrain,
    // hexside line, network kind or hexside reference does not resolve.
    static Board build(const HexXml::SheetDoc&, const HexRules::RuleSet&, const HexXml::PackageDoc&);

  private:
    BoardBuilder() = delete;

    // Records `terrain` on the hexside token "HEX:DIR" and on the matching direction of its
    // neighbour, if any; needs friend access to Board's private edge storage, so it is a member
    // here rather than a free function.
    static void recordEdge(Board&, const std::vector<HexCoord::Orientation>& orientationOf,
                            const std::string& token, EdgeTerrainId terrain);
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
