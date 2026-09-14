// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Sheet grids: offset indices, printed ids, pixels and the shared lattice of a two-map sheet.
//
// The reference pixels in kTrc, kDdat, kDsWest, kDsEast and kPgg were printed by
// hexcoord/test/reference-centres.py, which runs map_graphics/xml/hexsheet2svg.py's own Grid class
// over the four sheets. Regenerate them with that script if the renderer's geometry ever changes.
// ----------------------------------------------
#include "hexcoord/Grid.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

  using HexCoord::abcOfIndex;
  using HexCoord::Abc;
  using HexCoord::Grid;
  using HexCoord::GridIndex;
  using HexCoord::GridSpec;
  using HexCoord::HexCentre;
  using HexCoord::HexId;
  using HexCoord::indexOfAbc;
  using HexCoord::Orientation;
  using HexCoord::Parity;
  using HexCoord::Pixel;
  using HexCoord::Qrs;

  constexpr int kIterations = 400;
  constexpr double kSqrt3 = 1.7320508075688772;
  constexpr double kPixelTolerance = 0.01;

  struct Sample {
    const char* id;
    int col;
    int row;
    double x;
    double y;
  };

  // the-russian-campaign.xml
  GridSpec
  trcSpec()
  {
    GridSpec spec;
    spec.id = "main";
    spec.orientation = Orientation::Pointy;
    spec.offset = Parity::Odd;
    spec.cols = 33;
    spec.rows = 43;
    spec.size = 20.65;
    spec.ox = 22.27;
    spec.oy = 155.40;
    spec.idFormat = "{rowletter}{col}";
    spec.colStart = 33;
    spec.colStep = -1;
    spec.rowStart = 1;
    spec.rowStep = 1;
    return spec;
  }

  // d-day-at-tarawa.xml
  GridSpec
  ddatSpec()
  {
    GridSpec spec;
    spec.id = "main";
    spec.orientation = Orientation::Pointy;
    spec.offset = Parity::Odd;
    spec.cols = 42;
    spec.rows = 25;
    spec.size = 24.90;
    spec.ox = 8.59;
    spec.oy = 222.10;
    spec.idFormat = "{row:02}{col:02}";
    spec.colStart = 2;
    spec.rowStart = 25;
    spec.rowStep = -1;
    return spec;
  }

  // dai-senso.xml, the western map
  GridSpec
  dsWestSpec()
  {
    GridSpec spec;
    spec.id = "west";
    spec.orientation = Orientation::Pointy;
    spec.offset = Parity::Odd;
    spec.cols = 27;
    spec.rows = 53;
    spec.size = 40.95;
    spec.ox = 62.54;
    spec.oy = -8.05;
    spec.idFormat = "w{row:02}{col:02}";
    spec.colStart = 1;
    spec.rowStart = 61;
    spec.rowStep = -1;
    return spec;
  }

  // dai-senso.xml, the eastern map: the same lattice, 27 columns further east
  GridSpec
  dsEastSpec()
  {
    GridSpec spec = dsWestSpec();
    spec.id = "east";
    spec.cols = 29;
    spec.ox = 1977.63;
    spec.idFormat = "e{row:02}{col:02}";
    return spec;
  }

  // panzergruppe-guderian.xml, the one flat-topped sheet
  GridSpec
  pggSpec()
  {
    GridSpec spec;
    spec.id = "main";
    spec.orientation = Orientation::Flat;
    spec.offset = Parity::Odd;
    spec.cols = 56;
    spec.rows = 31;
    spec.size = 61.65;
    spec.ox = 138.13;
    spec.oy = 125.02;
    spec.idFormat = "{col:02}{row:02}";
    spec.colStart = 1;
    spec.rowStart = 1;
    return spec;
  }

  const Sample kTrc[] = {{"A33", 0, 0, 22.270000, 155.400000},
                         {"KK19", 14, 36, 523.005888, 1270.500000},
                         {"KK20", 13, 36, 487.239039, 1270.500000},
                         {"QQ1", 32, 42, 1166.809174, 1456.350000}};
  const Sample kDdat[] = {{"2336", 34, 2, 1474.944214, 296.800000},
                          {"2502", 0, 0, 8.590000, 222.100000},
                          {"0143", 41, 24, 1776.840669, 1118.500000}};
  const Sample kDsWest[] = {{"w5227", 26, 9, 1942.118235, 544.775000},
                            {"w6101", 0, 0, 62.540000, -8.050000},
                            {"w5201", 0, 9, 98.003740, 544.775000}};
  const Sample kDsEast[] = {{"e3411", 10, 27, 2722.368546, 1650.425000},
                            {"e5201", 0, 9, 2013.093740, 544.775000},
                            {"e6101", 0, 0, 1977.630000, -8.050000}};
  const Sample kPgg[] = {{"0101", 0, 0, 138.130000, 125.020000},
                         {"0109", 0, 8, 138.130000, 979.267458},
                         {"5631", 55, 30, 5224.255000, 3381.838435}};

  // hexsheet2svg.py's Grid.centre(), transcribed, for the grids no sheet happens to use.
  Pixel
  rendererCentre(const GridSpec& spec, GridIndex index)
  {
    const bool flatP = Orientation::Flat == spec.orientation;
    const int along = flatP ? index.col : index.row;
    const bool shiftedP = Parity::Odd == spec.offset ? 1 == HexCoord::iMod(along, 2)
                                                     : 0 == HexCoord::iMod(along, 2);
    const double jog = shiftedP ? kSqrt3 / 2.0 * spec.size : 0.0;
    if (flatP) {
      return Pixel{spec.ox + index.col * 1.5 * spec.size,
                   spec.oy + index.row * kSqrt3 * spec.size + jog};
    }
    return Pixel{spec.ox + index.col * kSqrt3 * spec.size + jog,
                 spec.oy + index.row * 1.5 * spec.size};
  }

  GridSpec
  smallSpec(Orientation orientation, Parity offset)
  {
    GridSpec spec;
    spec.id = "small";
    spec.orientation = orientation;
    spec.offset = offset;
    spec.cols = 7;
    spec.rows = 7;
    spec.size = 10.0;
    spec.ox = 100.0;
    spec.oy = 200.0;
    spec.idFormat = "{col:02}{row:02}";
    return spec;
  }

  void
  checkSamples(const Grid& grid, const Sample* samples, std::size_t count)
  {
    for (std::size_t i = 0; i < count; ++i) {
      const Sample& sample = samples[i];
      const std::optional<GridIndex> found = grid.find(HexId{sample.id});
      ASSERT_TRUE(found.has_value()) << sample.id;
      EXPECT_EQ(sample.col, found->col) << sample.id;
      EXPECT_EQ(sample.row, found->row) << sample.id;
      const Pixel pixel = grid.pixelOf(grid.centreOf(*found));
      EXPECT_NEAR(sample.x, pixel.x, kPixelTolerance) << sample.id;
      EXPECT_NEAR(sample.y, pixel.y, kPixelTolerance) << sample.id;
    }
    return;
  }

}  // namespace

