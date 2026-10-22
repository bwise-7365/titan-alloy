// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexsheet2svg.py's SYMBOLS, transcribed part for part: the path data and point lists are the
// reference's own strings, read by Shapes.cpp; rect, circle and polygon become paths in the
// canonical spelling Shapes.h describes. Where the reference leaves an attribute out, SVG's default
// is written here (a path with no fill is filled black, a stroke has butt caps and miter joins).
// The legend-mark shapes are included as "mark-<shape>" at the reference's default mark size, 0.3 x
// 0.3. counters2svg.py's face symbols are not here yet (task 17 covers the map half only).
// ----------------------------------------------
#include "hexview/SymbolLibrary.h"

#include "hexview/MarkShapes.h"
#include "hexview/Shapes.h"

#include <stdexcept>

namespace HexView {

  namespace {

    using Parts = std::vector<Primitive>;

    Primitive
    rect(double x, double y, double w, double h, std::string_view fill,
         std::string_view stroke = "none", double width = 1.0)
    {
      return symbolPart(rectPath(x, y, w, h), fill, stroke, width);
    }

    Primitive
    circle(double cx, double cy, double r, std::string_view fill, std::string_view stroke = "none",
           double width = 1.0)
    {
      return symbolPart(circlePath(cx, cy, r), fill, stroke, width);
    }

    Primitive
    polygon(std::string_view points, std::string_view fill, std::string_view stroke = "none",
            double width = 1.0)
    {
      return symbolPart(polygonPath(parsePoints(points)), fill, stroke, width);
    }

    Primitive
    path(std::string_view data, std::string_view fill, std::string_view stroke = "none",
         double width = 1.0)
    {
      return symbolPart(parsePathData(data), fill, stroke, width);
    }

    constexpr std::string_view kCur = "currentColor";
    constexpr std::string_view kAnchor =
        "M0,-0.14 L0,0.12 M-0.12,0 L0.12,0 M-0.13,0.04 A0.13,0.13 0 0 0 0.13,0.04";
    constexpr std::string_view kLeftHead = "-0.55,0 -0.35,-0.14 -0.35,0.14";
    constexpr std::string_view kRightHead = "0.55,0 0.35,-0.14 0.35,0.14";

    Parts
    arrivalBox()
    {
      // <rect x="-0.25" y="-0.25" width="0.5" height="0.5" transform="rotate(45)" .../>
      const Similarity turn{Pixel{}, 45.0, 1.0};
      return {symbolPart(turn.apply(rectPath(-0.25, -0.25, 0.5, 0.5)), kCur, "#fff", 0.03),
              path("M0,-0.35 L0,-0.85 M-0.15,-0.7 L0,-0.85 L0.15,-0.7", "none", kCur, 0.09)};
    }

