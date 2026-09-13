// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Toolchain smoke test: C++20 concepts, GoogleTest and TinyXML2 all link and run.
// ----------------------------------------------
#include <gtest/gtest.h>
#include <tinyxml2.h>

#include <concepts>

namespace {

  template <class T>
  concept Additive = requires(T a, T b) {
    { a + b } -> std::same_as<T>;
  };

  template <Additive T>
  T
  twice(T v)
  {
    return v + v;
  }

}  // namespace

TEST(Smoke, ConceptsCompileAndRun)
{
  EXPECT_EQ(6, twice(3));
}

TEST(Smoke, TinyXml2ParsesADocument)
{
  tinyxml2::XMLDocument doc;
  ASSERT_EQ(tinyxml2::XML_SUCCESS, doc.Parse("<game id='trc'><side id='axis'/></game>"));
  const tinyxml2::XMLElement* game = doc.RootElement();
  ASSERT_NE(nullptr, game);
  EXPECT_STREQ("trc", game->Attribute("id"));
  EXPECT_STREQ("axis", game->FirstChildElement("side")->Attribute("id"));
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
