// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// writeSvg: hexsheet2svg.py's document shape (symbols and the four terrain patterns in <defs>, one
// <g class="layer NAME"> per layer, scene numbers to two decimals, glyph rotations to one). Every
// primitive becomes one element, except a path with a pattern, which is written as the reference
// writes a patterned hex: the filled path, then the same path filled with the pattern.
// ----------------------------------------------
#include "hexview/SceneWriters.h"

#include "hexview/WriterText.h"

#include <stdexcept>

namespace HexView {

  namespace {

    std::string
    strokeAttrs(const Stroke& s, bool shortestP)
    {
      const auto num = [shortestP](double v) { return shortestP ? shortest(v) : fixed(v, 2); };
      std::string a = " stroke=\"" + colorText(s.color) + "\" stroke-width=\"" + num(s.width) +
                      "\" stroke-linecap=\"" + std::string(capName(s.cap)) +
                      "\" stroke-linejoin=\"" + std::string(joinName(s.join)) + "\"";
      const double opacity = s.opacity * alphaOf(s.color);
      if (1.0 != opacity) {
        a += " stroke-opacity=\"" + shortest(opacity) + "\"";
      }
      if (!s.dash.empty()) {
        std::string d;
        for (const double v : s.dash) {
          d += (d.empty() ? "" : " ") + num(v);
        }
        a += " stroke-dasharray=\"" + d + "\"";
      }
      return a;
    }

    std::string
    fillAttrs(const std::optional<Fill>& f)
    {
      if (!f.has_value()) {
        return " fill=\"none\"";
      }
      std::string a = " fill=\"" + colorText(f->color) + "\"";
      const double opacity = f->opacity * alphaOf(f->color);
      if (1.0 != opacity) {
        a += " fill-opacity=\"" + shortest(opacity) + "\"";
      }
      return a;
    }

    std::string
    pathElements(const PathShape& s, bool shortestP)
    {
      const std::string d = pathData(s.commands, shortestP);
      std::string out = "<path d=\"" + d + "\"" + fillAttrs(s.fill) +
                        (s.stroke.has_value() ? strokeAttrs(*s.stroke, shortestP) : "") + "/>";
      if (s.pattern.has_value()) {
        out += "\n<path d=\"" + d + "\" fill=\"url(#pat-" + std::string(patternName(*s.pattern)) +
               ")\"/>";
      }
      return out;
    }

    std::string
    baselineAttr(TextBaseline b)
    {
      switch (b) {
      case TextBaseline::Alphabetic:
        return "";
      case TextBaseline::Central:
        return " dominant-baseline=\"central\"";
      case TextBaseline::Middle:
        return " dominant-baseline=\"middle\"";
      }
      throw std::invalid_argument("baselineAttr: baseline out of range");
    }

    std::string
    textElement(const TextShape& t)
    {
      std::string a = 0.0 == t.angleDegrees
                          ? "<text x=\"" + fixed(t.at.x, 2) + "\" y=\"" + fixed(t.at.y, 2) + "\""
                          : "<text transform=\"translate(" + fixed(t.at.x, 2) + "," +
                                fixed(t.at.y, 2) + ") rotate(" + shortest(t.angleDegrees) + ")\"";
      a += " font-family=\"" + xmlEscape(t.font.family) + "\" font-size=\"" +
           fixed(t.font.size, 2) + "\"";
      if (FontWeight::Bold == t.font.weight) {
        a += " font-weight=\"bold\"";
      }
      if (t.font.italicP) {
        a += " font-style=\"italic\"";
      }
      if (0.0 != t.font.letterSpacing) {
        a += " letter-spacing=\"" + shortest(t.font.letterSpacing) + "\"";
      }
      a += " text-anchor=\"" + std::string(anchorName(t.anchor)) + "\"" + baselineAttr(t.baseline);
      a += " fill=\"" + colorText(t.color) + "\"";
      if (1.0 != alphaOf(t.color)) {
        a += " fill-opacity=\"" + fixed(alphaOf(t.color), 3) + "\"";
      }
      if (t.halo.has_value()) {
        a += strokeAttrs(*t.halo, false) + " paint-order=\"stroke\"";
      }
      return a + ">" + xmlEscape(t.text) + "</text>";
    }

