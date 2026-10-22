// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexview-internal: the SVG basic shapes as path commands, a reader for the path data and point
// lists the reference renderer writes, and similarity transforms (translate, rotate, uniform scale)
// over commands. The shapes are spelled out in one canonical way (a rectangle clockwise from its
// top-left corner, a circle as two half-circle arcs from its east point), which the golden test's
// normaliser uses too.
// ----------------------------------------------
#pragma once
#include "hexview/Scene.h"
#include "hexview/SymbolLibrary.h"

#include <string_view>
#include <vector>

namespace HexView {

  using Commands = std::vector<PathCommand>;

  Commands rectPath(double x, double y, double w, double h);
  Commands roundedRectPath(double x, double y, double w, double h, double r);
  Commands circlePath(double cx, double cy, double r);
  Commands ellipsePath(double cx, double cy, double rx, double ry);
  Commands polygonPath(const std::vector<Pixel>& points);

  // SVG path data with the commands M L H V Q A Z, absolute or relative. Throws
  // std::invalid_argument naming the data for anything else.
  Commands parsePathData(std::string_view data);
  // "x,y x,y ..." as a polygon's points attribute writes them.
  std::vector<Pixel> parsePoints(std::string_view points);

  // A colour as the reference writes it in a symbol: "#rgb", "#rrggbb", or "currentColor"
  // (kCurrentColor). Throws std::invalid_argument naming the text for anything else.
  Color svgColor(std::string_view text);

  // A symbol part painted as the reference's attributes say. fill: "none", a colour, or "" for an
  // absent attribute (SVG's default, black). stroke: "none" or a colour, with SVG's default butt
  // caps and miter joins.
  Primitive symbolPart(Commands, std::string_view fill, std::string_view stroke = "none",
                       double width = 1.0, std::vector<double> dash = {});

  // p -> translate + rotate(degrees) * scale * p, the order an SVG transform list
  // "translate(..) rotate(..) scale(..)" applies.
  struct Similarity {
    Pixel translate;
    double rotateDegrees = 0.0;
    double scale = 1.0;
    Pixel apply(Pixel) const;
    Commands apply(const Commands&) const;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
