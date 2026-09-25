// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexview/Scene.h"

#include <gtest/gtest.h>

#include <stdexcept>

namespace {

  using namespace HexView;

  Primitive
  square(double x, double y, double side, HitTag hit)
  {
    PathShape s;
    s.commands = {MoveTo{{x, y}}, LineTo{{x + side, y}}, LineTo{{x + side, y + side}},
                  LineTo{{x, y + side}}, ClosePath{}};
    s.fill = Fill{Color{10, 20, 30, 255}, 1.0};
    return Primitive{s, hit};
  }

  HexModel::HexIndex
  hexOf(const HitTag& hit)
  {
    return std::get<HexHit>(hit).hex;
  }

  TEST(SceneTest, LayersKeepInsertionOrder)
  {
    Scene scene(100, 100);
    for (std::uint32_t k = 0; k < 5; ++k) {
      scene.add(Layer::Terrain, square(k, 0, 1, HexHit{HexModel::HexIndex{k}}));
    }
    ASSERT_EQ(5u, scene.layer(Layer::Terrain).size());
    for (std::uint32_t k = 0; k < 5; ++k) {
      EXPECT_EQ(k, hexOf(scene.layer(Layer::Terrain)[k].hit).value);
    }
    EXPECT_TRUE(scene.layer(Layer::Units).empty());
  }

  TEST(SceneTest, HitAtPrefersTheTopLayer)
  {
    Scene scene(100, 100);
    scene.add(Layer::Terrain, square(0, 0, 50, HexHit{HexModel::HexIndex{1}}));
    scene.add(Layer::Units, square(10, 10, 10, UnitHit{HexModel::UnitId{7}}));
    scene.add(Layer::Terrain, square(0, 0, 50, HexHit{HexModel::HexIndex{2}}));
    EXPECT_EQ(7u, std::get<UnitHit>(scene.hitAt({15, 15})).unit.value);
    // within one layer the later primitive is on top
    EXPECT_EQ(2u, hexOf(scene.hitAt({40, 40})).value);
  }

  TEST(SceneTest, HitAtOffEverythingIsNoHit)
  {
    Scene scene(100, 100);
    scene.add(Layer::Terrain, square(0, 0, 10, HexHit{HexModel::HexIndex{1}}));
    EXPECT_TRUE(std::holds_alternative<NoHit>(scene.hitAt({50, 50})));
  }

  TEST(SceneTest, RefusesNonPositiveSize)
  {
    EXPECT_THROW(Scene(0, 10), std::invalid_argument);
    EXPECT_THROW(Scene(10, -1), std::invalid_argument);
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
