// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexxml/XmlDocument.h"

#include <tinyxml2.h>

#include <cctype>
#include <stdexcept>

namespace HexXml {

  namespace {

    std::string
    trimmed(std::string_view s)
    {
      std::size_t begin = 0;
      while (begin < s.size() && 0 != std::isspace(static_cast<unsigned char>(s[begin]))) {
        ++begin;
      }
      std::size_t end = s.size();
      while (end > begin && 0 != std::isspace(static_cast<unsigned char>(s[end - 1]))) {
        --end;
      }
      return std::string(s.substr(begin, end - begin));
    }

    [[noreturn]] void
    throwAt(const XmlNode& node, const std::string& message)
    {
      throw std::invalid_argument(node.file() + ":" + std::to_string(node.line()) + ": " + node.name() +
                                   ": " + message);
    }

    template <class T>
    T
    convert(const XmlNode& node, std::string_view attr, const std::string& raw);

    template <>
    std::string
    convert<std::string>(const XmlNode&, std::string_view, const std::string& raw)
    {
      return raw;
    }

    template <>
    int
    convert<int>(const XmlNode& node, std::string_view attr, const std::string& raw)
    {
      try {
        std::size_t consumed = 0;
        const int value = std::stoi(raw, &consumed);
        if (consumed != raw.size()) {
          throw std::invalid_argument("");
        }
        return value;
      } catch (const std::exception&) {
        throwAt(node, "attribute '" + std::string(attr) + "' is not an integer: '" + raw + "'");
      }
    }

    template <>
    double
    convert<double>(const XmlNode& node, std::string_view attr, const std::string& raw)
    {
      try {
        std::size_t consumed = 0;
        const double value = std::stod(raw, &consumed);
        if (consumed != raw.size()) {
          throw std::invalid_argument("");
        }
        return value;
      } catch (const std::exception&) {
        throwAt(node, "attribute '" + std::string(attr) + "' is not a number: '" + raw + "'");
      }
    }

    template <>
    bool
    convert<bool>(const XmlNode& node, std::string_view attr, const std::string& raw)
    {
      if ("true" == raw) {
        return true;
      }
      if ("false" == raw) {
        return false;
      }
      throwAt(node, "attribute '" + std::string(attr) + "' is not a boolean (true/false): '" + raw + "'");
    }

    template <>
    std::uint64_t
    convert<std::uint64_t>(const XmlNode& node, std::string_view attr, const std::string& raw)
    {
      try {
        std::size_t consumed = 0;
        const unsigned long long value = std::stoull(raw, &consumed);
        if (consumed != raw.size()) {
          throw std::invalid_argument("");
        }
        return static_cast<std::uint64_t>(value);
      } catch (const std::exception&) {
        throwAt(node, "attribute '" + std::string(attr) + "' is not an unsigned integer: '" + raw + "'");
      }
    }

  }  // namespace

  XmlNode::XmlNode(const tinyxml2::XMLElement* elem, std::shared_ptr<const std::string> file)
      : elem_(elem), file_(std::move(file))
  {
  }

  std::string
  XmlNode::name() const
  {
    const char* n = elem_->Name();
    return nullptr != n ? std::string(n) : std::string();
  }

  int
  XmlNode::line() const
  {
    return elem_->GetLineNum();
  }

  std::string
  XmlNode::file() const
  {
    return *file_;
  }

  std::string
  XmlNode::required(std::string_view attr) const
  {
    const std::string attrName(attr);
    const char* v = elem_->Attribute(attrName.c_str());
    if (nullptr == v) {
      throwAt(*this, "missing attribute '" + attrName + "'");
    }
    return std::string(v);
  }

  std::optional<std::string>
  XmlNode::optional(std::string_view attr) const
  {
    const std::string attrName(attr);
    const char* v = elem_->Attribute(attrName.c_str());
    if (nullptr == v) {
      return std::nullopt;
    }
    return std::string(v);
  }

  template <class T>
  T
  XmlNode::requiredAs(std::string_view attr) const
  {
    return convert<T>(*this, attr, required(attr));
  }

  template <class T>
  std::optional<T>
  XmlNode::optionalAs(std::string_view attr) const
  {
    const std::optional<std::string> raw = optional(attr);
    if (!raw) {
      return std::nullopt;
    }
    return convert<T>(*this, attr, *raw);
  }

  template std::string XmlNode::requiredAs<std::string>(std::string_view) const;
  template int XmlNode::requiredAs<int>(std::string_view) const;
  template double XmlNode::requiredAs<double>(std::string_view) const;
  template bool XmlNode::requiredAs<bool>(std::string_view) const;
  template std::uint64_t XmlNode::requiredAs<std::uint64_t>(std::string_view) const;

  template std::optional<std::string> XmlNode::optionalAs<std::string>(std::string_view) const;
  template std::optional<int> XmlNode::optionalAs<int>(std::string_view) const;
  template std::optional<double> XmlNode::optionalAs<double>(std::string_view) const;
  template std::optional<bool> XmlNode::optionalAs<bool>(std::string_view) const;
  template std::optional<std::uint64_t> XmlNode::optionalAs<std::uint64_t>(std::string_view) const;

  std::vector<XmlNode>
  XmlNode::children() const
  {
    std::vector<XmlNode> result;
    for (const tinyxml2::XMLElement* c = elem_->FirstChildElement(); nullptr != c;
         c = c->NextSiblingElement()) {
      result.push_back(XmlNode(c, file_));
    }
    return result;
  }

  std::vector<XmlNode>
  XmlNode::children(std::string_view name) const
  {
    const std::string wanted(name);
    std::vector<XmlNode> result;
    for (const tinyxml2::XMLElement* c = elem_->FirstChildElement(wanted.c_str()); nullptr != c;
         c = c->NextSiblingElement(wanted.c_str())) {
      result.push_back(XmlNode(c, file_));
    }
    return result;
  }

  std::optional<XmlNode>
  XmlNode::child(std::string_view name) const
  {
    const std::string wanted(name);
    const tinyxml2::XMLElement* c = elem_->FirstChildElement(wanted.c_str());
    if (nullptr == c) {
      return std::nullopt;
    }
    return XmlNode(c, file_);
  }

  std::string
  XmlNode::text() const
  {
    const char* t = elem_->GetText();
    if (nullptr == t) {
      return std::string();
    }
    return trimmed(t);
  }

  XmlDocument
  XmlDocument::load(const std::filesystem::path& path)
  {
    auto doc = std::make_shared<tinyxml2::XMLDocument>();
    const std::string pathStr = path.string();
    const tinyxml2::XMLError err = doc->LoadFile(pathStr.c_str());
    if (tinyxml2::XML_SUCCESS != err) {
      const int line = doc->ErrorLineNum();
      const char* errStr = doc->ErrorStr();
      throw std::invalid_argument(pathStr + ":" + std::to_string(line) + ": " +
                                   (nullptr != errStr ? errStr : "XML parse error"));
    }
    XmlDocument result;
    result.doc_ = doc;
    result.file_ = std::make_shared<const std::string>(pathStr);
    return result;
  }

  XmlNode
  XmlDocument::root() const
  {
    const tinyxml2::XMLElement* r = doc_->RootElement();
    if (nullptr == r) {
      throw std::invalid_argument(*file_ + ":0: document has no root element");
    }
    return XmlNode(r, file_);
  }

}  // namespace HexXml
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
