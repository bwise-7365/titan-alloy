// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the shared pformtext renderers: the label/pattern/matching helpers,
// the seed convention, and a frozen coalition report (the text both the CLI
// prints and the GUI displays).
// ----------------------------------------------
#include "pformtext.hpp"

#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <vector>

using namespace VINCP;
using namespace VINCP::App;

namespace {

  // The fixture of PformProblem.CoalitionsGroupTwoClassesWithOneFreeIssue:
  // M = 2 parties, D = 2 issues => K = 4; issue 0 varies within each class,
  // issue 1 pins the class (P0 for parliaments 0,1; P1 for 2,3).
  PformInstance
  twoClassInstance()
  {
    PformInstance in;
    in.data.weight = VectorXd(2);
    in.data.weight << 1.0, 2.0;
    in.data.position = MatrixXd::Zero(2, 2);
    in.data.salience = MatrixXd::Ones(2, 2);
    in.partyLabels = pformDefaultLabels("P", 2);
    in.issueLabels = pformDefaultLabels("I", 2);
    return in;
  }

  std::vector<PformCoalition>
  twoClassCoalitions()
  {
    PformResult res;
    res.effort = MatrixXd(2, 4);
    res.effort << 0.30, 0.30, 0.10, 0.10,    // P0
                  0.25, 0.25, 0.20, 0.20;    // P1
    res.probabilities = VectorXd(4);
    res.probabilities << 0.30, 0.30, 0.20, 0.20;
    res.utilities = VectorXd::Zero(2);
    res.eta = VectorXd::Zero(4);
    res.phi = VectorXd::Zero(4);
    return pformCoalitions(res, 2, 2);
  }

} // namespace

TEST(PformText, DefaultLabelsUsePrefixAndIndex)
{
  const std::vector<std::string> labels = pformDefaultLabels("P", 3);
  ASSERT_EQ(labels.size(), 3u);
  EXPECT_EQ(labels[0], "P0");
  EXPECT_EQ(labels[2], "P2");
}

TEST(PformText, ResolveSeedPassesNonZeroThroughAndRerollsZero)
{
  EXPECT_EQ(pformResolveSeed(42ULL), 42ULL);
  EXPECT_NE(pformResolveSeed(0ULL), 0ULL);   // surprise-me never returns 0
}

TEST(PformText, CoalitionLabelBoundaries)
{
  EXPECT_EQ(pformCoalitionLabel(0), "A");
  EXPECT_EQ(pformCoalitionLabel(25), "Z");
  EXPECT_EQ(pformCoalitionLabel(26), "AA");
  EXPECT_EQ(pformCoalitionLabel(27), "AB");
  EXPECT_EQ(pformCoalitionLabel(51), "AZ");
  EXPECT_EQ(pformCoalitionLabel(52), "BA");
}

TEST(PformText, MatchingTextListsControllingParties)
{
  // Mixed radix, issue 0 least significant: k = 6 over M = 2, D = 3.
  const std::vector<std::string> labels = pformDefaultLabels("P", 2);
  EXPECT_EQ(pformMatchingText(6, 2, 3, labels), "[P0 P1 P1]");
}

TEST(PformText, PatternTextRightJustifiesToWidestLabel)
{
  const std::vector<Index> pattern{ kFreeIssue, 0 };
  EXPECT_EQ(pformPatternText(pattern, pformDefaultLabels("P", 2)), "[ * P0]");
  // Uneven label widths: every cell is padded to the widest label.
  const std::vector<std::string> uneven{ "Alpha", "Bo" };
  const std::vector<Index> pinnedThenFree{ 1, kFreeIssue };
  EXPECT_EQ(pformPatternText(pinnedThenFree, uneven), "[   Bo     *]");
}

TEST(PformText, RenderCoalitionsIsFrozen)
{
  // Freezes the shared coalition report so the CLI/GUI text cannot drift
  // silently. If a deliberate format change lands, update this literal from a
  // captured run.
  const std::string expected =
      "\nCoalition structure (parliaments grouped by identical party support;\n"
      "'*' marks an issue whose controlling party is free within the coalition):\n"
      "  id parties pattern contributions   prob(each)  seats prob(total)\n"
      "  A  P0,P1   [ * P0] P0:0.30 P1:0.25     0.3000      2      0.6000\n"
      "  B  P0,P1   [ * P1] P0:0.10 P1:0.20     0.2000      2      0.4000\n"
      "\nParty vote split (effort committed to each coalition; xN = N seats):\n"
      "  P0       ->  A:0.30x2  B:0.10x2   (weight 1.00)\n"
      "  P1       ->  A:0.25x2  B:0.20x2   (weight 2.00)\n";
  EXPECT_EQ(renderPformCoalitions(twoClassInstance(), twoClassCoalitions()),
            expected);
}

TEST(PformText, RenderCoalitionsEmptyReportsNoSupport)
{
  const std::string expected =
      "\nCoalition structure (parliaments grouped by identical party support;\n"
      "'*' marks an issue whose controlling party is free within the coalition):\n"
      "  (no supported parliaments)\n";
  EXPECT_EQ(renderPformCoalitions(twoClassInstance(), {}), expected);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
