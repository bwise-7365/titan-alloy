// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] A Scene as files. writeSvg lays the document out the way hexsheet2svg.py
// does (one <g class="layer ..."> per layer, symbols and patterns in <defs>, numbers to two decimals),
// so hexview's SVG goldens compare against the Python reference renderers' output. writeJson is the
// HTML client's input: the same layers and primitives with their hit tags, plus the symbols and
// patterns they use, so the browser draws what hexqt draws.
// ----------------------------------------------
#pragma once
#include "hexview/Scene.h"
#include "hexview/SymbolLibrary.h"

#include <string>
#include <string_view>

namespace HexView {

  std::string writeSvg(const Scene&, const SymbolLibrary&, std::string_view title);
  std::string writeJson(const Scene&, const SymbolLibrary&);

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
