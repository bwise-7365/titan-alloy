// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexview-internal: hexsheet2svg.py's MARK_SHAPES, the geometric primitives a legend mark takes, in
// the unit frame (hex circumradius 1, y down) for a mark of size w x h. The size formulas are the
// reference's, and so is its rounding: it writes every body number with three decimals.
// ----------------------------------------------
#pragma once
#include "hexview/Scene.h"

#include <string>
#include <string_view>
#include <vector>

namespace HexView {

  // The shape names, in the reference's order.
  const std::vector<std::string>& markShapeNames();

  // Throws std::invalid_argument naming the shape when it is not one of markShapeNames(). Parts
  // painted in the mark's colour carry kCurrentColor.
  std::vector<Primitive> markBody(std::string_view shape, double w, double h);

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
