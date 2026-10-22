// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "ToyBoard.h"

#include <gtest/gtest.h>

#include <optional>
#include <span>
#include <stdexcept>

namespace {

  std::optional<HexSearch::CostHalves>
  oneEach(HexSearch::NodeIndex, std::size_t)
  {
    return HexSearch::CostHalves{1};
  }

}  // namespace

TEST(ScratchTest, FieldOutlivingItsSearchThrows)
{
  const HexModel::Board board = ToyBoard::build();
  const HexSearch::HexAdjacencyGraph graph = ToyBoard::openGraph(board);
  HexSearch::SearchScratch scratch;

  const HexSearch::Source source{ToyBoard::node(board, "C3"), HexSearch::CostHalves::zero()};
  const std::span<const HexSearch::Source> sources(&source, 1);

  const HexSearch::Field<HexSearch::CostHalves> first = HexSearch::Algorithms::dijkstraBounded(
      graph, scratch, sources, &oneEach, HexSearch::CostHalves{2}, nullptr);
  EXPECT_TRUE(first.reachedP(source.node));

  const HexSearch::Field<HexSearch::CostHalves> second = HexSearch::Algorithms::dijkstraBounded(
      graph, scratch, sources, &oneEach, HexSearch::CostHalves{1}, nullptr);
  EXPECT_TRUE(second.reachedP(source.node));

  EXPECT_THROW((void)first.reachedP(source.node), std::invalid_argument);
  EXPECT_THROW((void)first.reached(), std::invalid_argument);
}

TEST(ScratchTest, DefaultFieldThrows)
{
  const HexSearch::Field<HexSearch::CostHalves> nothing;
  EXPECT_THROW((void)nothing.reachedP(0), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
