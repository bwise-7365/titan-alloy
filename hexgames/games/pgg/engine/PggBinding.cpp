// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggBinding.h"

#include <charconv>
#include <stdexcept>
#include <string>
#include <vector>

namespace Pgg {

  namespace {

    [[noreturn]] void
    refuse(std::string_view line)
    {
      throw std::invalid_argument("Pgg::valueLine: '" + std::string(line) +
                                  "' is not a PGG value line (9-7, 2-4-6, U-6 or (3)*10)");
    }

    int
    numberOf(std::string_view text, std::string_view line)
    {
      int value = 0;
      const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
      if (std::errc() != error || text.data() + text.size() != end || text.empty()) {
        refuse(line);
      }
      return value;
    }

    std::vector<std::string_view>
    fieldsOf(std::string_view line)
    {
      std::vector<std::string_view> out;
      std::size_t start = 0;
      for (std::size_t dash = line.find('-'); std::string_view::npos != dash; dash = line.find('-', start)) {
        out.push_back(line.substr(start, dash - start));
        start = dash + 1;
      }
      out.push_back(line.substr(start));
      return out;
    }

    HexModel::Strengths
    leaderLine(std::string_view line)
    {
      const std::size_t close = line.find(')');
      if (std::string_view::npos == close) {
        refuse(line);
      }
      std::string_view rest = line.substr(close + 1);
      const std::string_view star = "\xE2\x98\x85";  // U+2605, as printed
      if (rest.starts_with(star)) {
        rest.remove_prefix(star.size());
      } else if (rest.starts_with("*")) {
        rest.remove_prefix(1);
      } else {
        refuse(line);
      }
      const int rating = numberOf(line.substr(1, close - 1), line);
      HexModel::Strengths out;
      out.defence = HexModel::Strength{rating};
      out.range = rating;
      out.allowance = HexModel::MovementPoints::whole(numberOf(rest, line));
      return out;
    }

  }  // namespace

  HexModel::Strengths
  valueLine(std::string_view line, HexModel::UnitKind)
  {
    if (line.starts_with("(")) {
      return leaderLine(line);
    }
    const std::vector<std::string_view> fields = fieldsOf(line);
    HexModel::Strengths out;
    if (2 == fields.size() && "U" == fields[0]) {
      out.allowance = HexModel::MovementPoints::whole(numberOf(fields[1], line));
      return out;
    }
    if (2 == fields.size()) {
      const int combat = numberOf(fields[0], line);
      out.attack = HexModel::Strength{combat};
      out.defence = HexModel::Strength{combat};
      out.allowance = HexModel::MovementPoints::whole(numberOf(fields[1], line));
      return out;
    }
    if (3 == fields.size()) {
      out.attack = HexModel::Strength{numberOf(fields[0], line)};
      out.defence = HexModel::Strength{numberOf(fields[1], line)};
      out.allowance = HexModel::MovementPoints::whole(numberOf(fields[2], line));
      return out;
    }
    refuse(line);
  }

  std::shared_ptr<const HexRules::GameDefinition>
  loadPackage(const std::filesystem::path& manifest)
  {
    return std::make_shared<const HexRules::GameDefinition>(HexRules::PackageLoader::load(manifest, &valueLine));
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
