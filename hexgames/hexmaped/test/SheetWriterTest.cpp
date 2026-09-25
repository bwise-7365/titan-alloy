// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Load, write, load: the document model must come back equal, field by field, on every sheet in the
// tree; and the written text must not carry a value the loader refuses.
#include "hexmaped/SheetWriter.h"
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

namespace {

  HexXml::SheetDoc
  load(const std::filesystem::path& p)
  {
    return HexXml::SheetDoc::parse(HexXml::XmlDocument::load(p));
  }

  std::filesystem::path
  sheetPath(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "map_graphics" / "xml" / name;
  }

  HexXml::SheetDoc
  roundTrip(const HexXml::SheetDoc& doc, const char* stem)
  {
    const std::filesystem::path tmp = std::filesystem::temp_directory_path() / (std::string("hexmaped-") + stem + ".xml");
    HexMapEd::writeSheet(tmp, doc);
    HexXml::SheetDoc back = load(tmp);
    std::filesystem::remove(tmp);
    return back;
  }

  void
  expectSame(const HexXml::SheetDoc& a, const HexXml::SheetDoc& b)
  {
    EXPECT_EQ(a.id, b.id);
    EXPECT_EQ(a.title, b.title);
    EXPECT_EQ(a.width, b.width);
    EXPECT_EQ(a.urban, b.urban);
    ASSERT_EQ(a.grids.size(), b.grids.size());
    for (std::size_t i = 0; i < a.grids.size(); ++i) {
      EXPECT_EQ(a.grids[i].cols, b.grids[i].cols);
      EXPECT_EQ(a.grids[i].idFormat, b.grids[i].idFormat);
      EXPECT_EQ(a.grids[i].clip, b.grids[i].clip);
      EXPECT_EQ(a.grids[i].terrain, b.grids[i].terrain);
      EXPECT_DOUBLE_EQ(a.grids[i].size, b.grids[i].size);
    }
    EXPECT_EQ(a.palette.size(), b.palette.size());
    EXPECT_EQ(a.terrains.size(), b.terrains.size());
    EXPECT_EQ(a.lines.size(), b.lines.size());
    ASSERT_EQ(a.hexesBulk.size(), b.hexesBulk.size());
    for (std::size_t i = 0; i < a.hexesBulk.size(); ++i) {
      EXPECT_EQ(a.hexesBulk[i].terrain, b.hexesBulk[i].terrain);
      EXPECT_EQ(a.hexesBulk[i].ids, b.hexesBulk[i].ids);
    }
    ASSERT_EQ(a.edges.size(), b.edges.size());
    for (std::size_t i = 0; i < a.edges.size(); ++i) {
      EXPECT_EQ(a.edges[i].at, b.edges[i].at);
      EXPECT_EQ(a.edges[i].line, b.edges[i].line);
      EXPECT_EQ(a.edges[i].symbol, b.edges[i].symbol);
    }
    ASSERT_EQ(a.links.size(), b.links.size());
    for (std::size_t i = 0; i < a.links.size(); ++i) {
      EXPECT_EQ(a.links[i].hexes, b.links[i].hexes);
      EXPECT_EQ(a.links[i].line, b.links[i].line);
      EXPECT_EQ(a.links[i].kind, b.links[i].kind);
    }
    EXPECT_EQ(a.paths.size(), b.paths.size());
    ASSERT_EQ(a.hexes.size(), b.hexes.size());
    for (std::size_t i = 0; i < a.hexes.size(); ++i) {
      EXPECT_EQ(a.hexes[i].id, b.hexes[i].id);
      EXPECT_EQ(a.hexes[i].name, b.hexes[i].name);
      EXPECT_EQ(a.hexes[i].ring, b.hexes[i].ring);
      ASSERT_EQ(a.hexes[i].glyphs.size(), b.hexes[i].glyphs.size());
      for (std::size_t j = 0; j < a.hexes[i].glyphs.size(); ++j) {
        EXPECT_EQ(a.hexes[i].glyphs[j].symbol, b.hexes[i].glyphs[j].symbol);
        EXPECT_EQ(a.hexes[i].glyphs[j].mark, b.hexes[i].glyphs[j].mark);
        EXPECT_EQ(a.hexes[i].glyphs[j].slot, b.hexes[i].glyphs[j].slot);
        EXPECT_EQ(a.hexes[i].glyphs[j].color, b.hexes[i].glyphs[j].color);
      }
      EXPECT_EQ(a.hexes[i].sides.size(), b.hexes[i].sides.size());
    }
    ASSERT_EQ(a.regions.size(), b.regions.size());
    for (std::size_t i = 0; i < a.regions.size(); ++i) {
      EXPECT_EQ(a.regions[i].hexes, b.regions[i].hexes);
      EXPECT_EQ(a.regions[i].name, b.regions[i].name);
    }
    ASSERT_EQ(a.labels.size(), b.labels.size());
    for (std::size_t i = 0; i < a.labels.size(); ++i) {
      EXPECT_EQ(a.labels[i].text, b.labels[i].text);
      EXPECT_EQ(a.labels[i].at, b.labels[i].at);
      EXPECT_EQ(a.labels[i].x, b.labels[i].x);
      EXPECT_EQ(a.labels[i].size, b.labels[i].size);
    }
    ASSERT_EQ(a.panels.size(), b.panels.size());
    for (std::size_t i = 0; i < a.panels.size(); ++i) {
      EXPECT_EQ(a.panels[i].id, b.panels[i].id);
      EXPECT_EQ(a.panels[i].texts.size(), b.panels[i].texts.size());
      EXPECT_EQ(a.panels[i].boxes.size(), b.panels[i].boxes.size());
      EXPECT_EQ(a.panels[i].tracks.size(), b.panels[i].tracks.size());
      EXPECT_EQ(a.panels[i].tables.size(), b.panels[i].tables.size());
      for (std::size_t j = 0; j < a.panels[i].tables.size(); ++j) {
        EXPECT_EQ(a.panels[i].tables[j].rows.size(), b.panels[i].tables[j].rows.size());
      }
    }
  }

}  // namespace

TEST(SheetWriterTest, RoundTripsEverySheetInTheTree)
{
  for (const char* name : {"stalin-moves-west.xml", "panzergruppe-guderian.xml", "the-russian-campaign.xml",
                           "dai-senso.xml", "d-day-at-tarawa.xml"}) {
    SCOPED_TRACE(name);
    const HexXml::SheetDoc doc = load(sheetPath(name));
    expectSame(doc, roundTrip(doc, "roundtrip"));
  }
}

TEST(SheetWriterTest, WritesTheSchemaOrderAndEscapesText)
{
  HexXml::SheetDoc doc = load(sheetPath("stalin-moves-west.xml"));
  doc.title = "A & B <C>";
  const std::string text = HexMapEd::writeSheet(doc);
  EXPECT_NE(text.find("title=\"A &amp; B &lt;C&gt;\""), std::string::npos);
  EXPECT_LT(text.find("<grid "), text.find("<palette>"));
  EXPECT_LT(text.find("<palette>"), text.find("<terrains>"));
  EXPECT_LT(text.find("<terrains>"), text.find("<lines>"));
  EXPECT_LT(text.find("<lines>"), text.find("<hexes "));
  EXPECT_EQ(text.find("\r\n"), std::string::npos);
}

TEST(SheetWriterTest, RefusesAnUnwritablePath)
{
  const HexXml::SheetDoc doc = load(sheetPath("stalin-moves-west.xml"));
  EXPECT_THROW(HexMapEd::writeSheet(std::filesystem::path("Z:/no/such/dir/x.xml"), doc), std::invalid_argument);
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
