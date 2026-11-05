// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Regions, the fringe and the allocation, on the toy board at a cost of one a step.
// ----------------------------------------------
#include "ToyBoard.h"

#include "hexai/Seize.h"

#include <gtest/gtest.h>

#include <vector>

namespace {

  using HexAi::Force;
  using HexAi::Piece;
  using HexAi::Target;
  using HexModel::HexIndex;

  std::optional<int>
  oneStep(HexIndex, HexModel::Direction)
  {
    return 1;
  }

  // A cost given by a table of force positions against targets.
  HexAi::TargetCost
  byDistance(const HexModel::Board& board, std::vector<Target> targets)
  {
    return [&board, targets = std::move(targets)](std::size_t target, HexIndex hex) -> std::optional<int> {
      return board.distance(targets[target].hex, hex);
    };
  }

}  // namespace

TEST(SeizeTest, ACostFieldDrawsRegionsAroundItsSources)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<HexIndex> sources{ToyBoard::at(board, "A1"), ToyBoard::at(board, "E5")};
  const HexAi::CostField field(board, sources, &oneStep);
  EXPECT_EQ(0, field.cost(sources[0]));
  EXPECT_EQ(0u, field.sourceOf(sources[0]));
  EXPECT_EQ(1u, field.sourceOf(sources[1]));
  const HexIndex nearFirst = ToyBoard::at(board, "A2");
  EXPECT_EQ(1, field.cost(nearFirst));
  EXPECT_EQ(0u, field.sourceOf(nearFirst));
}

TEST(SeizeTest, ACostIsThatOfTravellingTowardTheSource)
{
  const HexModel::Board board = ToyBoard::build();
  const HexIndex target = ToyBoard::at(board, "C3");
  const HexIndex dear = ToyBoard::at(board, "C4");
  const std::vector<HexIndex> sources{target};
  // Stepping off C4 costs ten. Anyone standing there pays it to reach C3; no one else does.
  const HexAi::CostField field(board, sources, [&](HexIndex from, HexModel::Direction) -> std::optional<int> { return from == dear ? 10 : 1; });
  EXPECT_EQ(10, field.cost(dear));
  EXPECT_EQ(1, field.cost(ToyBoard::at(board, "C2")));
}

TEST(SeizeTest, TheFringeIsTheRegionsAUnitStandsIn)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<HexIndex> unheld{ToyBoard::at(board, "A1"), ToyBoard::at(board, "C3"), ToyBoard::at(board, "E5")};
  const std::vector<HexIndex> positions{ToyBoard::at(board, "A2"), ToyBoard::at(board, "C4")};
  EXPECT_EQ((std::vector<HexIndex>{unheld[0], unheld[1]}), HexAi::occupiedRegions(board, unheld, positions, &oneStep));
  // Once A1 and C3 are held the regions are redrawn, and the same units stand in E5's.
  const std::vector<HexIndex> last{unheld[2]};
  EXPECT_EQ(last, HexAi::occupiedRegions(board, last, positions, &oneStep));
}

TEST(SeizeTest, StrengthIsCountedByRegion)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<HexIndex> locations{ToyBoard::at(board, "A1"), ToyBoard::at(board, "E5")};
  const std::vector<Piece> pieces{{ToyBoard::at(board, "A2"), 3.0}, {ToyBoard::at(board, "E4"), 4.0}, {ToyBoard::at(board, "D5"), 1.0}};
  EXPECT_EQ((std::vector<double>{3.0, 5.0}), HexAi::strengthByRegion(board, locations, pieces, &oneStep));
}

TEST(SeizeTest, ForcesGoInProportionToTheEnemyClosestFirst)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<Target> targets{{ToyBoard::at(board, "A1"), 3.0, true}, {ToyBoard::at(board, "E5"), 1.0, true}};
  // Four equal forces near A1: three go there, and the last, the one farthest from it, goes to E5.
  const std::vector<Force> forces{{ToyBoard::at(board, "A2"), 1.0},
                                  {ToyBoard::at(board, "B1"), 1.0},
                                  {ToyBoard::at(board, "B2"), 1.0},
                                  {ToyBoard::at(board, "C3"), 1.0}};
  const auto out = HexAi::allocate(forces, targets, 1.0, byDistance(board, targets));
  EXPECT_EQ(0u, out[0]);
  EXPECT_EQ(0u, out[1]);
  EXPECT_EQ(0u, out[2]);
  EXPECT_EQ(1u, out[3]);
}

TEST(SeizeTest, AnEmptyFringeLocationStillGetsAForce)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<Target> targets{{ToyBoard::at(board, "A1"), 10.0, true}, {ToyBoard::at(board, "E5"), 0.0, true}};
  const std::vector<Force> forces{{ToyBoard::at(board, "A2"), 1.0}, {ToyBoard::at(board, "A3"), 1.0}, {ToyBoard::at(board, "B3"), 1.0}};
  const auto out = HexAi::allocate(forces, targets, 1.0, byDistance(board, targets));
  int toEmpty = 0;
  for (const auto& target : out) {
    toEmpty += 1u == target ? 1 : 0;
  }
  EXPECT_EQ(1, toEmpty);
}

