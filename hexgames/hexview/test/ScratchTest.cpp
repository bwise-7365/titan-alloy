// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexview/Scratch.h"

#include <gtest/gtest.h>

#include <cmath>
#include <stdexcept>

namespace {

  using HexCoord::Pixel;
  using HexView::scratch;
  using HexView::ScratchSpec;

  const std::vector<Pixel> kLine{Pixel{0.0, 0.0}, Pixel{100.0, 0.0}};
  const std::vector<Pixel> kBent{Pixel{0.0, 0.0}, Pixel{100.0, 0.0}, Pixel{100.0, 100.0}};

  ScratchSpec
  spec(double roughness, double smoothing = 0.95, std::uint64_t seed = 7)
  {
    return ScratchSpec{roughness, smoothing, 20.0, 10, seed};
  }

  double
  maxDeviationFromXAxis(const std::vector<Pixel>& pts)
  {
    double m = 0.0;
    for (const Pixel& p : pts) {
      m = std::max(m, std::fabs(p.y));
    }
    return m;
  }

}  // namespace

TEST(ScratchTest, RoughnessZeroIsTheLineItself)
{
  const std::vector<Pixel> out = scratch(kLine, spec(0.0));
  ASSERT_EQ(2u, out.size());
  EXPECT_DOUBLE_EQ(0.0, out[0].x);
  EXPECT_DOUBLE_EQ(100.0, out[1].x);
}

TEST(ScratchTest, EndsArePinnedAndInteriorWobbles)
{
  const std::vector<Pixel> out = scratch(kLine, spec(0.5, 0.5));
  ASSERT_LE(3u, out.size());
  EXPECT_NEAR(0.0, out.front().y, 1e-12);
  EXPECT_NEAR(0.0, out.back().y, 1e-12);
  EXPECT_NEAR(0.0, out.front().x, 1e-12);
  EXPECT_NEAR(100.0, out.back().x, 1e-12);
  EXPECT_LT(0.0, maxDeviationFromXAxis(out));
  // never further from the line than half a unit at roughness 1, so a quarter here
  EXPECT_GE(0.25 * 20.0 + 1e-9, maxDeviationFromXAxis(out));
}

TEST(ScratchTest, SmoothingOneIsNearlyStraightAgain)
{
  // the fixed point at smoothing 1 is the straight line; irrgo's 250 Gauss-Seidel sweeps stop at the
  // change tolerance short of it, so "straight" here means under a pixel where the raw noise is +/-10
  const double raw = maxDeviationFromXAxis(scratch(kLine, spec(1.0, 0.0)));
  const double smooth = maxDeviationFromXAxis(scratch(kLine, spec(1.0, 1.0)));
  EXPECT_LT(5.0, raw);
  EXPECT_LT(smooth, 1.0);
}

TEST(ScratchTest, MoreSmoothingMeansLessWobble)
{
  const double rough = maxDeviationFromXAxis(scratch(kLine, spec(0.5, 0.2)));
  const double smooth = maxDeviationFromXAxis(scratch(kLine, spec(0.5, 0.95)));
  EXPECT_LT(smooth, rough);
}

TEST(ScratchTest, SameSeedSameLine)
{
  const std::vector<Pixel> a = scratch(kBent, spec(0.4));
  const std::vector<Pixel> b = scratch(kBent, spec(0.4));
  const std::vector<Pixel> c = scratch(kBent, spec(0.4, 0.95, 8));
  ASSERT_EQ(a.size(), b.size());
  bool differsP = false;
  for (std::size_t i = 0; i < a.size(); ++i) {
    EXPECT_DOUBLE_EQ(a[i].x, b[i].x);
    EXPECT_DOUBLE_EQ(a[i].y, b[i].y);
    differsP = differsP || a[i].x != c[i].x || a[i].y != c[i].y;
  }
  EXPECT_TRUE(differsP);
}

TEST(ScratchTest, SamplesFollowTheWholePolyline)
{
  // a bent line of length 200 at 10 samples per unit of 20 gives 101 samples, the corner included
  const std::vector<Pixel> out = scratch(kBent, spec(0.3));
  EXPECT_EQ(101u, out.size());
  EXPECT_NEAR(100.0, out.back().x, 1e-12);
  EXPECT_NEAR(100.0, out.back().y, 1e-12);
}

TEST(ScratchTest, BadSpecThrows)
{
  EXPECT_THROW((void)scratch(kLine, spec(1.5)), std::invalid_argument);
  EXPECT_THROW((void)scratch(kLine, ScratchSpec{0.5, 0.5, 0.0, 10, 1}), std::invalid_argument);
  EXPECT_THROW((void)scratch(kLine, ScratchSpec{0.5, 0.5, 20.0, 0, 1}), std::invalid_argument);
  EXPECT_THROW((void)scratch(std::vector<Pixel>{Pixel{0.0, 0.0}}, spec(0.5)), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