TEST(GridTest, RowClmRoundTripFlat)
{
  std::mt19937_64 rng(20260912u);
  std::uniform_int_distribution<int> rows(-10, 20);
  std::uniform_int_distribution<int> cols(-10, 30);
  for (int i = 0; i < kIterations; ++i) {
    const GridIndex index{cols(rng), rows(rng)};
    const Abc v = abcOfIndex(index, Orientation::Flat, Parity::Odd);
    const HexCentre centre{v};
    EXPECT_EQ(index, indexOfAbc(centre, Orientation::Flat, Parity::Odd));

    // The same lattice point written with an arbitrary diagonal shift reduces to the same cell.
    const int d = rows(rng);
    const HexCentre shifted{Abc{v.a() + d, v.b() + d, v.c() + d}};
    EXPECT_EQ(index, indexOfAbc(shifted, Orientation::Flat, Parity::Odd));
  }
}

TEST(GridTest, FlatDecompositionEven)
{
  std::mt19937_64 rng(20260912u);
  std::uniform_int_distribution<int> draw(-10, 20);
  for (int i = 0; i < kIterations; ++i) {
    const int n = draw(rng);
    const int row = draw(rng);
    const int clm = 2 * n;
    const Abc v = abcOfIndex(GridIndex{clm, row}, Orientation::Flat, Parity::Odd);
    EXPECT_EQ(Abc(3 * n, -row, row), v);
    EXPECT_EQ(0, v.hvCode());  // hex centre of an even column

    // tricoord's testHexCenter decomposition, on the canonical representative.
    const int d = (v.b() + v.c()) / 2;
    const int backRow = (v.c() - v.b()) / 2;
    EXPECT_EQ(v.b(), d - backRow);
    EXPECT_EQ(v.c(), d + backRow);
    const int backN = (v.a() - d) / 3;
    EXPECT_EQ(v.a(), 3 * backN + d);
    EXPECT_EQ(row, backRow);
    EXPECT_EQ(clm, 2 * backN);
  }
}

