// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexrules/Package.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <stdexcept>

namespace {

  std::filesystem::path
  root()
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR);
  }

  HexModel::Strengths
  trcValueLines(std::string_view line, HexModel::UnitKind)
  {
    using namespace HexModel;
    std::vector<std::string> tokens;
    std::string text(line);
    std::size_t pos = 0;
    while (pos <= text.size()) {
      const std::size_t dash = text.find('-', pos);
      tokens.push_back(text.substr(pos, std::string::npos == dash ? std::string::npos : dash - pos));
      if (std::string::npos == dash) {
        break;
      }
      pos = dash + 1;
    }
    const auto isNumber = [](const std::string& s) {
      return !s.empty() && std::all_of(s.begin(), s.end(), [](unsigned char c) { return 0 != std::isdigit(c); });
    };
    Strengths s;
    if (2 == tokens.size() && isNumber(tokens[0]) && "U" == tokens[1]) {
      s.attack = Strength{std::stoi(tokens[0])};
      s.unlimitedRangeP = true;
    } else if (2 == tokens.size() && isNumber(tokens[0]) && isNumber(tokens[1])) {
      s.attack = Strength{std::stoi(tokens[0])};
      s.defence = Strength{std::stoi(tokens[1])};
    } else if (3 == tokens.size() && isNumber(tokens[0]) && isNumber(tokens[1]) && isNumber(tokens[2])) {
      s.attack = Strength{std::stoi(tokens[0])};
      s.defence = Strength{std::stoi(tokens[1])};
      s.allowance = MovementPoints::whole(std::stoi(tokens[2]));
    } else if (1 == tokens.size() && isNumber(tokens[0])) {
      s.attack = Strength{std::stoi(tokens[0])};
    }
    return s;
  }

}  // namespace

TEST(PackageTest, TrcLoads)
{
  const std::filesystem::path manifest = root() / "packages" / "xml" / "trc.package.xml";
  const HexRules::GameDefinition def = HexRules::PackageLoader::load(manifest, &trcValueLines);

  EXPECT_GT(def.board->hexCount(), 1000u);
  EXPECT_EQ(202u, def.roster->units().size());

  const std::vector<HexRules::PackageProblem> problems = HexRules::PackageLoader::check(manifest, &trcValueLines);
  EXPECT_TRUE(problems.empty()) << (problems.empty() ? "" : problems.front().message);
}

TEST(PackageTest, BindingProblemsAreNamed)
{
  const std::filesystem::path manifest = root() / "hexrules" / "test" / "package-broken.xml";
  const std::vector<HexRules::PackageProblem> problems = HexRules::PackageLoader::check(manifest, &trcValueLines);

  ASSERT_EQ(4u, problems.size());
  const auto contains = [&](std::string_view needle) {
    for (const HexRules::PackageProblem& p : problems) {
      if (std::string::npos != p.message.find(needle)) {
        return true;
      }
    }
    return false;
  };
  EXPECT_TRUE(contains("bogus"));
  EXPECT_TRUE(contains("known-unit"));
  EXPECT_TRUE(contains("missing-panel"));
  EXPECT_TRUE(contains("Z9"));

  EXPECT_THROW((void)HexRules::PackageLoader::load(manifest, &trcValueLines), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