TEST(SeizeTest, AForceThatCannotMoveCountsFirstAgainstItsNearestTarget)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<Target> targets{{ToyBoard::at(board, "A1"), 1.0, true}, {ToyBoard::at(board, "E5"), 1.0, true}};
  // Two forces, one held fast next to A1. The other is nearer A1 too, but A1's half is already spent.
  const std::vector<Force> forces{{ToyBoard::at(board, "A2"), 1.0, true}, {ToyBoard::at(board, "B1"), 1.0, false}};
  const auto out = HexAi::allocate(forces, targets, 1.0, byDistance(board, targets));
  EXPECT_EQ(0u, out[1]);
  EXPECT_EQ(1u, out[0]);
}

TEST(SeizeTest, TheFringeShareDividesTheStrengthAndTheRearGoesByValue)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<Target> targets{{ToyBoard::at(board, "C3"), 1.0, true},
                                    {ToyBoard::at(board, "A1"), 3.0, false},
                                    {ToyBoard::at(board, "E5"), 1.0, false}};
  // Ten forces of one, half to the fringe. C3 takes five, A1 takes its three and a bit, E5 the rest.
  std::vector<Force> forces;
  for (const char* hex : {"C3", "C2", "C4", "B3", "D3"}) {
    forces.push_back({ToyBoard::at(board, hex), 1.0});
  }
  for (const char* hex : {"A1", "A2", "B1", "B2"}) {
    forces.push_back({ToyBoard::at(board, hex), 1.0});
  }
  forces.push_back({ToyBoard::at(board, "E5"), 1.0});
  const auto out = HexAi::allocate(forces, targets, 0.5, byDistance(board, targets));
  std::vector<int> counts(targets.size(), 0);
  for (const auto& target : out) {
    counts[*target] += 1;
  }
  EXPECT_EQ(5, counts[0]);
  EXPECT_EQ(4, counts[1]);
  EXPECT_EQ(1, counts[2]);
}

TEST(SeizeTest, AForceKeepsItsTargetWhileTheQuotaHasRoom)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<Target> targets{{ToyBoard::at(board, "A1"), 1.0, true}, {ToyBoard::at(board, "E5"), 1.0, true}};
  // Four forces beside A1. Without a memory, the nearest two go to A1 and the far two to E5.
  const std::vector<Force> forces{{ToyBoard::at(board, "A2"), 1.0},
                                  {ToyBoard::at(board, "B1"), 1.0},
                                  {ToyBoard::at(board, "B2"), 1.0},
                                  {ToyBoard::at(board, "C3"), 1.0}};
  const auto fresh = HexAi::allocate(forces, targets, 1.0, byDistance(board, targets));
  EXPECT_EQ(0u, fresh[0]);
  EXPECT_EQ(1u, fresh[3]);
  // Last time the first force was bound for E5, and the last for A1: each stays with its own.
  const std::vector<std::optional<std::size_t>> previous{1u, std::nullopt, std::nullopt, 0u};
  const auto kept = HexAi::allocate(forces, targets, 1.0, byDistance(board, targets), previous, 0.25);
  EXPECT_EQ(1u, kept[0]);
  EXPECT_EQ(0u, kept[3]);
}

TEST(SeizeTest, ATargetWithNoQuotaKeepsNoOne)
{
  const HexModel::Board board = ToyBoard::build();
  // A1 has been taken, so it is a rear target and the whole strength goes to the fringe.
  const std::vector<Target> targets{{ToyBoard::at(board, "A1"), 5.0, false}, {ToyBoard::at(board, "E5"), 1.0, true}};
  const std::vector<Force> forces{{ToyBoard::at(board, "A2"), 1.0}, {ToyBoard::at(board, "B1"), 1.0}};
  const std::vector<std::optional<std::size_t>> previous{0u, 0u};
  const auto out = HexAi::allocate(forces, targets, 1.0, byDistance(board, targets), previous, 0.25);
  EXPECT_EQ(1u, out[0]);
  EXPECT_EQ(1u, out[1]);
}

TEST(SeizeTest, ATargetGoesOverItsQuotaOnlyByTheMargin)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<Target> targets{{ToyBoard::at(board, "A1"), 1.0, true}, {ToyBoard::at(board, "E5"), 1.0, true}};
  // Eight of strength in all, so a quota of four each. A1 can keep five with a margin of 1/4, and no more.
  std::vector<Force> forces;
  std::vector<std::optional<std::size_t>> previous;
  for (const char* hex : {"A2", "A3", "B1", "B2", "B3", "C1", "C2"}) {
    forces.push_back({ToyBoard::at(board, hex), 1.0});
    previous.push_back(0u);
  }
  forces.push_back({ToyBoard::at(board, "E4"), 1.0});
  previous.push_back(1u);
  const auto out = HexAi::allocate(forces, targets, 1.0, byDistance(board, targets), previous, 0.25);
  int toFirst = 0;
  for (const auto& target : out) {
    toFirst += 0u == target ? 1 : 0;
  }
  EXPECT_EQ(5, toFirst);
}

TEST(SeizeTest, WithNoFringeEverythingGoesToTheRear)
{
  const HexModel::Board board = ToyBoard::build();
  const std::vector<Target> targets{{ToyBoard::at(board, "A1"), 1.0, false}};
  const std::vector<Force> forces{{ToyBoard::at(board, "C3"), 1.0}, {ToyBoard::at(board, "C4"), 1.0, false}};
  const auto out = HexAi::allocate(forces, targets, 0.5, byDistance(board, targets));
  EXPECT_EQ(0u, out[0]);
  EXPECT_EQ(0u, out[1]);
  EXPECT_TRUE(HexAi::allocate(forces, {}, 0.5, byDistance(board, targets))[0] == std::nullopt);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
