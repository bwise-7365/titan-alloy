// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Tests for the pmatrix GMS reader: the example file loads to the reference
// instance, and each contract violation throws std::invalid_argument.
// ----------------------------------------------
#include "pmatrixgms.hpp"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

using namespace VIMCP;
using namespace VIMCP::App;

namespace {

  // Write 'content' to a fresh temp .gms file and return its path.
  std::string
  writeTempGms(const std::string& content)
  {
    static int counter = 0;
    const std::filesystem::path path =
        std::filesystem::temp_directory_path()
        / ("pmatrix_test_" + std::to_string(counter++) + ".gms");
    std::ofstream(path) << content;
    return path.string();
  }

  // A minimal valid 2-actor x 2-option instance in the accepted subset.
  const std::string kValid =
      "Set act / A0, A1 / ;\n"
      "Set opt / P0, P1 / ;\n"
      "Parameter weight(act) /\n A0 10.0\n A1 20.0\n / ;\n"
      "Table reward(act, opt)\n"
      "     P0    P1\n"
      "A0  1.0   2.0\n"
      "A1  3.0   4.0\n"
      ";\n"
      "Parameters\n    raFrac\n    ;\n"
      "raFrac = 0.2 ;\n";

} // namespace

TEST(PmatrixGms, ExampleFileLoadsReferenceInstance)
{
  const std::string path =
      std::string(VIMCP_APPS_SAOE_DIR) + "/doc/pmatrix-example.gms";
  const PmatrixInput in = readPmatrixGms(path);

  EXPECT_EQ(in.R.rows(), 6);
  EXPECT_EQ(in.R.cols(), 10);
  EXPECT_EQ(in.S.size(), 6);
  EXPECT_DOUBLE_EQ(in.S(0), 68.0);
  EXPECT_DOUBLE_EQ(in.S(2), 125.0);
  EXPECT_DOUBLE_EQ(in.R(0, 4), 256.02);
  EXPECT_DOUBLE_EQ(in.R(4, 5), 270.83);
  EXPECT_DOUBLE_EQ(in.raFrac, 0.2);
  EXPECT_EQ(in.actorLabels.size(), 6u);
  EXPECT_EQ(in.optionLabels.size(), 10u);
  EXPECT_EQ(in.optionLabels[4], "P4");
}

TEST(PmatrixGms, MinimalValidLoads)
{
  const PmatrixInput in = readPmatrixGms(writeTempGms(kValid));
  EXPECT_EQ(in.R.rows(), 2);
  EXPECT_EQ(in.R.cols(), 2);
  EXPECT_DOUBLE_EQ(in.R(1, 1), 4.0);
  EXPECT_DOUBLE_EQ(in.S(1), 20.0);
  EXPECT_DOUBLE_EQ(in.raFrac, 0.2);
}

TEST(PmatrixGms, ExtraSymbolRejected)
{
  const std::string content =
      kValid + "Parameters\n    junk\n    ;\njunk = 1.0 ;\n";
  EXPECT_THROW(readPmatrixGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PmatrixGms, MissingRaFracRejected)
{
  const std::string content =
      "Set act / A0, A1 / ;\n"
      "Set opt / P0, P1 / ;\n"
      "Parameter weight(act) /\n A0 10.0\n A1 20.0\n / ;\n"
      "Table reward(act, opt)\n     P0    P1\nA0  1.0   2.0\nA1  3.0   4.0\n;\n";
  EXPECT_THROW(readPmatrixGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PmatrixGms, VariablePresentRejected)
{
  const std::string content = kValid + "Positive Variable v ;\n";
  EXPECT_THROW(readPmatrixGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PmatrixGms, RaFracOutOfRangeRejected)
{
  std::string content = kValid;
  const std::string bad = "raFrac = 1.5 ;\n";
  content += bad;   // last assignment wins -> raFrac = 1.5
  EXPECT_THROW(readPmatrixGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PmatrixGms, NonPositiveWeightRejected)
{
  const std::string content =
      "Set act / A0, A1 / ;\n"
      "Set opt / P0, P1 / ;\n"
      "Parameter weight(act) /\n A0 10.0\n A1 -5.0\n / ;\n"
      "Table reward(act, opt)\n     P0    P1\nA0  1.0   2.0\nA1  3.0   4.0\n;\n"
      "Parameters\n    raFrac\n    ;\nraFrac = 0.2 ;\n";
  EXPECT_THROW(readPmatrixGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PmatrixGms, WrongWeightDomainRejected)
{
  // weight declared over opt (size 3) instead of act (size 2): a shape mismatch.
  const std::string content =
      "Set act / A0, A1 / ;\n"
      "Set opt / P0, P1, P2 / ;\n"
      "Parameter weight(opt) /\n P0 10.0\n P1 20.0\n P2 30.0\n / ;\n"
      "Table reward(act, opt)\n     P0    P1    P2\n"
      "A0  1.0   2.0   3.0\nA1  4.0   5.0   6.0\n;\n"
      "Parameters\n    raFrac\n    ;\nraFrac = 0.2 ;\n";
  EXPECT_THROW(readPmatrixGms(writeTempGms(content)), std::invalid_argument);
}

TEST(PmatrixGms, MalformedGmsRejected)
{
  EXPECT_THROW(readPmatrixGms(writeTempGms("this is not valid gms %%%\n")),
               std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
