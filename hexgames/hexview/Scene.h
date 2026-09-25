// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] The presentation model: a Scene is layers of primitives, each with a hit
// tag, in sheet pixels. Qt6 (hexqt) paints it with QPainter, the HTML client draws its JSON form, and
// the SVG writer turns it into files comparable with the Python reference renderers. No Qt here.
// ----------------------------------------------
#pragma once
#include "hexcoord/Direction.h"
#include "hexcoord/Grid.h"
#include "hexmodel/Ids.h"
#include "hexview/Style.h"

#include <array>
#include <optional>
#include <span>
#include <string>
#include <variant>
#include <vector>

namespace HexView {

  using HexCoord::Pixel;

  // Path commands in sheet pixels: the SVG subset the reference renderers emit.
  struct MoveTo { Pixel to; };
  struct LineTo { Pixel to; };
  struct QuadTo { Pixel control; Pixel to; };  // rounded river corners (LineGeometry.h)
  // An SVG elliptical arc: circles, rounded rectangles and the arcs of the reference symbols.
  struct ArcTo {
    double rx = 0.0;
    double ry = 0.0;
    double rotationDegrees = 0.0;
    bool largeArcP = false;
    bool sweepP = false;
    Pixel to;
  };
  struct ClosePath {};
  using PathCommand = std::variant<MoveTo, LineTo, QuadTo, ArcTo, ClosePath>;

  struct PathShape {
    std::vector<PathCommand> commands;
    std::optional<Fill> fill;
    std::optional<Pattern> pattern;  // drawn over the fill, clipped to the path
    std::optional<Stroke> stroke;
  };

  struct TextShape {
    Pixel at;
    std::string text;
    Font font;
    Color color;
    TextAnchor anchor = TextAnchor::Middle;
    TextBaseline baseline = TextBaseline::Alphabetic;
    double angleDegrees = 0.0;
    // An outline painted under the glyphs (SVG paint-order="stroke"): the white halo of an authored
    // label, the black edge of an edge label. Unset: no outline.
    std::optional<Stroke> halo;
  };

  // A named symbol from the SymbolLibrary, placed, turned and scaled; `color` is its currentColor.
  struct SymbolUse {
    std::string symbol;
    Pixel at;
    double rotateDegrees = 0.0;
    double scale = 1.0;
    Color color;
  };

  using Shape = std::variant<PathShape, TextShape, SymbolUse>;

  // What a pointer over a primitive is over.
  struct NoHit {};
  struct HexHit { HexModel::HexIndex hex; };
  struct HexsideHit { HexModel::HexIndex hex; HexCoord::Direction dir; };
  struct UnitHit { HexModel::UnitId unit; };
  struct SpaceHit { HexModel::SpaceId space; };
  struct PanelHit { std::string panel; };
  using HitTag = std::variant<NoHit, HexHit, HexsideHit, UnitHit, SpaceHit, PanelHit>;

  struct Primitive {
    Shape shape;
    HitTag hit;
  };

  // Map layers in hexsheet2svg.py render() order, then the play-state layers on top.
  enum class Layer : std::uint8_t {
    Background, Terrain, Regions, Grid, Edges, Links, Rings, HexGlyphs, SideGlyphs, Labels, Panels,
    Control, Units, Markers, Highlights, Overlay
  };
  inline constexpr std::size_t kLayerCount = 16;

  class Scene {
  public:
    // Throws std::invalid_argument unless both sizes are positive.
    Scene(double width, double height);

    double width() const { return width_; }
    double height() const { return height_; }
    void add(Layer, Primitive);
    std::span<const Primitive> layer(Layer) const;
    // The hit tag of the topmost primitive under a pixel, layers from the top down; NoHit if none.
    HitTag hitAt(Pixel) const;

  private:
    double width_;
    double height_;
    std::array<std::vector<Primitive>, kLayerCount> layers_;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
