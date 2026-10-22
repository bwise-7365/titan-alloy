// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/XmlDocument.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <stdexcept>

namespace {

  std::filesystem::path
  fixture(const char* name)
  {
    return std::filesystem::path(HEXGAMES_SOURCE_DIR) / "hexxml" / "test" / name;
  }

}  // namespace

TEST(XmlDocumentTest, LoadsAndReadsAttributesAndChildren)
{
  const HexXml::XmlDocument doc = HexXml::XmlDocument::load(fixture("fixture.xml"));
  const HexXml::XmlNode root = doc.root();
  EXPECT_EQ("root", root.name());
  EXPECT_EQ("fixture", root.required("name"));
  EXPECT_EQ(3, root.requiredAs<int>("count"));
  EXPECT_DOUBLE_EQ(1.5, root.requiredAs<double>("ratio"));
  EXPECT_TRUE(root.requiredAs<bool>("flag"));
  EXPECT_FALSE(root.optional("missing").has_value());
  EXPECT_FALSE(root.optionalAs<int>("missing").has_value());

  const std::vector<HexXml::XmlNode> all = root.children();
  EXPECT_EQ(3u, all.size());

  const std::vector<HexXml::XmlNode> childOnly = root.children("child");
  ASSERT_EQ(2u, childOnly.size());
  EXPECT_EQ("a", childOnly[0].required("id"));
  EXPECT_EQ("first", childOnly[0].text());
  EXPECT_EQ("second, trimmed", childOnly[1].text());

  const std::optional<HexXml::XmlNode> firstChild = root.child("child");
  ASSERT_TRUE(firstChild.has_value());
  EXPECT_EQ("a", firstChild->required("id"));

  const std::optional<HexXml::XmlNode> none = root.child("nope");
  EXPECT_FALSE(none.has_value());

  const HexXml::XmlNode other = root.children("other").front();
  EXPECT_EQ("", other.text());
}

TEST(XmlDocumentTest, MissingAttributeThrowsWithFileAndLine)
{
  const HexXml::XmlDocument doc = HexXml::XmlDocument::load(fixture("fixture.xml"));
  const HexXml::XmlNode root = doc.root();
  try {
    (void)root.required("nope");
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string message = e.what();
    EXPECT_NE(std::string::npos, message.find("fixture.xml"));
    EXPECT_NE(std::string::npos, message.find("root"));
    EXPECT_NE(std::string::npos, message.find("nope"));
  }
}

TEST(XmlDocumentTest, MalformedBooleanThrowsNamingTheAttribute)
{
  const HexXml::XmlDocument doc = HexXml::XmlDocument::load(fixture("fixture.xml"));
  const HexXml::XmlNode root = doc.root();
  try {
    (void)root.requiredAs<bool>("name");
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string message = e.what();
    EXPECT_NE(std::string::npos, message.find("name"));
  }
}

TEST(XmlDocumentTest, ParseErrorNamesFileAndLine)
{
  try {
    (void)HexXml::XmlDocument::load(fixture("malformed.xml"));
    FAIL() << "expected std::invalid_argument";
  } catch (const std::invalid_argument& e) {
    const std::string message = e.what();
    EXPECT_NE(std::string::npos, message.find("malformed.xml"));
    EXPECT_NE(std::string::npos, message.find(':'));
  }
}
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
