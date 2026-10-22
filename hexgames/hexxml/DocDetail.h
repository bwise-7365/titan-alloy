// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Shared, header-only parsing helpers for the five document models (RulesDoc, SheetDoc,
// CounterSetDoc, PackageDoc, SaveDoc). Not part of the public facade; each *.cpp includes this and
// nothing outside hexxml includes it.
// ----------------------------------------------
#pragma once
#include "hexxml/XmlDocument.h"

#include <cctype>
#include <initializer_list>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace HexXml::Detail {

  inline std::vector<std::string>
  splitTokens(const std::string& s)
  {
    std::vector<std::string> result;
    std::size_t i = 0;
    while (i < s.size()) {
      while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
        ++i;
      }
      std::size_t begin = i;
      while (i < s.size() && !std::isspace(static_cast<unsigned char>(s[i]))) {
        ++i;
      }
      if (i > begin) {
        result.push_back(s.substr(begin, i - begin));
      }
    }
    return result;
  }

  // An IDREFS-shaped attribute (space separated tokens), empty if absent.
  inline std::vector<std::string>
  idrefs(const XmlNode& node, std::string_view attr)
  {
    const std::optional<std::string> raw = node.optional(attr);
    if (!raw) {
      return {};
    }
    return splitTokens(*raw);
  }

  // A required element child; throws std::invalid_argument naming file:line and the missing element,
  // never std::bad_optional_access, so every "required" spot in a parser reads the same way.
  inline XmlNode
  requiredChild(const XmlNode& node, std::string_view name)
  {
    if (const std::optional<XmlNode> c = node.child(name)) {
      return *c;
    }
    throw std::invalid_argument(node.file() + ":" + std::to_string(node.line()) + ": " + node.name() +
                                 ": missing required child element '" + std::string(name) + "'");
  }

  inline void
  checkEnum(const XmlNode& node, std::string_view attr, const std::string& value,
            std::initializer_list<std::string_view> allowed)
  {
    for (const std::string_view& a : allowed) {
      if (a == value) {
        return;
      }
    }
    throw std::invalid_argument(node.file() + ":" + std::to_string(node.line()) + ": " + node.name() +
                                 ": attribute '" + std::string(attr) + "' has an invalid value '" +
                                 value + "'");
  }

  inline void
  requireExactly(const XmlNode& node, std::size_t rowLen, std::size_t colCount, const std::string& rowLabel,
                  const std::string& tableId)
  {
    if (rowLen != colCount) {
      throw std::invalid_argument(node.file() + ":" + std::to_string(node.line()) + ": table '" + tableId +
                                   "': row '" + rowLabel + "' has " + std::to_string(rowLen) +
                                   " cells, expected " + std::to_string(colCount));
    }
  }

}  // namespace HexXml::Detail
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
