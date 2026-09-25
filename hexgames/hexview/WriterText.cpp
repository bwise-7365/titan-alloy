// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexview/WriterText.h"

#include "hexview/SymbolLibrary.h"

#include <array>
#include <charconv>
#include <cstdio>
#include <stdexcept>

namespace HexView {

  std::string
  fixed(double v, int decimals)
  {
    std::array<char, 64> buf{};
    std::snprintf(buf.data(), buf.size(), "%.*f", decimals, v);
    std::string out(buf.data());
    if ('-' == out.front() && std::string::npos == out.find_first_not_of("-0.")) {
      out.erase(0, 1);
    }
    return out;
  }

  std::string
  shortest(double v)
  {
    std::array<char, 64> buf{};
    const auto [end, ec] = std::to_chars(buf.data(), buf.data() + buf.size(), v);
    if (std::errc{} != ec) {
      throw std::invalid_argument("shortest: number does not format");
    }
    return std::string(buf.data(), end);
  }

  std::string
  colorText(const Color& c)
  {
    if (kCurrentColor == c) {
      return "currentColor";
    }
    std::array<char, 8> buf{};
    std::snprintf(buf.data(), buf.size(), "#%02x%02x%02x", c.r, c.g, c.b);
    return std::string(buf.data());
  }

  double
  alphaOf(const Color& c)
  {
    return kCurrentColor == c ? 1.0 : c.a / 255.0;
  }

  std::string
  pathData(const std::vector<PathCommand>& commands, bool shortestP)
  {
    const auto num = [shortestP](double v) { return shortestP ? shortest(v) : fixed(v, 2); };
    const auto pt = [&](Pixel p) { return num(p.x) + "," + num(p.y); };
    std::string out;
    for (const PathCommand& c : commands) {
      if (!out.empty()) {
        out += ' ';
      }
      if (const auto* m = std::get_if<MoveTo>(&c)) {
        out += "M" + pt(m->to);
      }
      else if (const auto* l = std::get_if<LineTo>(&c)) {
        out += "L" + pt(l->to);
      }
      else if (const auto* q = std::get_if<QuadTo>(&c)) {
        out += "Q" + pt(q->control) + " " + pt(q->to);
      }
      else if (const auto* a = std::get_if<ArcTo>(&c)) {
        out += "A" + num(a->rx) + "," + num(a->ry) + " " + num(a->rotationDegrees) + " " +
               (a->largeArcP ? "1" : "0") + " " + (a->sweepP ? "1" : "0") + " " + pt(a->to);
      }
      else {
        out += "Z";
      }
    }
    return out;
  }

  std::string
  xmlEscape(std::string_view text)
  {
    std::string out;
    for (const char c : text) {
      switch (c) {
      case '&':
        out += "&amp;";
        break;
      case '<':
        out += "&lt;";
        break;
      case '>':
        out += "&gt;";
        break;
      case '"':
        out += "&quot;";
        break;
      default:
        out += c;
        break;
      }
    }
    return out;
  }

  std::string_view
  layerName(Layer layer)
  {
    switch (layer) {
    case Layer::Background:
      return "background";
    case Layer::Terrain:
      return "terrain";
    case Layer::Regions:
      return "regions";
    case Layer::Grid:
      return "grid";
    case Layer::Edges:
      return "edges";
    case Layer::Links:
      return "links";
    case Layer::Rings:
      return "rings";
    case Layer::HexGlyphs:
      return "hexglyphs";
    case Layer::SideGlyphs:
      return "sideglyphs";
    case Layer::Labels:
      return "labels";
    case Layer::Panels:
      return "panels";
    case Layer::Control:
      return "control";
    case Layer::Units:
      return "units";
    case Layer::Markers:
      return "markers";
    case Layer::Highlights:
      return "highlights";
    case Layer::Overlay:
      return "overlay";
    }
    throw std::invalid_argument("layerName: layer out of range");
  }

  std::string_view
  patternName(Pattern p)
  {
    switch (p) {
    case Pattern::Dots:
      return "dots";
    case Pattern::Hatch:
      return "hatch";
    case Pattern::Mottle:
      return "mottle";
    case Pattern::Palms:
      return "palms";
    }
    throw std::invalid_argument("patternName: pattern out of range");
  }

  std::string_view
  capName(LineCap cap)
  {
    switch (cap) {
    case LineCap::Butt:
      return "butt";
    case LineCap::Round:
      return "round";
    case LineCap::Square:
      return "square";
    }
    throw std::invalid_argument("capName: cap out of range");
  }

  std::string_view
  joinName(LineJoin join)
  {
    switch (join) {
    case LineJoin::Miter:
      return "miter";
    case LineJoin::Round:
      return "round";
    case LineJoin::Bevel:
      return "bevel";
    }
    throw std::invalid_argument("joinName: join out of range");
  }

  std::string_view
  anchorName(TextAnchor anchor)
  {
    switch (anchor) {
    case TextAnchor::Start:
      return "start";
    case TextAnchor::Middle:
      return "middle";
    case TextAnchor::End:
      return "end";
    }
    throw std::invalid_argument("anchorName: anchor out of range");
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
