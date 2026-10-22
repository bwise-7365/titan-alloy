// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The builder's own promises (layer order, patterns, hit tags, refusals, the MapStyle switch, the
// buildings generator) and the writers' well-formedness. Parity with the reference renderer,
// element for element, is MapSvgGoldenTest's.
// ----------------------------------------------
#include "TestSupport.h"
#include "hexview/Buildings.h"
#include "hexview/MapSceneBuilder.h"
#include "hexview/SceneWriters.h"
#include "hexview/Shapes.h"

#include <gtest/gtest.h>

namespace {

  using namespace HexView;
  using namespace HexViewTest;

  struct Built {
    HexXml::SheetDoc sheet;
    MapFrame frame;
    SymbolLibrary lib;
    Scene scene;
  };

  Built
  build(const std::string& name, MapStyle style = MapStyle::reference())
  {
    HexXml::SheetDoc sheet = loadSheet(name);
    MapFrame frame = MapFrame::of(sheet);
    Built b{sheet, frame, SymbolLibrary::reference(), Scene(frame.width(), frame.height())};
    MapSceneBuilder(b.frame, b.lib, style).build(b.sheet, b.scene);
    return b;
  }

  bool
  hasQuadP(const Primitive& p)
  {
    const auto* s = std::get_if<PathShape>(&p.shape);
    return s && std::any_of(s->commands.begin(), s->commands.end(),
                            [](const PathCommand& c) { return std::holds_alternative<QuadTo>(c); });
  }

  std::size_t
  commandCount(const Primitive& p)
  {
    const auto* s = std::get_if<PathShape>(&p.shape);
    return s ? s->commands.size() : 0;
  }

  // Circling Dragons writes its rivers as path chains (so their confluences can be junctions at hex
  // corners). A river path is drawn as river edges are: rounded in the reference style, and the
  // border paths and edges after it stay straight.
  TEST(MapSceneBuilderTest, RiverPathsAreRoundedLikeRiverEdges)
  {
    const Built b = build("circling-dragons");
    const std::span<const Primitive> edges = b.scene.layer(Layer::Edges);
    const std::size_t rivers = b.sheet.paths.size();
    ASSERT_LT(rivers, edges.size());
    for (std::size_t i = 0; i < rivers; ++i) {
      // a chain of one hexside has no corner to round (the Yangtze between two confluences)
      EXPECT_EQ(2 <= b.sheet.paths[i].edges.size(), hasQuadP(edges[i])) << "river path " << i;
    }
    EXPECT_FALSE(hasQuadP(edges[rivers]));  // the first border hexside
  }

  // With the editor's scratch style the same river paths are scratched instead: straight segments
  // only, sampled far more finely than their hexsides.
  TEST(MapSceneBuilderTest, RiverPathsAreScratchedWhenTheStyleAsks)
  {
    MapStyle style = MapStyle::reference();
    style.scratch = ScratchStyle{0.3, 0.5};
    const Built b = build("circling-dragons", style);
    const std::span<const Primitive> edges = b.scene.layer(Layer::Edges);
    ASSERT_LE(b.sheet.paths.size(), edges.size());
    for (std::size_t i = 0; i < b.sheet.paths.size(); ++i) {
      EXPECT_FALSE(hasQuadP(edges[i])) << "river path " << i;
      EXPECT_GT(commandCount(edges[i]), 2 * b.sheet.paths[i].edges.size()) << "river path " << i;
    }
  }

