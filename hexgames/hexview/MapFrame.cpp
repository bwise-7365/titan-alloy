// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Centres come from hexcoord's Grid; corners and hexside midpoints are placed around a centre by
// angle, exactly as hexsheet2svg.py's Grid.corner and Grid.edge_mid place them, so the polygons
// list their corners in the reference's order.
// ----------------------------------------------
#include "hexview/MapFrame.h"

#include <cmath>
#include <numbers>
#include <stdexcept>
#include <string>

namespace HexView {

  namespace {

    HexCoord::Orientation
    parseOrientation(const std::string& gridId, const std::string& text)
    {
      if ("flat" == text) {
        return HexCoord::Orientation::Flat;
      }
      if ("pointy" == text) {
        return HexCoord::Orientation::Pointy;
      }
      throw std::invalid_argument("grid '" + gridId + "': orientation '" + text +
                                  "' is neither flat nor pointy");
    }

    HexCoord::Parity
    parseParity(const std::string& gridId, const std::string& text)
    {
      if ("odd" == text) {
        return HexCoord::Parity::Odd;
      }
      if ("even" == text) {
        return HexCoord::Parity::Even;
      }
      throw std::invalid_argument("grid '" + gridId + "': offset '" + text +
                                  "' is neither odd nor even");
    }

    HexCoord::GridSpec
    toSpec(const HexXml::SheetGridDoc& g)
    {
      HexCoord::GridSpec spec;
      spec.id = g.id.value_or("g");
      spec.orientation = parseOrientation(spec.id, g.orientation);
      spec.offset = parseParity(spec.id, g.offset);
      spec.cols = g.cols;
      spec.rows = g.rows;
      spec.size = g.size;
      spec.ox = g.ox;
      spec.oy = g.oy;
      spec.idFormat = g.idFormat;
      spec.colStart = g.colStart;
      spec.colStep = g.colStep;
      spec.rowStart = g.rowStart;
      spec.rowStep = g.rowStep;
      spec.clip = g.clip.value_or("");
      return spec;
    }

    Pixel
    around(Pixel centre, double radius, double degrees)
    {
      const double a = degrees * std::numbers::pi / 180.0;
      return Pixel{centre.x + radius * std::cos(a), centre.y + radius * std::sin(a)};
    }

    double
    firstCornerDegrees(HexCoord::Orientation o)
    {
      switch (o) {
      case HexCoord::Orientation::Flat:
        return 0.0;
      case HexCoord::Orientation::Pointy:
        return 30.0;
      }
      throw std::invalid_argument("MapFrame: orientation is neither Flat nor Pointy");
    }

  }  // namespace

  MapFrame::MapFrame(double width, double height, std::vector<HexCoord::Grid> grids)
      : width_(width), height_(height), grids_(std::move(grids))
  {}

  MapFrame
  MapFrame::of(const HexXml::SheetDoc& sheet)
  {
    if (sheet.grids.empty()) {
      throw std::invalid_argument("sheet '" + sheet.id + "': no grid element");
    }
    if (!(0.0 < sheet.width) || !(0.0 < sheet.height)) {
      throw std::invalid_argument("sheet '" + sheet.id + "': size is not positive");
    }
    std::vector<HexCoord::Grid> grids;
    for (const HexXml::SheetGridDoc& g : sheet.grids) {
      if (grids.empty()) {
        grids.emplace_back(toSpec(g));
      }
      else {
        grids.emplace_back(toSpec(g), grids.front());
      }
    }
    return MapFrame(sheet.width, sheet.height, std::move(grids));
  }

  double
  MapFrame::width() const
  {
    return width_;
  }

  double
  MapFrame::height() const
  {
    return height_;
  }

  HexCoord::Orientation
  MapFrame::orientation() const
  {
    return grids_.front().spec().orientation;
  }

  const std::vector<HexCoord::Grid>&
  MapFrame::grids() const
  {
    return grids_;
  }

  const HexCoord::Grid&
  MapFrame::gridOf(const HexId& id) const
  {
    for (const HexCoord::Grid& g : grids_) {
      if (g.find(id).has_value()) {
        return g;
      }
    }
    throw std::invalid_argument("hex '" + id.text + "' is not printed on any grid of the sheet");
  }

  HexModel::HexIndex
  MapFrame::index(const HexId& id) const
  {
    std::uint32_t before = 0;
    for (const HexCoord::Grid& g : grids_) {
      if (g.find(id).has_value()) {
        const std::vector<HexId>& ids = g.ids();
        for (std::size_t k = 0; k < ids.size(); ++k) {
          if (id == ids[k]) {
            return HexModel::HexIndex{before + static_cast<std::uint32_t>(k)};
          }
        }
      }
      before += static_cast<std::uint32_t>(g.ids().size());
    }
    throw std::invalid_argument("hex '" + id.text + "' is not printed on any grid of the sheet");
  }

  Pixel
  MapFrame::centre(const HexId& id) const
  {
    const HexCoord::Grid& g = gridOf(id);
    return g.pixelOf(g.centreOf(*g.find(id)));
  }

  std::array<Pixel, 6>
  MapFrame::corners(const HexId& id, double inset) const
  {
    const HexCoord::Grid& g = gridOf(id);
    const Pixel c = g.pixelOf(g.centreOf(*g.find(id)));
    const double radius = g.spec().size * (1.0 - inset);
    const double first = firstCornerDegrees(g.spec().orientation);
    std::array<Pixel, 6> out;
    for (std::size_t k = 0; k < 6; ++k) {
      out[k] = around(c, radius, first + 60.0 * static_cast<double>(k));
    }
    return out;
  }

  std::pair<Pixel, Pixel>
  MapFrame::hexsideEnds(const EdgeRef& e) const
  {
    const HexCoord::Grid& g = gridOf(e.hex);
    const Pixel c = centre(e.hex);
    const double a = HexCoord::edgeAngleDegrees(e.dir, g.spec().orientation);
    return {around(c, g.spec().size, a - 30.0), around(c, g.spec().size, a + 30.0)};
  }

  Pixel
  MapFrame::hexsideMidpoint(const EdgeRef& e) const
  {
    const HexCoord::Grid& g = gridOf(e.hex);
    const double a = HexCoord::edgeAngleDegrees(e.dir, g.spec().orientation);
    return around(centre(e.hex), g.spec().size * std::sqrt(3.0) / 2.0, a);
  }

  EdgeRef
  MapFrame::edgeRef(std::string_view token) const
  {
    const std::size_t colon = token.find(':');
    if (std::string_view::npos == colon || 0 == colon || token.size() - 1 == colon) {
      throw std::invalid_argument("hexside '" + std::string(token) + "' is not HEX:DIR");
    }
    const HexId hex{std::string(token.substr(0, colon))};
    const HexCoord::Grid& g = gridOf(hex);
    try {
      return EdgeRef{hex, HexCoord::fromCompass(token.substr(colon + 1), g.spec().orientation)};
    }
    catch (const std::invalid_argument& e) {
      throw std::invalid_argument("hexside '" + std::string(token) + "': " + e.what());
    }
  }

  std::optional<HexId>
  MapFrame::hexAt(Pixel p) const
  {
    for (const HexCoord::Grid& g : grids_) {
      const std::optional<HexCoord::HexCentre> centre = g.hexAt(p);
      if (centre.has_value()) {
        const std::optional<HexCoord::GridIndex> index = g.indexOf(*centre);
        if (index.has_value()) {
          return g.idOf(*index);
        }
      }
    }
    return std::nullopt;
  }

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
