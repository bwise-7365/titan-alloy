// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The expected names are read out of hexsheet2svg.py itself (the keys of its SYMBOLS and
// MARK_SHAPES dictionaries), so the test cannot drift from the reference.
// ----------------------------------------------
#include "TestSupport.h"
#include "hexview/SymbolLibrary.h"

#include <gtest/gtest.h>

#include <algorithm>
#include <regex>
#include <set>

namespace {

  using namespace HexViewTest;

  // The quoted keys of the Python dict literal that starts at "<name> = {".
  std::set<std::string>
  dictKeys(const std::string& source, const std::string& name)
  {
    const std::size_t start = source.find(name + " = {");
    if (std::string::npos == start) {
      throw std::invalid_argument("hexsheet2svg.py has no " + name + " dict");
    }
    const std::size_t end = source.find("\n    }", start) < source.find("\n}", start)
                                ? source.find("\n    }", start)
                                : source.find("\n}", start);
    const std::string body = source.substr(start, end - start);
    const std::regex key(R"re(\n\s*"([a-z-]+)":)re");
    std::set<std::string> out;
    for (auto it = std::sregex_iterator(body.begin(), body.end(), key);
         std::sregex_iterator() != it; ++it) {
      out.insert((*it)[1].str());
    }
    return out;
  }

  std::string
  referenceSource()
  {
    return readFile(sourceRoot() / "map_graphics" / "xml" / "hexsheet2svg.py");
  }

  TEST(SymbolLibraryTest, HasEveryReferenceSymbol)
  {
    const std::string source = referenceSource();
    const std::set<std::string> symbols = dictKeys(source, "SYMBOLS");
    const std::set<std::string> marks = dictKeys(source, "MARK_SHAPES");
    ASSERT_EQ(29u, symbols.size());
    ASSERT_LE(8u, marks.size());
    std::set<std::string> expected = symbols;
    for (const std::string& m : marks) {
      expected.insert("mark-" + m);
    }
    const std::vector<std::string> names = HexView::SymbolLibrary::reference().names();
    EXPECT_EQ(expected, std::set<std::string>(names.begin(), names.end()));
    EXPECT_TRUE(std::is_sorted(names.begin(), names.end()));
  }

  TEST(SymbolLibraryTest, RefusesAnUnknownSymbolNamingIt)
  {
    const HexView::SymbolLibrary lib = HexView::SymbolLibrary::reference();
    EXPECT_FALSE(lib.containsP("windmill"));
    try {
      lib.symbol("windmill");
      FAIL() << "no throw";
    }
    catch (const std::invalid_argument& e) {
      EXPECT_NE(std::string::npos, std::string(e.what()).find("windmill"));
    }
  }

  TEST(SymbolLibraryTest, CurrentColourPartsCarryThePlaceholder)
  {
    const HexView::SymbolLibrary lib = HexView::SymbolLibrary::reference();
    const HexView::Symbol& city = lib.symbol("city");
    ASSERT_EQ(1u, city.body.size());
    const auto& shape = std::get<HexView::PathShape>(city.body[0].shape);
    EXPECT_EQ(HexView::kCurrentColor, shape.fill->color);
    EXPECT_EQ((HexView::Color{0x22, 0x22, 0x22, 255}), shape.stroke->color);
    EXPECT_TRUE(lib.symbol("text").body.empty());
  }

}  // namespace
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
