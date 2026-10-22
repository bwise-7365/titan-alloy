// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#pragma once
// SheetFrame -- the pixel geometry of a sheet document: printed ids in, pixels out. The editor's one
// geometry, built from the sheet's grid elements through hexcoord::Grid exactly as HexModel's
// BoardBuilder builds them, so that what the editor draws is what the engine and the SVG renderer
// see. Offset (col, row) never leaves this class; printed ids are the identity.

#include "hexcoord/Direction.h"
#include "hexcoord/Grid.h"
#include "hexxml/SheetDoc.h"

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace HexMapEd {

  using HexCoord::Pixel;

  struct Hexside {
    std::string hex;  // printed id
    HexCoord::Direction dir;
  };

  class SheetFrame {
  public:
    static SheetFrame of(const HexXml::SheetDoc&);

    double width() const { return width_; }
    double height() const { return height_; }
    HexCoord::Orientation orientation() const;
    const std::vector<HexCoord::Grid>& grids() const { return grids_; }
    std::vector<std::string> ids() const;  // every printed id of every grid, grid order then row-major

    bool printsP(std::string_view id) const;
    Pixel centre(std::string_view id) const;                            // throws: unknown id
    std::array<Pixel, 6> corners(std::string_view id, double inset = 0.0) const;
    std::pair<Pixel, Pixel> hexsideEnds(const Hexside&) const;
    Pixel hexsideMidpoint(const Hexside&) const;
    std::optional<std::string> neighbour(std::string_view id, HexCoord::Direction) const;

    // "HEX:DIR" tokens, the sheet's own spelling of a hexside.
    Hexside hexside(std::string_view token) const;                      // throws: bad token
    std::string token(const Hexside&) const;
    // The canonical name of a hexside shared by two hexes is the one the lower-indexed hex gives it;
    // a map-edge hexside has only one name.
    std::string canonical(const Hexside&) const;

    std::optional<std::string> hexAt(Pixel) const;                      // nullopt off every grid
    std::optional<Hexside> hexsideAt(Pixel, double withinPixels) const;  // nearest side of the hex at the point

  private:
    SheetFrame(double width, double height, std::vector<HexCoord::Grid> grids);
    struct Found {
      const HexCoord::Grid* grid;
      HexCoord::GridIndex index;
    };
    Found find(std::string_view id) const;
    HexCoord::Direction directionOfSide(const HexCoord::Grid&, HexCoord::HexCentre, int corner) const;
    int cornerOfDirection(std::string_view id, HexCoord::Direction) const;

    double width_;
    double height_;
    std::vector<HexCoord::Grid> grids_;
  };

}  // namespace HexMapEd
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