  // A panel picture (hexsheet.xsd PanelImage) is an ImageShape in the panels layer: its href as the sheet
  // wrote it, its path resolved beside the sheet's file, its corner placed and turned with the panel; and
  // both writers emit it. A fixture sheet written into the build tree, so no real sheet need carry one.
  TEST(MapSceneBuilderTest, PanelImageIsPlacedInThePanelsLayer)
  {
    const std::filesystem::path dir = outDir();
    writeFile(dir / "fixture-logo.svg",
              "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"10\" height=\"8\"><rect width=\"10\" height=\"8\" fill=\"#c00\"/></svg>\n");
    writeFile(dir / "image-fixture.xml",
              "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
              "<sheet id=\"image-fixture\" title=\"Panel image fixture\" width=\"300\" height=\"200\" background=\"paper\" urban=\"symbol\" junctions=\"implicit\">\n"
              "  <grid id=\"g\" orientation=\"flat\" offset=\"odd\" cols=\"2\" rows=\"2\" size=\"30\" ox=\"40\" oy=\"40\" id-format=\"{col:02}{row:02}\" terrain=\"clear\" id-side=\"n\"/>\n"
              "  <palette><color id=\"paper\" value=\"#ffffff\"/><color id=\"tan\" value=\"#eeddcc\"/><color id=\"grid\" value=\"#888888\"/><color id=\"ink\" value=\"#111111\"/></palette>\n"
              "  <terrains><terrain id=\"clear\" name=\"Clear\" fill=\"tan\" stroke=\"grid\"/></terrains>\n"
              "  <panel id=\"p\" x=\"100\" y=\"20\" w=\"180\" h=\"160\" rotate=\"30\" fill=\"paper\" stroke=\"ink\">\n"
              "    <image href=\"fixture-logo.svg\" x=\"10\" y=\"20\" w=\"100\" h=\"80\"/>\n"
              "  </panel>\n"
              "</sheet>\n");
    HexXml::SheetDoc sheet = HexXml::SheetDoc::parse(HexXml::XmlDocument::load(dir / "image-fixture.xml"));
    MapFrame frame = MapFrame::of(sheet);
    Built b{sheet, frame, SymbolLibrary::reference(), Scene(frame.width(), frame.height())};
    MapSceneBuilder(b.frame, b.lib, MapStyle::reference()).build(b.sheet, b.scene);
    const ImageShape* found = nullptr;
    for (const Primitive& p : b.scene.layer(Layer::Panels)) {
      if (const auto* im = std::get_if<ImageShape>(&p.shape)) {
        found = im;
      }
    }
    ASSERT_NE(nullptr, found);
    EXPECT_EQ("fixture-logo.svg", found->href);
    EXPECT_EQ((dir / "fixture-logo.svg").generic_string(), found->path);
    EXPECT_TRUE(std::filesystem::exists(found->path)) << found->path;
    EXPECT_DOUBLE_EQ(100.0, found->w);
    EXPECT_DOUBLE_EQ(80.0, found->h);
    EXPECT_DOUBLE_EQ(30.0, found->angleDegrees);
    const Pixel corner = Similarity{Pixel{100.0, 20.0}, 30.0, 1.0}.apply(Pixel{10.0, 20.0});
    EXPECT_NEAR(corner.x, found->at.x, 1e-9);
    EXPECT_NEAR(corner.y, found->at.y, 1e-9);
    const std::string svg = writeSvg(b.scene, b.lib, b.sheet.title);
    EXPECT_NE(std::string::npos, svg.find("<image transform=\"translate("));
    EXPECT_NE(std::string::npos, svg.find("xlink:href=\"fixture-logo.svg\""));
    EXPECT_NE(std::string::npos, writeJson(b.scene, b.lib).find("\"kind\":\"image\""));
  }

  TEST(MapSceneBuilderTest, LayerOrderIsTheReferenceOrder)
  {
    const Built b = build("the-russian-campaign");
    ASSERT_EQ(1u, b.scene.layer(Layer::Background).size());
    // what each map layer holds on TRC, in the reference's order
    for (const Layer l : {Layer::Terrain, Layer::Grid, Layer::Edges, Layer::Links, Layer::Rings,
                          Layer::HexGlyphs, Layer::Labels, Layer::Panels}) {
      EXPECT_FALSE(b.scene.layer(l).empty()) << static_cast<int>(l);
    }
    for (const Layer l :
         {Layer::Control, Layer::Units, Layer::Markers, Layer::Highlights, Layer::Overlay}) {
      EXPECT_TRUE(b.scene.layer(l).empty());
    }
    EXPECT_TRUE(std::holds_alternative<TextShape>(b.scene.layer(Layer::Labels).front().shape));
  }

  TEST(MapSceneBuilderTest, TerrainPatternsAppearWhereDeclared)
  {
    const Built b = build("d-day-at-tarawa");
    std::map<Pattern, int> seen;
    int plain = 0;
    for (const Primitive& p : b.scene.layer(Layer::Terrain)) {
      const PathShape& s = std::get<PathShape>(p.shape);
      if (s.pattern.has_value()) {
        ++seen[*s.pattern];
      }
      else {
        ++plain;
      }
    }
    EXPECT_EQ(4u, seen.size());  // Tarawa uses dots, hatch, mottle and palms
    EXPECT_LT(0, plain);
  }

  TEST(MapSceneBuilderTest, TerrainCarriesTheBoardsHexIndex)
  {
    const Built b = build("dai-senso");
    const Primitive& first = b.scene.layer(Layer::Terrain).front();
    const HexHit hit = std::get<HexHit>(first.hit);
    // column-major reference order starts at the first grid's cell (0, 0)
    EXPECT_EQ(b.frame.index(b.frame.grids().front().idOf(HexCoord::GridIndex{0, 0})).value,
              hit.hex.value);
  }

  TEST(MapSceneBuilderTest, RefusesDanglingPaletteColourNamingItsLine)
  {
    HexXml::SheetDoc sheet = loadSheet("panzergruppe-guderian");
    ASSERT_FALSE(sheet.lines.empty());
    sheet.lines.front().stroke = "no-such-colour";
    const MapFrame frame = MapFrame::of(sheet);
    const SymbolLibrary lib = SymbolLibrary::reference();
    Scene scene(frame.width(), frame.height());
    try {
      MapSceneBuilder(frame, lib, MapStyle::reference()).build(sheet, scene);
      FAIL() << "no throw";
    }
    catch (const std::invalid_argument& e) {
      EXPECT_NE(std::string::npos, std::string(e.what()).find("no-such-colour")) << e.what();
      const std::string line = "line '" + sheet.lines.front().id + "'";
      EXPECT_NE(std::string::npos, std::string(e.what()).find(line)) << e.what();
    }
  }

