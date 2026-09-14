// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] A sheet's pixel frame: its size and its grids on one ABC lattice, with the
// hex, corner and hexside geometry every builder draws from. Printed hex ids in, pixels out; the
// arithmetic is hexcoord's, so it agrees with hexsheet2svg.py's Grid to within GridTest's tolerance.
// ----------------------------------------------
#pragma once
#include "hexcoord/Direction.h"
#include "hexcoord/Grid.h"
#include "hexxml/SheetDoc.h"

#include <array>
#include <optional>
#include <string_view>
#include <utility>
#include <vector>

namespace HexView {

  using HexCoord::HexId;
  using HexCoord::Pixel;

  // A hexside as the sheet writes it ("KK20:e"), resolved against the sheet's orientation.
  struct EdgeRef {
    HexId hex;
    HexCoord::Direction dir;
  };

  class MapFrame {
  public:
    // Throws std::invalid_argument naming the grid (bad spec, a second grid off the first's lattice)
    // or the sheet (no grid, non-positive size).
    static MapFrame of(const HexXml::SheetDoc&);

    double width() const;
    double height() const;
    HexCoord::Orientation orientation() const;
    const std::vector<HexCoord::Grid>& grids() const;

    // Each throws std::invalid_argument naming the id when no grid prints it.
    Pixel centre(const HexId&) const;
    std::array<Pixel, 6> corners(const HexId&, double inset = 0.0) const;
    std::pair<Pixel, Pixel> hexsideEnds(const EdgeRef&) const;
    Pixel hexsideMidpoint(const EdgeRef&) const;
    // "HEX:DIR" to an EdgeRef; throws naming the token for a bad direction or an unknown hex.
    EdgeRef edgeRef(std::string_view token) const;

    // The printed hex under a pixel; nullopt off every grid or in a clipped cell.
    std::optional<HexId> hexAt(Pixel) const;

  private:
    MapFrame(double width, double height, std::vector<HexCoord::Grid> grids);
    double width_;
    double height_;
    std::vector<HexCoord::Grid> grids_;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
