// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] Paint for scene primitives: colours, fills, strokes, fonts and the four
// terrain patterns hexsheet2svg.py defines. Colours are parsed once, where a document is read; a
// primitive never carries a colour name.
// ----------------------------------------------
#pragma once
#include <compare>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace HexView {

  // An sRGB colour with alpha.
  struct Color {
    std::uint8_t r = 0;
    std::uint8_t g = 0;
    std::uint8_t b = 0;
    std::uint8_t a = 255;
    // "#rrggbb" as the palettes write it. Throws std::invalid_argument naming `what` (the palette id
    // or the element) for anything else; "none" is not a colour and is refused here too.
    static Color parse(std::string_view text, std::string_view what);
    auto operator<=>(const Color&) const = default;
  };

  enum class LineCap : std::uint8_t { Butt, Round, Square };
  enum class LineJoin : std::uint8_t { Miter, Round, Bevel };

  // hexsheet2svg.py defs(): dots, hatch, mottle, palms, each a tile drawn over the terrain fill.
  enum class Pattern : std::uint8_t { Dots, Hatch, Mottle, Palms };

  struct Fill {
    Color color;
    double opacity = 1.0;
  };

  struct Stroke {
    Color color;
    double width = 1.0;
    std::vector<double> dash;  // empty: solid
    double opacity = 1.0;
    LineCap cap = LineCap::Round;
    LineJoin join = LineJoin::Round;
  };

  enum class FontWeight : std::uint8_t { Normal, Bold };
  enum class TextAnchor : std::uint8_t { Start, Middle, End };
  // Where the anchor point sits vertically: on the alphabetic baseline (SVG's default), at the glyphs'
  // centre ("central": ids, glyph text, labels) or middle ("middle": text along a path).
  enum class TextBaseline : std::uint8_t { Alphabetic, Central, Middle };

  struct Font {
    std::string family;
    double size = 10.0;
    FontWeight weight = FontWeight::Normal;
    bool italicP = false;
    double letterSpacing = 0.0;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
