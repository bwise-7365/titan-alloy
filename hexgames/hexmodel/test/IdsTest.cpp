// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/Ids.h"

#include <gtest/gtest.h>

#include <type_traits>

TEST(IdsTest, DistinctTagsDoNotConvert)
{
  static_assert(!std::is_convertible_v<HexModel::UnitId, HexModel::HexIndex>);
  static_assert(!std::is_convertible_v<HexModel::HexIndex, HexModel::UnitId>);
  static_assert(!std::is_convertible_v<HexModel::RuleId, HexModel::CounterId>);

  const HexModel::UnitId a{3};
  const HexModel::UnitId b{3};
  const HexModel::UnitId c{4};
  EXPECT_EQ(a, b);
  EXPECT_NE(a, c);
  EXPECT_LT(a, c);

  const HexModel::RuleId r1{"zoc"};
  const HexModel::RuleId r2{"zoc"};
  const HexModel::RuleId r3{"aaa"};
  EXPECT_EQ(r1, r2);
  EXPECT_LT(r3, r1);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
