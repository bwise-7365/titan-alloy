// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/BoardBuilder.h"
#include "hexrules/RuleSetBuilder.h"
#include "hexsearch/Search.h"
#include "hexxml/PackageDoc.h"
#include "hexxml/RulesDoc.h"
#include "hexxml/SheetDoc.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <functional>
#include <optional>
#include <span>

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

TEST(NetworkGraphTest, TrcRailReachability)
{
  const HexModel::Board board = buildTrc();
  const HexModel::NetworkId rail = board.networkId("rail");
  const HexModel::LinkNetwork& network = board.network(rail);
  ASSERT_GT(network.linkCount(), 200u);

  // FF33 - EE32 - DD32 - DD31 is a chain of three drawn rail links, with no branch between them, on the
  // west edge of the sheet; over the network graph those hexes are one, two and three links apart,
  // whatever their distance on the hex lattice is.
  const HexModel::HexIndex start = board.indexOf(HexCoord::HexId{"FF33"});
  const HexModel::HexIndex middle = board.indexOf(HexCoord::HexId{"EE32"});
  const HexModel::HexIndex finish = board.indexOf(HexCoord::HexId{"DD31"});

  const HexSearch::NetworkGraph graph(board, rail, [](std::size_t) { return true; });
  HexSearch::SearchScratch scratch;
  const HexSearch::NodeIndex source = start.value;
  const std::function<bool(HexSearch::NodeIndex, std::size_t)> anyLink = [](HexSearch::NodeIndex, std::size_t) {
    return true;
  };
  const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
      graph, scratch, std::span<const HexSearch::NodeIndex>(&source, 1), anyLink, 99);

  ASSERT_TRUE(field.reachedP(middle.value));
  ASSERT_TRUE(field.reachedP(finish.value));
  EXPECT_EQ(1, field.distance(middle.value).halves);
  EXPECT_EQ(3, field.distance(finish.value).halves);

  // A hex the sheet draws no rail through is not on the network at all.
  EXPECT_FALSE(field.reachedP(board.indexOf(HexCoord::HexId{"A1"}).value));
}

TEST(NetworkGraphTest, RemovingOneLinkDisconnectsItsPair)
{
  const HexModel::Board board = buildTrc();
  const HexModel::NetworkId rail = board.networkId("rail");
  const HexModel::LinkNetwork& network = board.network(rail);

  // The sheet's first rail link, Berlin E31 - F31, is Berlin's only link: refusing it leaves Berlin
  // alone on the network, cut off from F31.
  const HexModel::HexIndex from = board.indexOf(HexCoord::HexId{"E31"});
  const HexModel::HexIndex to = board.indexOf(HexCoord::HexId{"F31"});
  std::optional<std::size_t> theLink;
  for (std::size_t i = 0; i < network.links().size(); ++i) {
    const HexModel::LinkNetwork::Link& link = network.links()[i];
    if ((link.a == from && link.b == to) || (link.a == to && link.b == from)) {
      theLink = i;
      break;
    }
  }
  ASSERT_TRUE(theLink.has_value());
  EXPECT_TRUE(network.connectedP(from, to));

  HexSearch::SearchScratch scratch;
  const HexSearch::NodeIndex source = from.value;
  const std::function<bool(HexSearch::NodeIndex, std::size_t)> anyLink = [](HexSearch::NodeIndex, std::size_t) {
    return true;
  };

  {
    const HexSearch::NetworkGraph whole(board, rail, [](std::size_t) { return true; });
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
        whole, scratch, std::span<const HexSearch::NodeIndex>(&source, 1), anyLink, 99);
    EXPECT_TRUE(field.reachedP(to.value));
  }
  {
    const std::size_t cut = *theLink;
    const HexSearch::NetworkGraph severed(board, rail, [cut](std::size_t link) { return link != cut; });
    const HexSearch::Field<HexSearch::CostHalves> field = HexSearch::Algorithms::bfsFlood(
        severed, scratch, std::span<const HexSearch::NodeIndex>(&source, 1), anyLink, 99);
    EXPECT_FALSE(field.reachedP(to.value));
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