TEST(GridTest, FlatDecompositionOdd)
{
  std::mt19937_64 rng(20260912u);
  std::uniform_int_distribution<int> draw(-10, 20);
  for (int i = 0; i < kIterations; ++i) {
    const int n = draw(rng);
    const int row = draw(rng);
    const int clm = 2 * n + 1;
    const Abc v = abcOfIndex(GridIndex{clm, row}, Orientation::Flat, Parity::Odd);
    EXPECT_EQ(Abc(3 * n + 1, -(row + 1), row), v);
    EXPECT_EQ(3, v.hvCode());  // hex centre of an odd column

    const int d = (v.b() + v.c() + 1) / 2;
    const int backRow = (v.c() - (v.b() + 1)) / 2;
    EXPECT_EQ(v.b(), d - (backRow + 1));
    EXPECT_EQ(v.c(), d + backRow);
    const int backN = (v.a() - (d + 1)) / 3;
    EXPECT_EQ(v.a(), 3 * backN + d + 1);
    EXPECT_EQ(row, backRow);
    EXPECT_EQ(clm, 2 * backN + 1);
  }
}

TEST(GridTest, RowClmQrsRowClm)
{
  std::mt19937_64 rng(20260912u);
  std::uniform_int_distribution<int> rows(-10, 20);
  std::uniform_int_distribution<int> cols(-10, 30);
  for (int i = 0; i < kIterations; ++i) {
    const GridIndex index{cols(rng), rows(rng)};
    const HexCentre centre{abcOfIndex(index, Orientation::Flat, Parity::Odd)};
    const Qrs w = centre.qrs();
    EXPECT_EQ(centre, HexCentre::fromQrs(w));
    EXPECT_EQ(index, indexOfAbc(HexCentre::fromQrs(w), Orientation::Flat, Parity::Odd));
    // A whole hex step changes exactly one cell of the offset frame.
    const HexCentre east = centre + HexCoord::step(HexCoord::Direction::D3);
    EXPECT_EQ(1, HexCoord::hexDist(centre, east));
    EXPECT_NE(index, indexOfAbc(east, Orientation::Flat, Parity::Odd));
  }
}

TEST(GridTest, PointyRoundTrip)
{
  for (const Parity offset : {Parity::Odd, Parity::Even}) {
    const Grid grid{smallSpec(Orientation::Pointy, offset)};
    for (int row = 0; row < grid.spec().rows; ++row) {
      for (int col = 0; col < grid.spec().cols; ++col) {
        const GridIndex index{col, row};
        const HexCentre centre = grid.centreOf(index);
        const std::optional<GridIndex> back = grid.indexOf(centre);
        ASSERT_TRUE(back.has_value());
        EXPECT_EQ(index, *back);
      }
    }
    EXPECT_THROW(grid.centreOf(GridIndex{7, 0}), std::invalid_argument);
    EXPECT_THROW(grid.centreOf(GridIndex{0, -1}), std::invalid_argument);
    EXPECT_EQ(std::size_t{49}, grid.ids().size());
  }
}

TEST(GridTest, PixelLayoutMatchesRendererForBothParities)
{
  for (const Orientation orientation : {Orientation::Flat, Orientation::Pointy}) {
    for (const Parity offset : {Parity::Odd, Parity::Even}) {
      const GridSpec spec = smallSpec(orientation, offset);
      const Grid grid{spec};
      for (int row = 0; row < spec.rows; ++row) {
        for (int col = 0; col < spec.cols; ++col) {
          const Pixel mine = grid.pixelOf(grid.centreOf(GridIndex{col, row}));
          const Pixel theirs = rendererCentre(spec, GridIndex{col, row});
          EXPECT_NEAR(theirs.x, mine.x, kPixelTolerance) << col << ", " << row;
          EXPECT_NEAR(theirs.y, mine.y, kPixelTolerance) << col << ", " << row;
        }
      }
    }
  }
}

