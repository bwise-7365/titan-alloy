// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/BoardBuilder.h"

#include "hexrules/RuleSetBuilder.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/SheetDoc.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>

namespace {

  std::filesystem::path
  root()
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR);
  }

  HexModel::Board
  buildTrc()
  {
    const HexXml::RulesDoc rules = HexXml::RulesDoc::parse(
        HexXml::XmlDocument::load(root() / "game_rules" / "xml" / "the-russian-campaign.xml"));
    const HexRules::RuleSet ruleSet = HexRules::RuleSetBuilder::build(rules);
    const HexXml::SheetDoc sheet = HexXml::SheetDoc::parse(
        HexXml::XmlDocument::load(root() / "map_graphics" / "xml" / "the-russian-campaign.xml"));
    const HexXml::PackageDoc package =
        HexXml::PackageDoc::parse(HexXml::XmlDocument::load(root() / "packages" / "xml" / "trc.package.xml"));
    return HexModel::BoardBuilder::build(sheet, ruleSet, package);
  }

}  // namespace

TEST(BoardTest, TrcBoardFromSheet)
{
  const HexModel::Board board = buildTrc();

  EXPECT_GT(board.hexCount(), 1000u);

  const HexModel::HexIndex kk19 = board.indexOf(HexCoord::HexId{"KK19"});
  const HexModel::HexIndex kk20 = board.indexOf(HexCoord::HexId{"KK20"});
  const std::optional<HexModel::HexIndex> east = board.neighbour(kk20, HexCoord::fromCompass("e", board.orientation()));
  ASSERT_TRUE(east.has_value());
  EXPECT_EQ(kk19, *east);

  // The strait itself is a rules fact (TrcFacts::kerchP), not a sheet line; a blocked hexside is a sheet line.
  const HexModel::HexIndex kk32 = board.indexOf(HexCoord::HexId{"KK32"});
  const std::vector<HexModel::EdgeTerrainId>& kk32east =
      board.edge(kk32, HexCoord::fromCompass("e", board.orientation()));
  EXPECT_FALSE(kk32east.empty());

  const HexModel::HexIndex someSea = board.indexOf(HexCoord::HexId{"HH31"});
  (void)board.terrain(someSea);  // must not throw; TRC's sheet marks HH31 as sea

  const HexModel::HexIndex moscow = board.indexOf(HexCoord::HexId{"R9"});
  const std::vector<HexModel::Feature>& features = board.features(moscow);
  ASSERT_FALSE(features.empty());
  EXPECT_TRUE(features.front().name.has_value());
  EXPECT_EQ("MOSCOW", *features.front().name);

  const HexModel::NetworkId rail = board.networkId("rail");
  const HexModel::LinkNetwork& net = board.network(rail);
  EXPECT_GT(net.linkCount(), 0u);
}

TEST(BoardTest, EveryPackageAvailableBuilds)
{
  // Only trc.package.xml exists today; this test automatically covers more games as their
  // manifests are authored (see PackageTest.FourPackagesCheckClean's own such note).
  const std::filesystem::path packagesDir = root() / "packages" / "xml";
  for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(packagesDir)) {
    if (".xml" != entry.path().extension() || "hexpackage.xsd" == entry.path().filename()) {
      continue;
    }
    const HexXml::PackageDoc package = HexXml::PackageDoc::parse(HexXml::XmlDocument::load(entry.path()));
    const std::filesystem::path base = entry.path().parent_path();
    const HexXml::RulesDoc rulesDoc =
        HexXml::RulesDoc::parse(HexXml::XmlDocument::load(base / package.rules.path));
    const HexRules::RuleSet ruleSet = HexRules::RuleSetBuilder::build(rulesDoc);
    ASSERT_FALSE(package.sheets.empty());
    const HexXml::SheetDoc sheet =
        HexXml::SheetDoc::parse(HexXml::XmlDocument::load(base / package.sheets.front().path));
    EXPECT_NO_THROW((void)HexModel::BoardBuilder::build(sheet, ruleSet, package)) << entry.path();
  }
}

TEST(BoardTest, DaiSensoTwoGridsOneLattice)
{
  // No package manifest exists yet for Dai Senso; its sheet and rules terrain vocabularies do not
  // match by identity (sheet: sea/hills/clear/desert/forest/swamp; rules: clear/city/port/town/
  // rough/all-sea-hex/ice), so this test supplies just enough terrain bindings, locally, to
  // exercise the grid-sharing mechanism it is actually about.
  const HexRules::RuleSet ruleSet = HexRules::RuleSetBuilder::build(
      HexXml::RulesDoc::parse(HexXml::XmlDocument::load(root() / "game_rules" / "xml" / "dai-senso.xml")));
  const HexXml::SheetDoc sheet = HexXml::SheetDoc::parse(
      HexXml::XmlDocument::load(root() / "map_graphics" / "xml" / "dai-senso.xml"));
  HexXml::PackageDoc package;
  for (const auto& [sheetTerrain, rulesTerrain] :
       {std::pair{"sea", "all-sea-hex"}, std::pair{"hills", "rough"}, std::pair{"desert", "rough"},
        std::pair{"forest", "rough"}, std::pair{"swamp", "rough"}}) {
    HexXml::PackageTerrainBindingDoc binding;
    binding.sheet = sheetTerrain;
    binding.rules = rulesTerrain;
    package.terrain.push_back(binding);
  }
  const HexModel::Board board = HexModel::BoardBuilder::build(sheet, ruleSet, package);

  ASSERT_EQ(2u, board.grids().size());

  // The west grid's east-most column and the east grid's west-most column form the seam; any
  // hex from each side that are geometric neighbours confirms both grids sit on one lattice.
  bool foundNeighbouringSeamPair = false;
  for (std::size_t i = 0; i < board.hexCount() && !foundNeighbouringSeamPair; ++i) {
    const HexModel::HexIndex hi{static_cast<std::uint32_t>(i)};
    if (!board.id(hi).text.starts_with("w")) {
      continue;
    }
    for (int d = 0; d < 6; ++d) {
      const auto nb = board.neighbour(hi, static_cast<HexCoord::Direction>(d));
      if (nb && board.id(*nb).text.starts_with("e")) {
        foundNeighbouringSeamPair = true;
        break;
      }
    }
  }
  EXPECT_TRUE(foundNeighbouringSeamPair);
}

TEST(BoardTest, UnknownSheetTerrainThrowsNamingTheId)
{
  const HexRules::RuleSet ruleSet = HexRules::RuleSetBuilder::build(HexXml::RulesDoc::parse(
      HexXml::XmlDocument::load(root() / "game_rules" / "xml" / "the-russian-campaign.xml")));
  const HexXml::SheetDoc badSheet = HexXml::SheetDoc::parse(
      HexXml::XmlDocument::load(root() / "hexmodel" / "test" / "sheet-unknown-terrain.xml"));
  const HexXml::PackageDoc emptyPackage;
  try {
    (void)HexModel::BoardBuilder::build(badSheet, ruleSet, emptyPackage);
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string message = e.what();
    EXPECT_NE(std::string::npos, message.find("not-a-real-terrain"));
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
