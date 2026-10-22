// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmaped/SheetFrame.h"

#include <cmath>
#include <numbers>
#include <stdexcept>

namespace HexMapEd {

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
      throw std::invalid_argument("grid '" + gridId + "': orientation '" + text + "' is neither flat nor pointy");
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
      throw std::invalid_argument("grid '" + gridId + "': offset '" + text + "' is neither odd nor even");
    }

    HexCoord::GridSpec
    toSpec(const HexXml::SheetGridDoc& g)
    {
      HexCoord::GridSpec spec;
      spec.id = g.id.value_or("");
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

    double
    angleDegrees(Pixel from, Pixel to)
    {
      const double a = std::atan2(to.y - from.y, to.x - from.x) * 180.0 / std::numbers::pi;
      return a < 0.0 ? a + 360.0 : a;
    }

    double
    angleGap(double a, double b)
    {
      const double d = std::fabs(a - b);
      return d > 180.0 ? 360.0 - d : d;
    }

  }  // namespace

  SheetFrame::SheetFrame(double width, double height, std::vector<HexCoord::Grid> grids)
    : width_(width), height_(height), grids_(std::move(grids))
  {
  }

  SheetFrame
  SheetFrame::of(const HexXml::SheetDoc& sheet)
  {
    if (sheet.grids.empty()) {
      throw std::invalid_argument("sheet '" + sheet.id + "': no grid element");
    }
    std::vector<HexCoord::Grid> grids;
    for (const HexXml::SheetGridDoc& g : sheet.grids) {
      const HexCoord::GridSpec spec = toSpec(g);
      if (grids.empty()) {
        grids.emplace_back(spec);
      } else {
        grids.emplace_back(spec, grids.front());
      }
    }
    return SheetFrame(sheet.width, sheet.height, std::move(grids));
  }

  HexCoord::Orientation
  SheetFrame::orientation() const
  {
    return grids_.front().spec().orientation;
  }

  std::vector<std::string>
  SheetFrame::ids() const
  {
    std::vector<std::string> out;
    for (const HexCoord::Grid& g : grids_) {
      for (const HexCoord::HexId& id : g.ids()) {
        out.push_back(id.text);
      }
    }
    return out;
  }

  bool
  SheetFrame::printsP(std::string_view id) const
  {
    for (const HexCoord::Grid& g : grids_) {
      if (g.find(HexCoord::HexId{std::string(id)}).has_value()) {
        return true;
      }
    }
    return false;
  }

  SheetFrame::Found
  SheetFrame::find(std::string_view id) const
  {
    for (const HexCoord::Grid& g : grids_) {
      const std::optional<HexCoord::GridIndex> index = g.find(HexCoord::HexId{std::string(id)});
      if (index.has_value()) {
        return Found{&g, *index};
      }
    }
    throw std::invalid_argument("hex '" + std::string(id) + "' is not printed on any grid of the sheet");
  }

  Pixel
  SheetFrame::centre(std::string_view id) const
  {
    const Found f = find(id);
    return f.grid->pixelOf(f.grid->centreOf(f.index));
  }

  std::array<Pixel, 6>
  SheetFrame::corners(std::string_view id, double inset) const
  {
    const Found f = find(id);
    return f.grid->polygon(f.grid->centreOf(f.index), inset);
  }

  HexCoord::Direction
  SheetFrame::directionOfSide(const HexCoord::Grid& grid, HexCoord::HexCentre centre, int corner) const
  {
    const std::array<Pixel, 6> poly = grid.polygon(centre);
    const Pixel middle = grid.pixelOf(centre);
    const Pixel a = poly[static_cast<std::size_t>(corner)];
    const Pixel b = poly[static_cast<std::size_t>((corner + 1) % 6)];
    const Pixel mid{(a.x + b.x) / 2.0, (a.y + b.y) / 2.0};
    const double angle = angleDegrees(middle, mid);
    HexCoord::Direction best = HexCoord::Direction::D0;
    double bestGap = 1e9;
    for (int k = 0; k < HexCoord::kDirections; ++k) {
      const auto d = static_cast<HexCoord::Direction>(k);
      const double gap = angleGap(angle, HexCoord::edgeAngleDegrees(d, grid.spec().orientation));
      if (gap < bestGap) {
        bestGap = gap;
        best = d;
      }
    }
    return best;
  }

  int
  SheetFrame::cornerOfDirection(std::string_view id, HexCoord::Direction dir) const
  {
    const Found f = find(id);
    const HexCoord::HexCentre centre = f.grid->centreOf(f.index);
    for (int corner = 0; corner < 6; ++corner) {
      if (dir == directionOfSide(*f.grid, centre, corner)) {
        return corner;
      }
    }
    throw std::invalid_argument("hex '" + std::string(id) + "': no side faces the requested direction");
  }

  std::pair<Pixel, Pixel>
  SheetFrame::hexsideEnds(const Hexside& side) const
  {
    const int corner = cornerOfDirection(side.hex, side.dir);
    const std::array<Pixel, 6> poly = corners(side.hex);
    return {poly[static_cast<std::size_t>(corner)], poly[static_cast<std::size_t>((corner + 1) % 6)]};
  }

  Pixel
  SheetFrame::hexsideMidpoint(const Hexside& side) const
  {
    const auto [a, b] = hexsideEnds(side);
    return Pixel{(a.x + b.x) / 2.0, (a.y + b.y) / 2.0};
  }

  std::optional<std::string>
  SheetFrame::neighbour(std::string_view id, HexCoord::Direction dir) const
  {
    const Found f = find(id);
    const HexCoord::HexCentre next = f.grid->centreOf(f.index).neighbour(dir);
    for (const HexCoord::Grid& g : grids_) {
      const std::optional<HexCoord::GridIndex> index = g.indexOf(next);
      if (index.has_value()) {
        return g.idOf(*index).text;
      }
    }
    return std::nullopt;
  }

  Hexside
  SheetFrame::hexside(std::string_view token) const
  {
    const std::size_t colon = token.find(':');
    if (std::string_view::npos == colon || 0 == colon || token.size() - 1 == colon) {
      throw std::invalid_argument("hexside '" + std::string(token) + "' is not HEX:DIR");
    }
    const std::string hex(token.substr(0, colon));
    const Found f = find(hex);
    return Hexside{hex, HexCoord::fromCompass(token.substr(colon + 1), f.grid->spec().orientation)};
  }

  std::string
  SheetFrame::token(const Hexside& side) const
  {
    const Found f = find(side.hex);
    return side.hex + ":" + std::string(HexCoord::compassName(side.dir, f.grid->spec().orientation));
  }

  std::string
  SheetFrame::canonical(const Hexside& side) const
  {
    const std::optional<std::string> other = neighbour(side.hex, side.dir);
    if (!other.has_value()) {
      return token(side);
    }
    const Found mine = find(side.hex);
    const Found theirs = find(*other);
    const bool mineFirstP = mine.grid < theirs.grid ||
                            (mine.grid == theirs.grid && mine.index < theirs.index);
    if (mineFirstP) {
      return token(side);
    }
    return token(Hexside{*other, HexCoord::opposite(side.dir)});
  }

  std::optional<std::string>
  SheetFrame::hexAt(Pixel p) const
  {
    for (const HexCoord::Grid& g : grids_) {
      const std::optional<HexCoord::HexCentre> centre = g.hexAt(p);
      if (centre.has_value()) {
        const std::optional<HexCoord::GridIndex> index = g.indexOf(*centre);
        if (index.has_value()) {
          return g.idOf(*index).text;
        }
      }
    }
    return std::nullopt;
  }

  std::optional<Hexside>
  SheetFrame::hexsideAt(Pixel p, double withinPixels) const
  {
    const std::optional<std::string> hex = hexAt(p);
    if (!hex.has_value()) {
      return std::nullopt;
    }
    const Found f = find(*hex);
    const HexCoord::HexCentre centre = f.grid->centreOf(f.index);
    const std::array<Pixel, 6> poly = f.grid->polygon(centre);
    int bestCorner = -1;
    double bestDistance = withinPixels;
    for (int corner = 0; corner < 6; ++corner) {
      const Pixel a = poly[static_cast<std::size_t>(corner)];
      const Pixel b = poly[static_cast<std::size_t>((corner + 1) % 6)];
      const double vx = b.x - a.x;
      const double vy = b.y - a.y;
      const double len2 = vx * vx + vy * vy;
      const double t = len2 > 0.0 ? std::max(0.0, std::min(1.0, ((p.x - a.x) * vx + (p.y - a.y) * vy) / len2)) : 0.0;
      const double dx = p.x - (a.x + t * vx);
      const double dy = p.y - (a.y + t * vy);
      const double distance = std::sqrt(dx * dx + dy * dy);
      if (distance < bestDistance) {
        bestDistance = distance;
        bestCorner = corner;
      }
    }
    if (0 > bestCorner) {
      return std::nullopt;
    }
    return Hexside{*hex, directionOfSide(*f.grid, centre, bestCorner)};
  }

}  // namespace HexMapEd
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