// doc/hex-ABC-offset-odd-down.svg and hex-ABC-offset-even-down.svg: the same lattice and the same
// ABC origin; on the even grid the origin is the unprinted hex just before cell (0, 0).
TEST(GridTest, EvenGridKeepsTheAbcOrigin)
{
  const double half = kSqrt3 / 2.0 * 10.0;
  for (const Orientation orientation : {Orientation::Flat, Orientation::Pointy}) {
    const bool flatP = Orientation::Flat == orientation;
    const GridSpec spec = smallSpec(orientation, Parity::Even);
    const Grid even{spec};
    const Grid odd{smallSpec(orientation, Parity::Odd)};
    const HexCentre origin{Abc{}};

    EXPECT_EQ(origin, odd.centreOf(GridIndex{0, 0}));
    EXPECT_FALSE(even.indexOf(origin).has_value());
    EXPECT_EQ(1, HexCoord::hexDist(origin, even.centreOf(GridIndex{0, 0})));

    const Pixel atOrigin = even.pixelOf(origin);
    EXPECT_NEAR(flatP ? spec.ox : spec.ox - half, atOrigin.x, kPixelTolerance);
    EXPECT_NEAR(flatP ? spec.oy - half : spec.oy, atOrigin.y, kPixelTolerance);

    std::mt19937_64 rng(20260914u);
    std::uniform_int_distribution<int> draw(-10, 20);
    for (int i = 0; i < kIterations; ++i) {
      const GridIndex index{draw(rng), draw(rng)};
      const HexCentre centre{abcOfIndex(index, orientation, Parity::Even)};
      EXPECT_EQ(index, indexOfAbc(centre, orientation, Parity::Even));
    }
  }
}

TEST(GridTest, PixelMatchesRendererFourSheets)
{
  const Grid trc{trcSpec()};
  checkSamples(trc, kTrc, std::size(kTrc));
  const Grid ddat{ddatSpec()};
  checkSamples(ddat, kDdat, std::size(kDdat));
  const Grid west{dsWestSpec()};
  checkSamples(west, kDsWest, std::size(kDsWest));
  const Grid east{dsEastSpec()};
  checkSamples(east, kDsEast, std::size(kDsEast));
  const Grid pgg{pggSpec()};
  checkSamples(pgg, kPgg, std::size(kPgg));

  // KK19 and KK20 are printed side by side, and are neighbours on the lattice.
  const HexCentre kk19 = trc.centreOf(*trc.find(HexId{"KK19"}));
  const HexCentre kk20 = trc.centreOf(*trc.find(HexId{"KK20"}));
  EXPECT_EQ(1, HexCoord::hexDist(kk19, kk20));

  // Every corner of a hex is one circumradius from its centre, and the polygon is inset by scaling.
  const std::array<Pixel, 6> corners = pgg.polygon(pgg.centreOf(GridIndex{3, 3}));
  const Pixel middle = pgg.pixelOf(pgg.centreOf(GridIndex{3, 3}));
  for (const Pixel& corner : corners) {
    EXPECT_NEAR(pggSpec().size, std::hypot(corner.x - middle.x, corner.y - middle.y), 1e-9);
  }
  const std::array<Pixel, 6> shrunk = pgg.polygon(pgg.centreOf(GridIndex{3, 3}), 0.25);
  for (const Pixel& corner : shrunk) {
    EXPECT_NEAR(0.75 * pggSpec().size, std::hypot(corner.x - middle.x, corner.y - middle.y), 1e-9);
  }
  EXPECT_THROW(pgg.polygon(pgg.centreOf(GridIndex{3, 3}), 1.0), std::invalid_argument);
}

TEST(GridTest, HexAtInvertsPixelOf)
{
  std::mt19937_64 rng(20260912u);
  std::uniform_real_distribution<double> angle(0.0, 6.283185307179586);
  std::uniform_real_distribution<double> radius(0.0, 0.4);
  for (const Orientation orientation : {Orientation::Flat, Orientation::Pointy}) {
    for (const Parity offset : {Parity::Odd, Parity::Even}) {
      const GridSpec spec = smallSpec(orientation, offset);
      const Grid grid{spec};
      for (int row = 0; row < spec.rows; ++row) {
        for (int col = 0; col < spec.cols; ++col) {
          const HexCentre centre = grid.centreOf(GridIndex{col, row});
          const Pixel middle = grid.pixelOf(centre);
          const double a = angle(rng);
          const double r = radius(rng) * spec.size;
          const Pixel jittered{middle.x + r * std::cos(a), middle.y + r * std::sin(a)};
          const std::optional<HexCentre> found = grid.hexAt(jittered);
          ASSERT_TRUE(found.has_value()) << col << ", " << row;
          EXPECT_EQ(centre, *found) << col << ", " << row;
        }
      }
      EXPECT_FALSE(grid.hexAt(Pixel{spec.ox - 10.0 * spec.size, spec.oy}).has_value());
      EXPECT_FALSE(grid.hexAt(Pixel{spec.ox, spec.oy + 20.0 * spec.size}).has_value());
    }
  }

  // The same on a real sheet, at its own scale.
  const Grid pgg{pggSpec()};
  for (const Sample& sample : kPgg) {
    const HexCentre centre = pgg.centreOf(GridIndex{sample.col, sample.row});
    const std::optional<HexCentre> found = pgg.hexAt(Pixel{sample.x + 1.0, sample.y - 2.0});
    ASSERT_TRUE(found.has_value()) << sample.id;
    EXPECT_EQ(centre, *found) << sample.id;
  }
}

