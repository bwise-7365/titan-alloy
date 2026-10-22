// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Printed identifiers, offset indices, ABC coordinates and pixels, for one grid of one sheet.
//
// The offset formulas are tricoord's CoordABC(row, clm) for flat-topped grids with odd columns
// shifted down, and its pointy-topped counterpart, derived so that the pixel mapping below agrees
// with hexsheet2svg.py's Grid.centre() for both orientations:
//
//   flat,   column 2n    -> (3n,     -row,        row)      column 2n+1 -> (3n+1, -(row+1), row)
//   pointy, row    2m    -> (col,    -3m,        -col)      row    2m+1 -> (col+1, -(3m+1), -col)
//
// Parity::Even keeps the lattice and the ABC origin and pushes the other columns (flat) or rows
// (pointy) down or right: a pushed cell is the odd frame's cell one index further along its own
// column or row. The ABC origin is then the centre of the unprinted hex just before cell (0, 0),
// half a hex before hexsheet2svg.py's (ox, oy) along the shifted axis (doc/hex-ABC-offset-*.svg).
// ----------------------------------------------
#include "Grid.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace HexCoord {

  namespace {

    constexpr double kSqrt3 = 1.7320508075688772;

    struct Basis {
      Pixel a;
      Pixel b;
      Pixel c;
    };

    // Flat: A due east, B and C to the two north-west and south-west corners. Pointy: the same three
    // turned 30 degrees clockwise on screen, which is the turn that takes the flat compass names to
    // the pointy ones.
    Basis
    basisOf(Orientation orientation, double size)
    {
      const double half = size / 2.0;
      const double tall = kSqrt3 * size / 2.0;
      switch (orientation) {
        case Orientation::Flat:
          return Basis{Pixel{size, 0.0}, Pixel{-half, -tall}, Pixel{-half, tall}};
        case Orientation::Pointy:
          return Basis{Pixel{tall, half}, Pixel{0.0, -size}, Pixel{-tall, half}};
      }
      throw std::invalid_argument("basisOf: orientation is neither Flat nor Pointy");
    }

    Pixel
    combine(const Basis& basis, Abc v)
    {
      const double x = v.a() * basis.a.x + v.b() * basis.b.x + v.c() * basis.c.x;
      const double y = v.a() * basis.a.y + v.b() * basis.b.y + v.c() * basis.c.y;
      return Pixel{x, y};
    }

    // The nearest lattice hex to a displacement from the lattice origin: solve the 2x2 system in the
    // sum-zero ABC frame (the cube frame of a hex grid), then round the three fractional components
    // to integers summing to zero by correcting the one that moved furthest.
    Abc
    roundToCentre(const Basis& basis, Pixel rel)
    {
      const double ex = basis.a.x - basis.c.x;
      const double ey = basis.a.y - basis.c.y;
      const double fx = basis.b.x - basis.c.x;
      const double fy = basis.b.y - basis.c.y;
      const double det = ex * fy - fx * ey;
      const double fa = (rel.x * fy - fx * rel.y) / det;
      const double fb = (ex * rel.y - rel.x * ey) / det;
      const double fc = -fa - fb;

      double ra = std::round(fa);
      double rb = std::round(fb);
      double rc = std::round(fc);
      const double da = std::abs(ra - fa);
      const double db = std::abs(rb - fb);
      const double dc = std::abs(rc - fc);
      if (da > db && da > dc) {
        ra = -rb - rc;
      }
      else if (db > dc) {
        rb = -ra - rc;
      }
      else {
        rc = -ra - rb;
      }
      return Abc{static_cast<int>(ra), static_cast<int>(rb), static_cast<int>(rc)};
    }

    Abc
    abcOfIndexOdd(GridIndex index, Orientation orientation)
    {
      switch (orientation) {
        case Orientation::Flat: {
          if (0 == iMod(index.col, 2)) {
            const int n = index.col / 2;
            return Abc{3 * n, -index.row, index.row};
          }
          const int n = (index.col - 1) / 2;
          return Abc{3 * n + 1, -(index.row + 1), index.row};
        }
        case Orientation::Pointy: {
          if (0 == iMod(index.row, 2)) {
            const int m = index.row / 2;
            return Abc{index.col, -3 * m, -index.col};
          }
          const int m = (index.row - 1) / 2;
          return Abc{index.col + 1, -(3 * m + 1), -index.col};
        }
      }
      throw std::invalid_argument("abcOfIndexOdd: orientation is neither Flat nor Pointy");
    }

    // tricoord's getRC, and its pointy-topped counterpart: the shifted axis is read off a difference
    // that survives the arbitrary diagonal shift, and the other axis from the mean of the rest.
    GridIndex
    indexOfAbcOdd(HexCentre centre, Orientation orientation)
    {
      const Qrs v = centre.qrs();
      switch (orientation) {
        case Orientation::Flat: {
          const int col = v.r() - v.s();
          const int shift = 0 == iMod(col, 2) ? (v.r() + v.s()) / 2 : (v.r() + v.s() + 1) / 2;
          return GridIndex{col, v.q() - shift};
        }
        case Orientation::Pointy: {
          const int row = v.q() - v.s();
          const int shift = 0 == iMod(row, 2) ? (v.q() + v.s()) / 2 : (v.q() + v.s() + 1) / 2;
          return GridIndex{v.r() - shift, row};
        }
      }
      throw std::invalid_argument("indexOfAbcOdd: orientation is neither Flat nor Pointy");
    }

    // An even-frame cell as the odd-frame cell at the same place: the pushed columns (flat) or rows
    // (pointy) are the even ones, one index further along their own column or row.
    GridIndex
    oddIndexOfEven(GridIndex index, Orientation orientation)
    {
      switch (orientation) {
        case Orientation::Flat:
          return 0 == iMod(index.col, 2) ? GridIndex{index.col, index.row + 1} : index;
        case Orientation::Pointy:
          return 0 == iMod(index.row, 2) ? GridIndex{index.col + 1, index.row} : index;
      }
      throw std::invalid_argument("oddIndexOfEven: orientation is neither Flat nor Pointy");
    }

    GridIndex
    evenIndexOfOdd(GridIndex index, Orientation orientation)
    {
      switch (orientation) {
        case Orientation::Flat:
          return 0 == iMod(index.col, 2) ? GridIndex{index.col, index.row - 1} : index;
        case Orientation::Pointy:
          return 0 == iMod(index.row, 2) ? GridIndex{index.col - 1, index.row} : index;
      }
      throw std::invalid_argument("evenIndexOfOdd: orientation is neither Flat nor Pointy");
    }

    // (ox, oy) moved half a hex back along the shifted axis: up (flat) or left (pointy).
    Pixel
    halfHexBack(const GridSpec& spec)
    {
      const double half = kSqrt3 / 2.0 * spec.size;
      switch (spec.orientation) {
        case Orientation::Flat:
          return Pixel{spec.ox, spec.oy - half};
        case Orientation::Pointy:
          return Pixel{spec.ox - half, spec.oy};
      }
      throw std::invalid_argument("halfHexBack: orientation is neither Flat nor Pointy");
    }

    // The pixel of the grid's ABC origin: (ox, oy) on an odd grid, where it is the centre of cell
    // (0, 0); half a hex back on an even grid, the centre of the unprinted hex before cell (0, 0).
    Pixel
    abcOriginPixel(const GridSpec& spec)
    {
      switch (spec.offset) {
        case Parity::Odd:
          return Pixel{spec.ox, spec.oy};
        case Parity::Even:
          return halfHexBack(spec);
      }
      throw std::invalid_argument("abcOriginPixel: offset parity is neither Odd nor Even");
    }

    std::string
    zeroFill(std::string text, int width)
    {
      if (static_cast<int>(text.size()) >= width) {
        return text;
      }
      std::string sign;
      if ('-' == text.front() || '+' == text.front()) {
        sign = text.substr(0, 1);
        text = text.substr(1);
      }
      const int pad = width - static_cast<int>(sign.size() + text.size());
      return sign + std::string(static_cast<std::size_t>(pad), '0') + text;
    }

    std::string
    expandPlaceholder(const std::string& body, int printedCol, int printedRow)
    {
      const std::size_t colon = body.find(':');
      const std::string key = body.substr(0, colon);
      std::string value;
      if ("col" == key) {
        value = std::to_string(printedCol);
      }
      else if ("row" == key) {
        value = std::to_string(printedRow);
      }
      else if ("colletter" == key) {
        value = letters(printedCol);
      }
      else if ("rowletter" == key) {
        value = letters(printedRow);
      }
      else {
        throw std::invalid_argument("HexIdFormat: unknown placeholder '{" + body + "}'");
      }
      if (std::string::npos == colon) {
        return value;
      }
      std::string digits = body.substr(colon + 1);
      if (!digits.empty() && '0' == digits.front()) {
        digits = digits.substr(1);
      }
      if (digits.empty() || std::string::npos != digits.find_first_not_of("0123456789")) {
        throw std::invalid_argument("HexIdFormat: '{" + body + "}' has no field width");
      }
      return zeroFill(value, std::stoi(digits));
    }

  }  // namespace

  std::string
  letters(int n)
  {
    if (1 > n) {
      throw std::invalid_argument("letters: " + std::to_string(n) + " is not a row or column number");
    }
    if (26 >= n) {
      return std::string(1, static_cast<char>(64 + n));
    }
    return std::string(2, static_cast<char>(64 + n - 26));
  }

  HexIdFormat::HexIdFormat(std::string_view format) : format_(format)
  {
    render(1, 1);  // the placeholders are checked once, here, and never again
  }

  std::string
  HexIdFormat::render(int printedCol, int printedRow) const
  {
    std::string out;
    std::size_t i = 0;
    while (i < format_.size()) {
      if ('{' != format_[i]) {
        out += format_[i];
        ++i;
        continue;
      }
      const std::size_t close = format_.find('}', i);
      if (std::string::npos == close) {
        throw std::invalid_argument("HexIdFormat: unclosed '{' in '" + format_ + "'");
      }
      out += expandPlaceholder(format_.substr(i + 1, close - i - 1), printedCol, printedRow);
      i = close + 1;
    }
    return out;
  }

  Abc
  abcOfIndex(GridIndex index, Orientation orientation, Parity offset)
  {
    switch (offset) {
      case Parity::Odd:
        return abcOfIndexOdd(index, orientation);
      case Parity::Even:
        return abcOfIndexOdd(oddIndexOfEven(index, orientation), orientation);
    }
    throw std::invalid_argument("abcOfIndex: offset parity is neither Odd nor Even");
  }

  GridIndex
  indexOfAbc(HexCentre centre, Orientation orientation, Parity offset)
  {
    switch (offset) {
      case Parity::Odd:
        return indexOfAbcOdd(centre, orientation);
      case Parity::Even:
        return evenIndexOfOdd(indexOfAbcOdd(centre, orientation), orientation);
    }
    throw std::invalid_argument("indexOfAbc: offset parity is neither Odd nor Even");
  }

  Grid::Grid(GridSpec spec, Abc latticeOffset)
      : spec_(std::move(spec)), format_(spec_.idFormat), base_(latticeOffset), origin_{0.0, 0.0}
  {
    if (1 > spec_.cols || 1 > spec_.rows) {
      throw std::invalid_argument("Grid '" + spec_.id + "': extent " + std::to_string(spec_.cols) +
                                  " by " + std::to_string(spec_.rows) + " is empty");
    }
    if (0.0 >= spec_.size) {
      throw std::invalid_argument("Grid '" + spec_.id + "': size " + std::to_string(spec_.size) +
                                  " is not positive");
    }
    if (!isCentreCode(base_.hvCode())) {
      throw std::invalid_argument("Grid '" + spec_.id + "': lattice offset has hvCode " +
                                  std::to_string(base_.hvCode()) + ", so it is a vertex, not a hex");
    }
    const Pixel here = combine(basisOf(spec_.orientation, spec_.size), base_);
    const Pixel abcOrigin = abcOriginPixel(spec_);
    origin_ = Pixel{abcOrigin.x - here.x, abcOrigin.y - here.y};

    const std::vector<HexId> clipped = clippedIds();
    for (int row = 0; row < spec_.rows; ++row) {
      for (int col = 0; col < spec_.cols; ++col) {
        const GridIndex index{col, row};
        const HexId id = renderedId(index);
        if (clipped.end() != std::find(clipped.begin(), clipped.end(), id)) {
          continue;
        }
        ids_.push_back(id);
        byId_.emplace(id, index);
        byIndex_.emplace(index, id);
      }
    }
    return;
  }

  Grid::Grid(GridSpec spec, const Grid& sheetLattice)
      : Grid(spec, sheetLattice.offsetFor(spec))
  {
  }

  HexCentre
  Grid::centreOf(GridIndex index) const
  {
    if (0 > index.col || index.col >= spec_.cols || 0 > index.row || index.row >= spec_.rows) {
      throw std::invalid_argument("Grid '" + spec_.id + "': cell " + std::to_string(index.col) +
                                  ", " + std::to_string(index.row) + " is outside the grid");
    }
    return HexCentre{abcOfIndex(index, spec_.orientation, spec_.offset) + base_};
  }

  std::optional<GridIndex>
  Grid::indexOf(HexCentre centre) const
  {
    const HexCentre local{centre.abc() - base_};
    const GridIndex index = indexOfAbc(local, spec_.orientation, spec_.offset);
    if (0 > index.col || index.col >= spec_.cols || 0 > index.row || index.row >= spec_.rows) {
      return std::nullopt;
    }
    if (byIndex_.end() == byIndex_.find(index)) {
      return std::nullopt;
    }
    return index;
  }

  const HexId&
  Grid::idOf(GridIndex index) const
  {
    const auto found = byIndex_.find(index);
    if (byIndex_.end() == found) {
      throw std::invalid_argument("Grid '" + spec_.id + "': cell " + std::to_string(index.col) +
                                  ", " + std::to_string(index.row) + " has no printed id");
    }
    return found->second;
  }

  std::optional<GridIndex>
  Grid::find(const HexId& id) const
  {
    const auto found = byId_.find(id);
    if (byId_.end() == found) {
      return std::nullopt;
    }
    return found->second;
  }

  Pixel
  Grid::pixelOf(Abc v) const
  {
    const Pixel here = combine(basisOf(spec_.orientation, spec_.size), v);
    return Pixel{origin_.x + here.x, origin_.y + here.y};
  }

  HexCentre
  Grid::nearestCentre(Pixel pixel) const
  {
    const Pixel rel{pixel.x - origin_.x, pixel.y - origin_.y};
    return HexCentre{roundToCentre(basisOf(spec_.orientation, spec_.size), rel)};
  }

  std::optional<HexCentre>
  Grid::hexAt(Pixel pixel) const
  {
    const HexCentre centre = nearestCentre(pixel);
    if (!indexOf(centre).has_value()) {
      return std::nullopt;
    }
    return centre;
  }

  std::array<Pixel, 6>
  Grid::polygon(HexCentre centre, double inset) const
  {
    if (0.0 > inset || 1.0 <= inset) {
      throw std::invalid_argument("Grid '" + spec_.id + "': inset " + std::to_string(inset) +
                                  " is not in [0, 1)");
    }
    const Pixel middle = pixelOf(centre.abc());
    const std::array<HexVertex, 6> corners = centre.vertices();
    std::array<Pixel, 6> out;
    for (std::size_t i = 0; i < out.size(); ++i) {
      const Pixel corner = pixelOf(corners[i].abc());
      out[i] = Pixel{middle.x + (1.0 - inset) * (corner.x - middle.x),
                     middle.y + (1.0 - inset) * (corner.y - middle.y)};
    }
    return out;
  }

  HexId
  Grid::renderedId(GridIndex index) const
  {
    const int printedCol = spec_.colStart + index.col * spec_.colStep;
    const int printedRow = spec_.rowStart + index.row * spec_.rowStep;
    return HexId{format_.render(printedCol, printedRow)};
  }

  std::optional<GridIndex>
  Grid::locate(const HexId& id) const
  {
    for (int col = 0; col < spec_.cols; ++col) {
      for (int row = 0; row < spec_.rows; ++row) {
        const GridIndex index{col, row};
        if (renderedId(index) == id) {
          return index;
        }
      }
    }
    return std::nullopt;
  }

  // "A1-A9 B1-B3": whitespace-separated tokens, each a single printed id or an inclusive range.
  std::vector<HexId>
  Grid::clippedIds() const
  {
    std::vector<HexId> out;
    std::istringstream tokens(spec_.clip);
    std::string token;
    while (tokens >> token) {
      const std::size_t dash = token.find('-');
      if (std::string::npos == dash) {
        out.push_back(HexId{token});
        continue;
      }
      const std::vector<HexId> range =
          expandRange(HexId{token.substr(0, dash)}, HexId{token.substr(dash + 1)});
      out.insert(out.end(), range.begin(), range.end());
    }
    return out;
  }

  Abc
  Grid::offsetFor(const GridSpec& other) const
  {
    if (other.orientation != spec_.orientation) {
      throw std::invalid_argument("Grid '" + other.id + "': orientation differs from the lattice of '" +
                                  spec_.id + "'");
    }
    if (1e-9 * spec_.size < std::abs(other.size - spec_.size)) {
      throw std::invalid_argument("Grid '" + other.id + "': size " + std::to_string(other.size) +
                                  " differs from the lattice of '" + spec_.id + "', size " +
                                  std::to_string(spec_.size));
    }
    const Pixel abcOrigin = abcOriginPixel(other);
    const HexCentre nearest = nearestCentre(abcOrigin);
    const Pixel onLattice = pixelOf(nearest.abc());
    const double off = std::hypot(abcOrigin.x - onLattice.x, abcOrigin.y - onLattice.y);
    if (kLatticeTolerance * spec_.size < off) {
      throw std::invalid_argument("Grid '" + other.id + "': origin is " + std::to_string(off) +
                                  " pixels off the lattice of '" + spec_.id + "', more than " +
                                  std::to_string(kLatticeTolerance) + " of size " +
                                  std::to_string(spec_.size));
    }
    return nearest.abc();
  }

  std::vector<HexId>
  Grid::expandRange(const HexId& from, const HexId& to) const
  {
    const std::optional<GridIndex> a = locate(from);
    const std::optional<GridIndex> b = locate(to);
    if (!a.has_value() || !b.has_value()) {
      throw std::invalid_argument("Grid '" + spec_.id + "': range " + from.text + "-" + to.text +
                                  " names an id this grid does not print");
    }
    const int colLo = a->col < b->col ? a->col : b->col;
    const int colHi = a->col < b->col ? b->col : a->col;
    const int rowLo = a->row < b->row ? a->row : b->row;
    const int rowHi = a->row < b->row ? b->row : a->row;
    std::vector<HexId> out;
    for (int row = rowLo; row <= rowHi; ++row) {
      for (int col = colLo; col <= colHi; ++col) {
        out.push_back(renderedId(GridIndex{col, row}));
      }
    }
    return out;
  }

}  // namespace HexCoord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