    std::map<std::string, Parts>
    hexSymbols()
    {
      std::map<std::string, Parts> s;
      s["position-badge"] = {polygon(
          "0.42,0 0.21,0.364 -0.21,0.364 -0.42,0 -0.21,-0.364 0.21,-0.364", kCur, "#fff", 0.04)};
      s["fire-intense"] = {path("M-0.2,0 A0.2,0.2 0 0 1 0.2,0 Z", kCur, "#fff", 0.02)};
      s["fire-steady"] = {path("M-0.2,0 A0.2,0.2 0 0 1 0.2,0 Z", kCur, "#fff", 0.02),
                          rect(-0.045, -0.2, 0.09, 0.12, "#fff")};
      s["fire-square"] = {rect(-0.09, -0.09, 0.18, 0.18, kCur, "#fff", 0.02)};
      s["lvt-wreck"] = {polygon("0,-0.3 0.3,0 0,0.3 -0.3,0", "none", kCur, 0.07)};
      s["arrival-box"] = arrivalBox();
      s["artillery"] = {rect(-0.08, -0.22, 0.36, 0.24, "#fff", "#444", 0.02),
                        circle(0.1, -0.1, 0.08, "#d22"),
                        path("M-0.08,-0.22 L-0.08,0.25", "", "#444", 0.03)};
      s["tank"] = {symbolPart(roundedRectPath(-0.3, -0.08, 0.6, 0.22, 0.08), kCur),
                   rect(-0.12, -0.2, 0.24, 0.14, kCur),
                   path("M0.12,-0.13 L0.36,-0.13", "", kCur, 0.05)};
      s["pier-head"] = {circle(0, 0, 0.26, kCur, "#fff", 0.04)};
      s["city-major"] = {rect(-0.26, -0.26, 0.52, 0.52, kCur, "#e8e8e8", 0.06)};
      s["city-minor"] = {circle(0, 0, 0.22, "#fff", "#555", 0.04)};
      s["city"] = {circle(0, 0, 0.2, kCur, "#222", 0.04)};
      s["capital"] = {polygon("0,-0.36 0.11,-0.12 0.36,-0.11 0.17,0.06 0.22,0.32 0,0.18 -0.22,0.32 "
                              "-0.17,0.06 -0.36,-0.11 -0.11,-0.12",
                              kCur, "#222", 0.04)};
      s["town"] = {rect(-0.14, -0.14, 0.28, 0.28, "#fff", "#333", 0.04)};
      s["port"] = {circle(0, 0, 0.22, kCur, "#124", 0.03), path(kAnchor, "none", "#124", 0.04),
                   circle(0, -0.14, 0.04, "none", "#124", 0.03)};
      s["port-multi"] = {circle(0, 0, 0.3, "none", kCur, 0.05),
                         circle(0, 0, 0.22, kCur, "#124", 0.03),
                         path(kAnchor, "none", "#124", 0.04)};
      s["port-key"] = {circle(0, 0, 0.22, kCur, "#124", 0.03), path(kAnchor, "none", "#124", 0.04),
                       rect(-0.3, 0.2, 0.6, 0.08, "#124")};
      s["oil"] = {path("M-0.16,0.3 L-0.05,-0.3 L0.05,-0.3 L0.16,0.3 Z M-0.12,0.1 L0.12,0.1 "
                       "M-0.09,-0.1 L0.09,-0.1",
                       "none", kCur, 0.04)};
      s["range-dot"] = {circle(0, 0, 0.06, "#fff")};
      s["stacking"] = {rect(-0.17, -0.11, 0.34, 0.22, "#fff", "#222", 0.03),
                       circle(-0.09, 0, 0.035, "#222"), circle(0, 0, 0.035, "#222"),
                       circle(0.09, 0, 0.035, "#222")};
      s["star"] = {polygon("0,-0.22 0.065,-0.07 0.22,-0.07 0.1,0.03 0.13,0.2 0,0.11 -0.13,0.2 "
                           "-0.1,0.03 -0.22,-0.07 -0.065,-0.07",
                           "#fff", "#222", 0.03)};
      s["aid"] = {symbolPart(roundedRectPath(-0.13, -0.13, 0.26, 0.26, 0.04), kCur, "#fff", 0.03),
                  path("M0,-0.08 L0,0.08 M-0.08,0 L0.08,0", "", "#fff", 0.05)};
      s["ice"] = {path("M0,-0.25 L0,0.25 M-0.22,-0.125 L0.22,0.125 M-0.22,0.125 L0.22,-0.125", "",
                       kCur, 0.04)};
      s["strait"] = {path("M-0.42,0 L0.42,0", "", kCur, 0.09), polygon(kLeftHead, kCur),
                     polygon(kRightHead, kCur)};
      s["strait-broken"] = {
          path("M-0.42,0 L-0.2,-0.1 L-0.05,0.1 L0.1,-0.1 L0.42,0", "none", kCur, 0.09),
          polygon(kLeftHead, kCur), polygon(kRightHead, kCur)};
      s["arrow"] = {path("M0,0.1 L0,-0.7 M-0.16,-0.52 L0,-0.7 L0.16,-0.52", "none", kCur, 0.09)};
      s["junction"] = {rect(-0.09, -0.09, 0.18, 0.18, "#fff", "#222", 0.03)};
      s["dot"] = {circle(0, 0, 0.1, kCur)};
      s["text"] = {};
      return s;
    }

  }  // namespace

  SymbolLibrary
  SymbolLibrary::reference()
  {
    SymbolLibrary lib;
    for (auto& [name, parts] : hexSymbols()) {
      lib.symbols_.emplace(name, Symbol{SymbolFrame::Hex, std::move(parts)});
    }
    for (const std::string& shape : markShapeNames()) {
      lib.symbols_.emplace("mark-" + shape, Symbol{SymbolFrame::Mark, markBody(shape, 0.3, 0.3)});
    }
    return lib;
  }

  bool
  SymbolLibrary::containsP(const std::string& name) const
  {
    return symbols_.contains(name);
  }

  const Symbol&
  SymbolLibrary::symbol(const std::string& name) const
  {
    const auto it = symbols_.find(name);
    if (symbols_.end() == it) {
      throw std::invalid_argument("symbol '" + name + "' is not in the symbol library");
    }
    return it->second;
  }

  std::vector<std::string>
  SymbolLibrary::names() const
  {
    std::vector<std::string> out;
    for (const auto& [name, symbol] : symbols_) {
      out.push_back(name);
    }
    return out;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
