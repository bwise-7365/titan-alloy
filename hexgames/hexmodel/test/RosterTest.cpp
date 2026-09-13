// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/RosterBuilder.h"

#include "hexrules/RuleSetBuilder.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>
#include <vector>

namespace {

  std::filesystem::path
  root()
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR);
  }

  // A simple, test-only reader; recognises "A-D", "A-D-M", "A-U" (unlimited range), a bare number,
  // and a leading parenthesised group as in PGG's "(3)*10" (a support value ignored here, followed
  // by the printed strength).
  HexModel::Strengths
  simpleValueLineReader(std::string_view line, HexModel::UnitKind)
  {
    using namespace HexModel;
    std::string text(line);
    std::size_t paren = text.find(')');
    if (!text.empty() && '(' == text.front() && std::string::npos != paren) {
      text = text.substr(paren + 1);
    }
    std::vector<std::string> tokens;
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
    // Anything else (illustrative text values like "Hero RD") is left as an all-nullopt Strengths
    // rather than thrown, since this reader's job in these tests is only to prove the mechanism.
    return s;
  }

  HexXml::CounterSetDoc
  loadCounters(const char* name)
  {
    return HexXml::CounterSetDoc::parse(
        HexXml::XmlDocument::load(root() / "unit_graphics" / "xml" / name));
  }

}  // namespace

TEST(RosterTest, TrcEveryCounterParsesAndMarkersAreExcluded)
{
  const HexRules::RuleSet ruleSet = HexRules::RuleSetBuilder::build(
      HexXml::RulesDoc::parse(HexXml::XmlDocument::load(root() / "game_rules" / "xml" / "the-russian-campaign.xml")));
  const HexXml::CounterSetDoc counters = loadCounters("the-russian-campaign.xml");
  const HexXml::PackageDoc package =
      HexXml::PackageDoc::parse(HexXml::XmlDocument::load(root() / "packages" / "xml" / "trc.package.xml"));

  const HexModel::Roster roster =
      HexModel::RosterBuilder::build(counters, ruleSet, package, &simpleValueLineReader);

  EXPECT_EQ(202u, roster.units().size());  // 214 counters minus the 12 markers

  const std::optional<HexModel::UnitId> hitler = roster.find(HexModel::CounterId{"g-ge-hitler-hitler"});
  ASSERT_TRUE(hitler.has_value());
  const HexModel::UnitSpec& hitlerSpec = roster.unit(*hitler);
  ASSERT_TRUE(hitlerSpec.front.attack.has_value());
  EXPECT_EQ(1, hitlerSpec.front.attack->value);
}

TEST(RosterTest, UnlimitedRangeIsSetForAFourDashUValueLine)
{
  using namespace HexModel;
  const Strengths s = simpleValueLineReader("4-U", UnitKind::Air);
  ASSERT_TRUE(s.attack.has_value());
  EXPECT_EQ(4, s.attack->value);
  EXPECT_TRUE(s.unlimitedRangeP);
}

TEST(RosterTest, ParenthesisedSupportValueParses)
{
  using namespace HexModel;
  const Strengths s = simpleValueLineReader("(3)10", UnitKind::Hq);
  ASSERT_TRUE(s.attack.has_value());
  EXPECT_EQ(10, s.attack->value);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
