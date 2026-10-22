// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The hexxml facade: a thin, non-owning view over a parsed XML document. Every hexxml document model
// is built through this API alone; only the .cpp files of this module include <tinyxml2.h>, so no
// caller ever names a TinyXML2 type. Every failure throws std::invalid_argument naming "file:line" and
// the offending element or attribute.
// ----------------------------------------------
#pragma once
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace tinyxml2 {
  class XMLDocument;
  class XMLElement;
}  // namespace tinyxml2

namespace HexXml {

  // A non-owning view of one element. Copies share the same underlying document, which the owning
  // XmlDocument keeps alive; an XmlNode must not outlive the XmlDocument it came from.
  class XmlNode {
  public:
    std::string name() const;
    int line() const;

    // Throws std::invalid_argument "file:line: element: missing attribute 'x'" if absent.
    std::string required(std::string_view attr) const;
    std::optional<std::string> optional(std::string_view attr) const;

    // T in int, double, bool, std::string, std::uint64_t. Throws naming file:line and the attribute
    // if the attribute is missing (requiredAs) or its text does not parse as T. Booleans accept only
    // "true" and "false".
    template <class T>
    T requiredAs(std::string_view attr) const;
    template <class T>
    std::optional<T> optionalAs(std::string_view attr) const;

    std::vector<XmlNode> children() const;              // element children, document order
    std::vector<XmlNode> children(std::string_view name) const;
    std::optional<XmlNode> child(std::string_view name) const;  // first, if any

    std::string text() const;  // trimmed element text, "" if none
    std::string file() const;

  private:
    friend class XmlDocument;
    XmlNode(const tinyxml2::XMLElement* elem, std::shared_ptr<const std::string> file);

    const tinyxml2::XMLElement* elem_ = nullptr;
    std::shared_ptr<const std::string> file_;
  };

  class XmlDocument {
  public:
    // Throws std::invalid_argument on a parse error, naming the file and the line TinyXML2 reports.
    static XmlDocument load(const std::filesystem::path&);
    XmlNode root() const;

  private:
    std::shared_ptr<tinyxml2::XMLDocument> doc_;
    std::shared_ptr<const std::string> file_;
  };

  extern template std::string XmlNode::requiredAs<std::string>(std::string_view) const;
  extern template int XmlNode::requiredAs<int>(std::string_view) const;
  extern template double XmlNode::requiredAs<double>(std::string_view) const;
  extern template bool XmlNode::requiredAs<bool>(std::string_view) const;
  extern template std::uint64_t XmlNode::requiredAs<std::uint64_t>(std::string_view) const;

  extern template std::optional<std::string> XmlNode::optionalAs<std::string>(std::string_view) const;
  extern template std::optional<int> XmlNode::optionalAs<int>(std::string_view) const;
  extern template std::optional<double> XmlNode::optionalAs<double>(std::string_view) const;
  extern template std::optional<bool> XmlNode::optionalAs<bool>(std::string_view) const;
  extern template std::optional<std::uint64_t> XmlNode::optionalAs<std::uint64_t>(std::string_view) const;

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