  TEST(MapSceneBuilderTest, RefusesAnUnknownSymbolNamingIt)
  {
    HexXml::SheetDoc sheet = loadSheet("d-day-at-tarawa");
    const auto withGlyph =
        std::find_if(sheet.hexes.begin(), sheet.hexes.end(),
                     [](const HexXml::SheetHexDoc& h) { return !h.glyphs.empty(); });
    ASSERT_NE(sheet.hexes.end(), withGlyph);
    withGlyph->glyphs.front().symbol = "windmill";
    const MapFrame frame = MapFrame::of(sheet);
    const SymbolLibrary lib = SymbolLibrary::reference();
    Scene scene(frame.width(), frame.height());
    try {
      MapSceneBuilder(frame, lib, MapStyle::reference()).build(sheet, scene);
      FAIL() << "no throw";
    }
    catch (const std::invalid_argument& e) {
      EXPECT_NE(std::string::npos, std::string(e.what()).find("windmill")) << e.what();
      EXPECT_NE(std::string::npos, std::string(e.what()).find(withGlyph->id)) << e.what();
    }
  }

  TEST(MapSceneBuilderTest, RefusesAMarkMissingFromTheLegendNamingIt)
  {
    HexXml::SheetDoc sheet = loadSheet("velikiye-luki");
    ASSERT_FALSE(sheet.legend.empty());
    sheet.legend.erase(std::remove_if(sheet.legend.begin(), sheet.legend.end(),
                                      [](const HexXml::SheetMarkDoc& m) { return "lake" == m.id; }),
                       sheet.legend.end());
    const MapFrame frame = MapFrame::of(sheet);
    const SymbolLibrary lib = SymbolLibrary::reference();
    Scene scene(frame.width(), frame.height());
    try {
      MapSceneBuilder(frame, lib, MapStyle::reference()).build(sheet, scene);
      FAIL() << "no throw";
    }
    catch (const std::invalid_argument& e) {
      EXPECT_NE(std::string::npos, std::string(e.what()).find("mark 'lake'")) << e.what();
    }
  }

  TEST(MapSceneBuilderTest, MapStyleReferenceRoundsOnlyRivers)
  {
    // PGG draws its rivers along hexsides (TRC's now run centre to centre, as links).
    const Built rounded = build("panzergruppe-guderian");
    const auto& edges = rounded.scene.layer(Layer::Edges);
    EXPECT_LT(0, std::count_if(edges.begin(), edges.end(), hasQuadP));  // river paths, rounded
    const Built straight = build("panzergruppe-guderian", MapStyle{{}, true});
    const auto& plain = straight.scene.layer(Layer::Edges);
    EXPECT_EQ(0, std::count_if(plain.begin(), plain.end(), hasQuadP));
    EXPECT_GT(plain.size(), edges.size());  // each river hexside drawn on its own
  }

  TEST(MapSceneBuilderTest, BuildingsAreFourOrFiveAndRepeatable)
  {
    for (const char* hex : {"0101", "0604", "KK19", "w5227"}) {
      const std::vector<Box> a = scatterBuildings("vl", hex, {100, 100}, 50);
      const std::vector<Box> b = scatterBuildings("vl", hex, {100, 100}, 50);
      ASSERT_TRUE(4 == a.size() || 5 == a.size()) << hex;
      ASSERT_EQ(a.size(), b.size());
      for (std::size_t k = 0; k < a.size(); ++k) {
        EXPECT_EQ(a[k].x0, b[k].x0);
        EXPECT_LE(std::abs((a[k].x0 + a[k].x1) / 2 - 100), 0.55 * 50 + 1e-9);
        EXPECT_LE(std::abs((a[k].y0 + a[k].y1) / 2 - 100), 0.48 * 50 + 1e-9);
      }
    }
    // the documented seed: FNV-1a 64 and SplitMix64 as published
    EXPECT_EQ(0xcbf29ce484222325ull, fnv1a64(""));
    EXPECT_EQ(0xaf63dc4c8601ec8cull, fnv1a64("a"));
    EXPECT_EQ(0xe220a8397b1dcdafull, splitMix64(0));
  }

  TEST(MapSceneBuilderTest, SvgIsWellFormedXmlAndJsonNamesEveryLayer)
  {
    const Built b = build("velikiye-luki");
    const std::filesystem::path svg = outDir() / "velikiye-luki.wellformed.svg";
    writeFile(svg, writeSvg(b.scene, b.lib, b.sheet.title));
    const HexXml::XmlDocument doc = HexXml::XmlDocument::load(svg);
    EXPECT_EQ("svg", doc.root().name());
    const std::string json = writeJson(b.scene, b.lib);
    for (const char* layer : {"\"terrain\"", "\"links\"", "\"hexglyphs\"", "\"overlay\""}) {
      EXPECT_NE(std::string::npos, json.find(layer)) << layer;
    }
    EXPECT_NE(std::string::npos, json.find("\"hit\":{\"hex\":"));
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
