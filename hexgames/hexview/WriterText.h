// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexview-internal: the text forms the scene writers share (numbers, colours, path data, names).
// ----------------------------------------------
#pragma once
#include "hexview/Scene.h"

#include <string>
#include <string_view>

namespace HexView {

  std::string fixed(double v, int decimals);  // "%.<decimals>f", "-0.00" written "0.00"
  std::string shortest(double v);             // the shortest text that reads back as v
  // "#rrggbb", or "currentColor" for kCurrentColor. The alpha is written separately (opacity()).
  std::string colorText(const Color&);
  double alphaOf(const Color&);
  // Path data with two decimals (scene pixels) or the shortest form (symbol unit frames).
  std::string pathData(const std::vector<PathCommand>&, bool shortestP);
  std::string xmlEscape(std::string_view);
  std::string_view layerName(Layer);
  std::string_view patternName(Pattern);
  std::string_view capName(LineCap);
  std::string_view joinName(LineJoin);
  std::string_view anchorName(TextAnchor);

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