TEST(GridTest, ClipRemovesIds)
{
  GridSpec spec = smallSpec(Orientation::Pointy, Parity::Odd);
  spec.idFormat = "{colletter}{row}";
  spec.clip = "A1-A3 B7";
  const Grid grid{spec};
  EXPECT_EQ(std::size_t{49 - 4}, grid.ids().size());
  for (const char* gone : {"A1", "A2", "A3", "B7"}) {
    EXPECT_FALSE(grid.find(HexId{gone}).has_value()) << gone;
    EXPECT_EQ(grid.ids().end(), std::find(grid.ids().begin(), grid.ids().end(), HexId{gone}));
  }
  EXPECT_TRUE(grid.find(HexId{"A4"}).has_value());
  EXPECT_TRUE(grid.find(HexId{"B6"}).has_value());

  // A clipped cell still has a place on the lattice, but no index and no printed id.
  const HexCentre clipped = grid.centreOf(GridIndex{0, 0});
  EXPECT_FALSE(grid.indexOf(clipped).has_value());
  EXPECT_FALSE(grid.hexAt(grid.pixelOf(clipped)).has_value());
  EXPECT_THROW(grid.idOf(GridIndex{0, 0}), std::invalid_argument);
  EXPECT_EQ(HexId{"A4"}, grid.idOf(GridIndex{0, 3}));

  const std::vector<HexId> range = grid.expandRange(HexId{"A1"}, HexId{"A3"});
  EXPECT_EQ(std::size_t{3}, range.size());
  EXPECT_THROW(grid.expandRange(HexId{"A1"}, HexId{"ZZ9"}), std::invalid_argument);
}

TEST(GridTest, TwoGridsShareOneLattice)
{
  const Grid west{dsWestSpec()};
  const Grid east{dsEastSpec(), west};
  EXPECT_EQ(Abc(27, 0, -27), east.latticeOffset());
  EXPECT_EQ(Abc{}, west.latticeOffset());

  // w5227 is the last hex of the western map's row; e5201 is the first of the eastern map's.
  const HexCentre seamWest = west.centreOf(*west.find(HexId{"w5227"}));
  const HexCentre seamEast = east.centreOf(*east.find(HexId{"e5201"}));
  EXPECT_EQ(1, HexCoord::hexDist(seamWest, seamEast));
  EXPECT_EQ(seamEast, seamWest.neighbour(HexCoord::Direction::D1));  // due east, pointy-topped
  EXPECT_FALSE(west.indexOf(seamEast).has_value());                  // off the western map
  EXPECT_FALSE(east.indexOf(seamWest).has_value());

  // The two maps agree about where a shared hex is drawn, to within the fit of the sheet XML.
  const Pixel byWest = west.pixelOf(seamEast);
  const Pixel byEast = east.pixelOf(seamEast);
  EXPECT_NEAR(byWest.x, byEast.x, HexCoord::kLatticeTolerance * dsWestSpec().size);
  EXPECT_NEAR(byWest.y, byEast.y, HexCoord::kLatticeTolerance * dsWestSpec().size);

  // Half a hex out is not on the lattice at all.
  GridSpec adrift = dsEastSpec();
  adrift.ox += 0.5 * adrift.size;
  EXPECT_THROW(Grid(adrift, west), std::invalid_argument);
  GridSpec tilted = dsEastSpec();
  tilted.orientation = Orientation::Flat;
  EXPECT_THROW(Grid(tilted, west), std::invalid_argument);
  GridSpec resized = dsEastSpec();
  resized.size = 41.0;
  EXPECT_THROW(Grid(resized, west), std::invalid_argument);

  // A lattice offset that is a vertex, not a hex, is not a grid origin.
  EXPECT_THROW(Grid(dsEastSpec(), HexCoord::AVec), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
