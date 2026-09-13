// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// A sheet grid: the bridge between printed hex identifiers, offset (column, row) indices, ABC
// coordinates and pixels. Everything here follows hexsheet.xsd's <grid> element and reproduces
// hexsheet2svg.py's Grid class (make_id, centre, neighbour, letters).
// ----------------------------------------------
#pragma once
#include "Direction.h"
#include "HexAddress.h"

#include <array>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace HexCoord {

  // A printed hex identifier such as "KK19", "2336", "w5227", "0109".
  struct HexId {
    std::string text;
    auto operator<=>(const HexId&) const = default;
  };

  struct GridIndex {
    int col = 0;  // zero-based grid index, not the printed number
    int row = 0;
    auto operator<=>(const GridIndex&) const = default;
  };

  struct Pixel {
    double x = 0.0;
    double y = 0.0;
  };

  // The <grid> element, verbatim.
  struct GridSpec {
    std::string id;
    Orientation orientation = Orientation::Flat;
    Parity offset = Parity::Odd;  // which columns (flat) or rows (pointy) are shifted
    int cols = 0;
    int rows = 0;
    double size = 0.0;  // circumradius in sheet pixels
    double ox = 0.0;    // pixel centre of grid cell (0, 0)
    double oy = 0.0;
    std::string idFormat = "{col}{row}";
    int colStart = 1;
    int colStep = 1;
    int rowStart = 1;
    int rowStep = 1;
    std::string clip;  // "A1-A9 B1-B3": printed ids removed from the grid
  };

  // Renders printed identifiers from printed column and row numbers.
  // Placeholders: {col} {row} {col:02} {row:02} {rowletter} {colletter}; anything else is literal.
  // Letters: 1 -> A .. 26 -> Z, 27 -> AA, 28 -> BB ... (doubled letters, as TRC's row names).
  class HexIdFormat {
  public:
    // Throws std::invalid_argument on an unknown placeholder.
    explicit HexIdFormat(std::string_view format);
    std::string render(int printedCol, int printedRow) const;

  private:
    std::string format_;
  };

  std::string letters(int n);  // throws std::invalid_argument for n < 1

  // The offset (col, row) frame. It exists here and nowhere else in the engine. Flat-topped grids
  // shift odd columns down and pointy-topped ones shift odd rows right; Parity::Even shifts the
  // others, which is the same lattice read one index over. Unbounded: no grid extent is consulted.
  Abc abcOfIndex(GridIndex, Orientation, Parity);
  GridIndex indexOfAbc(HexCentre, Orientation, Parity);

  // How far a second grid's pixel origin may sit from the shared lattice, as a fraction of size.
  // The sheet XML's origins are fitted to a scan by hand, so they are good to a small part of a hex
  // and no better: Dai Senso's two maps are 0.0012 * size apart.
  inline constexpr double kLatticeTolerance = 0.05;

  // One grid on a sheet. Several grids on one sheet (Dai Senso's west and east maps) share one ABC
  // lattice: latticeOffset places this grid's (0, 0) cell on it, and the constructor throws unless
  // the pixel origin sits on that lattice within 1e-6 * size.
  class Grid {
  public:
    // Throws std::invalid_argument on an empty extent, a non-positive size, an unknown placeholder
    // in the id format, or a lattice offset that is not a hex centre.
    explicit Grid(GridSpec spec, Abc latticeOffset = Abc{});
    // A second grid of the same sheet, on the lattice of an existing one: the offset is read off the
    // two pixel origins. Throws unless the orientation and size agree and the origin lands on a hex
    // centre of that lattice within kLatticeTolerance * size.
    Grid(GridSpec spec, const Grid& sheetLattice);

    const GridSpec& spec() const { return spec_; }
    // The ABC coordinate of this grid's cell (0, 0) on the sheet's lattice.
    Abc latticeOffset() const { return base_; }

    // (col, row) <-> centre. The offset formulas are the two documented in tricoord for flat-topped
    // grids (odd columns shifted down) and their pointy-topped counterpart (odd rows shifted
    // right); offset="even" evaluates the shifted formula one index over and subtracts the base.
    HexCentre centreOf(GridIndex) const;              // throws if outside the grid
    std::optional<GridIndex> indexOf(HexCentre) const;  // nullopt: not on this grid (or clipped)

    // printed id <-> index. Ids are generated once and interned; unknown ids are absent, never
    // guessed.
    const HexId& idOf(GridIndex) const;
    std::optional<GridIndex> find(const HexId&) const;
    const std::vector<HexId>& ids() const { return ids_; }  // row-major, clipped cells omitted

    // Pixels for any lattice point (centre, vertex, or twice-midpoint / 2).
    Pixel pixelOf(Abc) const;
    Pixel pixelOf(HexCentre centre) const { return pixelOf(centre.abc()); }
    // The hex whose polygon contains the pixel, by inverse basis and cube rounding; nullopt off-grid.
    std::optional<HexCentre> hexAt(Pixel) const;
    // The six corner pixels, clockwise from the leading corner of Direction::D0; inset shrinks
    // toward the centre by that fraction of size.
    std::array<Pixel, 6> polygon(HexCentre, double inset = 0.0) const;

    // "A1-A9" style inclusive ranges along one axis, as the renderer's expand_range.
    std::vector<HexId> expandRange(const HexId& from, const HexId& to) const;

  private:
    // The lattice hex nearest a pixel, with no regard for the grid's extent; hexAt() adds the
    // extent and the clip, and the sheet-lattice constructor uses it to place a second grid.
    HexCentre nearestCentre(Pixel) const;
    // The lattice offset that puts `other`'s pixel origin on this grid's lattice, or a throw.
    Abc offsetFor(const GridSpec& other) const;
    // The printed id of a cell and the cell of a printed id, both ignoring the clip, as the
    // renderer's make_id and locate do while the clip is still being resolved.
    HexId renderedId(GridIndex) const;
    std::optional<GridIndex> locate(const HexId&) const;
    std::vector<HexId> clippedIds() const;

    GridSpec spec_;
    HexIdFormat format_;
    Abc base_;
    Pixel origin_;  // the pixel of the lattice point Abc{}, which is not this grid's cell (0, 0)
    std::vector<HexId> ids_;
    std::map<HexId, GridIndex> byId_;
    std::map<GridIndex, HexId> byIndex_;
  };

}  // namespace HexCoord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