    std::string
    useElement(const SymbolUse& u, const SymbolLibrary& lib)
    {
      if (!lib.containsP(u.symbol)) {
        throw std::invalid_argument("writeSvg: symbol '" + u.symbol + "' is not in the library");
      }
      return "<use xlink:href=\"#sym-" + xmlEscape(u.symbol) + "\" transform=\"translate(" +
             fixed(u.at.x, 2) + "," + fixed(u.at.y, 2) + ") rotate(" + fixed(u.rotateDegrees, 1) +
             ") scale(" + fixed(u.scale, 2) + ")\" style=\"color:" + colorText(u.color) + "\"/>";
    }

    std::string
    element(const Shape& shape, const SymbolLibrary& lib, bool shortestP)
    {
      if (const auto* p = std::get_if<PathShape>(&shape)) {
        return pathElements(*p, shortestP);
      }
      if (const auto* t = std::get_if<TextShape>(&shape)) {
        return textElement(*t);
      }
      return useElement(std::get<SymbolUse>(shape), lib);
    }

    // The reference's pattern tiles, verbatim.
    constexpr std::string_view kPatterns =
        "<pattern id=\"pat-dots\" width=\"6\" height=\"6\" patternUnits=\"userSpaceOnUse\"><circle "
        "cx=\"3\" cy=\"3\" r=\"0.9\" fill=\"#000\" fill-opacity=\"0.25\"/></pattern>\n"
        "<pattern id=\"pat-hatch\" width=\"6\" height=\"6\" patternUnits=\"userSpaceOnUse\"><path "
        "d=\"M0,6 L6,0\" stroke=\"#000\" stroke-opacity=\"0.2\" stroke-width=\"1\"/></pattern>\n"
        "<pattern id=\"pat-mottle\" width=\"9\" height=\"9\" "
        "patternUnits=\"userSpaceOnUse\"><circle cx=\"2\" cy=\"3\" r=\"1.6\" fill=\"#000\" "
        "fill-opacity=\"0.12\"/><circle cx=\"6.5\" cy=\"7\" r=\"1.2\" fill=\"#000\" "
        "fill-opacity=\"0.12\"/></pattern>\n"
        "<pattern id=\"pat-palms\" width=\"10\" height=\"10\" "
        "patternUnits=\"userSpaceOnUse\"><path d=\"M5,8 L5,4 M5,4 L2,2 M5,4 L8,2 M5,4 L3,6 M5,4 "
        "L7,6\" stroke=\"#2a7a2a\" stroke-opacity=\"0.6\" stroke-width=\"0.9\" "
        "fill=\"none\"/></pattern>\n";

    std::string
    defs(const SymbolLibrary& lib)
    {
      std::string out = "<defs>\n";
      for (const std::string& name : lib.names()) {
        const Symbol& s = lib.symbol(name);
        if (SymbolFrame::Hex != s.frame) {
          continue;
        }
        out += "<symbol id=\"sym-" + xmlEscape(name) + "\" overflow=\"visible\">";
        for (const Primitive& p : s.body) {
          out += element(p.shape, lib, true);
        }
        out += "</symbol>\n";
      }
      return out + std::string(kPatterns) + "</defs>\n";
    }

  }  // namespace

  std::string
  writeSvg(const Scene& scene, const SymbolLibrary& lib, std::string_view title)
  {
    const std::string w = shortest(scene.width());
    const std::string h = shortest(scene.height());
    std::string out = "<svg xmlns=\"http://www.w3.org/2000/svg\" "
                      "xmlns:xlink=\"http://www.w3.org/1999/xlink\" width=\"" +
                      w + "\" height=\"" + h + "\" viewBox=\"0 0 " + w + " " + h + "\">\n";
    out += "<title>" + xmlEscape(title) + "</title>\n";
    out += defs(lib);
    for (std::size_t k = 0; k < kLayerCount; ++k) {
      const Layer layer = static_cast<Layer>(k);
      out += "<g class=\"layer " + std::string(layerName(layer)) + "\">\n";
      for (const Primitive& p : scene.layer(layer)) {
        out += element(p.shape, lib, false) + "\n";
      }
      out += "</g>\n";
    }
    return out + "</svg>\n";
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
