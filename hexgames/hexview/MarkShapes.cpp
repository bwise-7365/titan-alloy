// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexview/MarkShapes.h"

#include "hexview/Shapes.h"

#include <cmath>
#include <stdexcept>
#include <string>

namespace HexView {

  namespace {

    // The reference writes every body number with "%.3f".
    double
    r3(double v)
    {
      return std::round(v * 1000.0) / 1000.0;
    }

    // hexsheet2svg.py mark_svg's dictionary v, rounded as it is printed.
    struct MarkSizes {
      double w, h, hw, hh, rw, rh, sw, sw2, dash, gap, ah, sa, nsa, sb, sc, nsc, sd, se, nse, sf;

      MarkSizes(double width, double height)
          : w(r3(width)), h(r3(height)), hw(r3(-width / 2)), hh(r3(-height / 2)), rw(r3(width / 2)),
            rh(r3(height / 2)), sw(r3(width * 0.34)), sw2(r3(width * 0.08)), dash(r3(width * 0.06)),
            gap(r3(width * 0.2)), ah(r3(-height / 2 + width * 0.5)), sa(r3(width * 0.15)),
            nsa(r3(-width * 0.15)), sb(r3(-height * 0.16)), sc(r3(width * 0.23)),
            nsc(r3(-width * 0.23)), sd(r3(height * 0.14)), se(r3(width * 0.3)),
            nse(r3(-width * 0.3)), sf(r3(height * 0.25))
      {}
    };

    constexpr std::string_view kCur = "currentColor";

    std::vector<Primitive>
    cross(const MarkSizes& v)
    {
      // M hw,0 h w M 0,hh v h
      const Commands bars{MoveTo{{v.hw, 0}}, LineTo{{v.hw + v.w, 0}}, MoveTo{{0, v.hh}},
                          LineTo{{0, v.hh + v.h}}};
      return {symbolPart(bars, "none", kCur, v.sw),
              symbolPart(bars, "none", "#fff", v.sw2, {v.dash, v.gap})};
    }

  }  // namespace

  const std::vector<std::string>&
  markShapeNames()
  {
    static const std::vector<std::string> names{"rect",     "bar",   "ellipse", "diamond",
                                                "triangle", "cross", "arrow",   "star"};
    return names;
  }

  std::vector<Primitive>
  markBody(std::string_view shape, double width, double height)
  {
    const MarkSizes v(width, height);
    if ("rect" == shape) {
      return {symbolPart(rectPath(v.hw, v.hh, v.w, v.h), kCur, "#222", 0.02)};
    }
    if ("bar" == shape) {
      return {symbolPart(rectPath(v.hw, v.hh, v.w, v.h), kCur)};
    }
    if ("ellipse" == shape) {
      return {symbolPart(ellipsePath(0, 0, v.rw, v.rh), kCur, "#222", 0.02)};
    }
    if ("diamond" == shape) {
      return {symbolPart(polygonPath({{0, v.hh}, {v.rw, 0}, {0, v.rh}, {v.hw, 0}}), kCur, "#222",
                         0.02)};
    }
    if ("triangle" == shape) {
      return {symbolPart(polygonPath({{0, v.hh}, {v.rw, v.rh}, {v.hw, v.rh}}), kCur, "#222", 0.02)};
    }
    if ("cross" == shape) {
      return cross(v);
    }
    if ("arrow" == shape) {
      const Commands arrow{MoveTo{{0, v.rh}}, LineTo{{0, v.hh}}, MoveTo{{v.hw, v.ah}},
                           LineTo{{0, v.hh}}, LineTo{{v.rw, v.ah}}};
      return {symbolPart(arrow, "none", kCur, 0.06)};
    }
    if ("star" == shape) {
      return {symbolPart(polygonPath({{0, v.hh},
                                      {v.sa, v.sb},
                                      {v.rw, v.sb},
                                      {v.sc, v.sd},
                                      {v.se, v.rh},
                                      {0, v.sf},
                                      {v.nse, v.rh},
                                      {v.nsc, v.sd},
                                      {v.hw, v.sb},
                                      {v.nsa, v.sb}}),
                         kCur, "#222", 0.02)};
    }
    throw std::invalid_argument("mark shape '" + std::string(shape) +
                                "' is not a legend-mark shape");
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
