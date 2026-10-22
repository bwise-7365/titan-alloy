// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Legend marks, hexsheet2svg.py mark_svg: a shape mark is its MARK_SHAPES body inside
// <g transform="translate(x,y) rotate(rot) scale(size)" style="color:...">. The Scene has no
// groups, so the body is placed here, through the transform exactly as the reference prints it
// (translate to two decimals, rotation to one, scale to two); the body itself carries the
// reference's three-decimal rounding (MarkShapes.cpp). Stroke widths and dashes scale with the
// body.
// ----------------------------------------------
#include "hexview/MapLayers.h"
#include "hexview/MarkShapes.h"

#include <cmath>
#include <sstream>
#include <stdexcept>

namespace HexView {

  namespace {

    double
    rounded(double v, double unit)
    {
      return std::round(v / unit) * unit;
    }

    // The mark's size "w h" in hex units; the reference's default is 0.3 x 0.3.
    std::pair<double, double>
    markSize(const HexXml::SheetMarkDoc& m)
    {
      std::istringstream in(m.size.value_or("0.3 0.3"));
      double w = 0.0;
      double h = 0.0;
      if (!(in >> w >> h)) {
        throw std::invalid_argument("mark '" + m.id + "': size '" + m.size.value_or("") +
                                    "' is not \"w h\"");
      }
      return {w, h};
    }

    Color
    tinted(const Color& c, const Color& current)
    {
      return kCurrentColor == c ? current : c;
    }

    Primitive
    placed(const Primitive& part, const Similarity& place, Color color, const HitTag& hit)
    {
      PathShape s = std::get<PathShape>(part.shape);
      s.commands = place.apply(s.commands);
      if (s.fill.has_value()) {
        s.fill->color = tinted(s.fill->color, color);
      }
      if (s.stroke.has_value()) {
        s.stroke->color = tinted(s.stroke->color, color);
        s.stroke->width *= place.scale;
        for (double& d : s.stroke->dash) {
          d *= place.scale;
        }
      }
      return Primitive{s, hit};
    }

  }  // namespace

  std::vector<Primitive>
  markPrimitives(const MapContext& ctx, const HexXml::SheetMarkDoc& m, Pixel at, double rot,
                 double size, Color color, HitTag hit)
  {
    const auto [w, h] = markSize(m);
    if ("pictogram" == m.shape) {
      const std::string symbol = m.pictogram.value_or("dot");
      if (!ctx.symbols.containsP(symbol)) {
        throw std::invalid_argument("mark '" + m.id + "': pictogram '" + symbol +
                                    "' is not in the symbol library");
      }
      return {Primitive{SymbolUse{symbol, at, rot, size, color}, hit}};
    }
    if ("text" == m.shape) {
      Font f = ctx.font(size * h);
      f.weight = FontWeight::Bold;
      return {Primitive{
          textShape(at, m.name.value_or(m.id), f, color, TextAnchor::Middle, TextBaseline::Central),
          hit}};
    }
    const Similarity place{Pixel{rounded(at.x, 0.01), rounded(at.y, 0.01)}, rounded(rot, 0.1),
                           rounded(size, 0.01)};
    std::vector<Primitive> out;
    for (const Primitive& part : markBody(m.shape, w, h)) {
      out.push_back(placed(part, place, color, hit));
    }
    return out;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
