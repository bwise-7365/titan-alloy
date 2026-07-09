// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the pform GMS reader: the example file loads to the expected
// instance, and each contract violation throws std::invalid_argument.
// ----------------------------------------------
#include "pformgms.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace VINCP;
using namespace VINCP::App;

namespace {

  std::string
  writeTempGms(const std::string& content)
  {
    static int counter = 0;
    const std::filesystem::path path =
        std::filesystem::temp_directory_path()
        / ("pform_test_" + std::to_string(counter++) + ".gms");
    std::ofstream(path) << content;
    return path.string();
  }

  // A minimal valid 2-party x 2-issue instance (K = 4).
  const std::string kValid =
      "Set act / P0, P1 / ;\n"
      "Set iss / I0, I1 / ;\n"
      "Parameter weight(act) /\n P0 5.0\n P1 3.0\n / ;\n"
      "Table position(iss, act)\n"
      "     P0    P1\n"
      "I0  0.2   0.8\n"
      "I1  0.6   0.1\n"
      ";\n"
      "Table salience(iss, act)\n"
      "     P0    P1\n"
      "I0  0.6   0.7\n"
      "I1  0.6   0.5\n"
      ";\n"
      "Parameters\n    unselectedProb\n    ;\n"
      "unselectedProb = 0.05 ;\n";

} // namespace

TEST(PformGms, ExampleFileLoads)
{
  const std::string path =
      std::string(VINCP_APPS_PFORM_DIR) + "/doc/pform-example.gms";
  const PformGmsInput in = readPformGms(path);

  EXPECT_EQ(in.data.weight.size(), 3);
  EXPECT_EQ(in.data.position.rows(), 4);   // issues
  EXPECT_EQ(in.data.position.cols(), 3);   // parties
  EXPECT_EQ(in.data.salience.rows(), 4);
  EXPECT_EQ(in.data.salience.cols(), 3);
  EXPECT_DOUBLE_EQ(in.data.weight(0), 5.0);
  EXPECT_DOUBLE_EQ(in.data.weight(2), 7.0);
  EXPECT_DOUBLE_EQ(in.data.position(0, 0), 0.20);   // I0, P0
  EXPECT_DOUBLE_EQ(in.data.position(3, 2), 0.60);   // I3, P2
  EXPECT_DOUBLE_EQ(in.data.salience(1, 1), 0.50);   // I1, P1
  EXPECT_DOUBLE_EQ(in.unselectedProb, 0.05);
  EXPECT_EQ(in.partyLabels.size(), 3u);
  EXPECT_EQ(in.issueLabels.size(), 4u);
}

TEST(PformGms, MinimalValidLoads)
{
  const PformGmsInput in = readPformGms(writeTempGms(kValid));
  EXPECT_EQ(in.data.weight.size(), 2);
  EXPECT_EQ(in.data.position.rows(), 2);
  EXPECT_DOUBLE_EQ(in.data.position(1, 0), 0.6);   // I1, P0
  EXPECT_DOUBLE_EQ(in.unselectedProb, 0.05);
}

TEST(PformGms, ExtraSymbolRejected)
{
  const std::string content =
      kValid + "Parameters\n    junk\n    ;\njunk = 1.0 ;\n";
  EXPECT_THROW(readPformGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PformGms, MissingSymbolRejected)
{
  // Drop the salience table.
  const std::string content =
      "Set act / P0, P1 / ;\n"
      "Set iss / I0, I1 / ;\n"
      "Parameter weight(act) /\n P0 5.0\n P1 3.0\n / ;\n"
      "Table position(iss, act)\n     P0    P1\nI0  0.2   0.8\nI1  0.6   0.1\n;\n"
      "Parameters\n    unselectedProb\n    ;\nunselectedProb = 0.05 ;\n";
  EXPECT_THROW(readPformGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PformGms, WrongWeightDomainRejected)
{
  // weight declared over iss (3) instead of act (2): a shape mismatch.
  const std::string content =
      "Set act / P0, P1 / ;\n"
      "Set iss / I0, I1, I2 / ;\n"
      "Parameter weight(iss) /\n I0 1.0\n I1 2.0\n I2 3.0\n / ;\n"
      "Table position(iss, act)\n     P0    P1\n"
      "I0  0.2   0.8\nI1  0.6   0.1\nI2  0.5   0.4\n;\n"
      "Table salience(iss, act)\n     P0    P1\n"
      "I0  0.4   0.4\nI1  0.4   0.4\nI2  0.4   0.4\n;\n"
      "Parameters\n    unselectedProb\n    ;\nunselectedProb = 0.05 ;\n";
  EXPECT_THROW(readPformGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PformGms, PositionOutOfRangeRejected)
{
  const std::string content =
      "Set act / P0, P1 / ;\n"
      "Set iss / I0, I1 / ;\n"
      "Parameter weight(act) /\n P0 5.0\n P1 3.0\n / ;\n"
      "Table position(iss, act)\n     P0    P1\nI0  0.2   1.5\nI1  0.6   0.1\n;\n"
      "Table salience(iss, act)\n     P0    P1\nI0  0.6   0.7\nI1  0.6   0.5\n;\n"
      "Parameters\n    unselectedProb\n    ;\nunselectedProb = 0.05 ;\n";
  EXPECT_THROW(readPformGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PformGms, LowSalienceColumnRejected)
{
  const std::string content =
      "Set act / P0, P1 / ;\n"
      "Set iss / I0, I1 / ;\n"
      "Parameter weight(act) /\n P0 5.0\n P1 3.0\n / ;\n"
      "Table position(iss, act)\n     P0    P1\nI0  0.2   0.8\nI1  0.6   0.1\n;\n"
      "Table salience(iss, act)\n     P0    P1\nI0  0.2   0.7\nI1  0.2   0.5\n;\n"
      "Parameters\n    unselectedProb\n    ;\nunselectedProb = 0.05 ;\n";
  EXPECT_THROW(readPformGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PformGms, NonPositiveWeightRejected)
{
  const std::string content =
      "Set act / P0, P1 / ;\n"
      "Set iss / I0, I1 / ;\n"
      "Parameter weight(act) /\n P0 5.0\n P1 0.0\n / ;\n"
      "Table position(iss, act)\n     P0    P1\nI0  0.2   0.8\nI1  0.6   0.1\n;\n"
      "Table salience(iss, act)\n     P0    P1\nI0  0.6   0.7\nI1  0.6   0.5\n;\n"
      "Parameters\n    unselectedProb\n    ;\nunselectedProb = 0.05 ;\n";
  EXPECT_THROW(readPformGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PformGms, UnselectedProbOutOfRangeRejected)
{
  // q = 0.9 >= (K-1)/K = 0.75 for K = 4.
  std::string content = kValid;
  content += "unselectedProb = 0.9 ;\n";   // last assignment wins
  EXPECT_THROW(readPformGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PformGms, MalformedGmsRejected)
{
  EXPECT_THROW(readPformGms(writeTempGms("this is not valid gms %%%\n")),
               std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
