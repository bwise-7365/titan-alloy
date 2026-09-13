// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Document-level golden comparison: no Session, no HexModel::Position -- just two SaveModels and
// their canonical texts.
// ----------------------------------------------
#include "hexrecord/GoldenCompare.h"

#include <algorithm>
#include <vector>

namespace HexRecord {

  namespace {

    std::vector<std::string>
    splitLines(const std::string& text)
    {
      std::vector<std::string> lines;
      std::size_t start = 0;
      while (start <= text.size()) {
        const std::size_t nl = text.find('\n', start);
        if (std::string::npos == nl) {
          if (start < text.size()) {
            lines.push_back(text.substr(start));
          }
          break;
        }
        lines.push_back(text.substr(start, nl - start));
        start = nl + 1;
      }
      return lines;
    }

  }  // namespace

  std::optional<Divergence>
  compareLogs(const SaveModel& golden, const SaveModel& actual)
  {
    const std::size_t n = std::min(golden.log.size(), actual.log.size());
    for (std::size_t i = 0; i < n; ++i) {
      const SaveMove& g = golden.log[i];
      const SaveMove& a = actual.log[i];

      const std::string gOutcome = g.result.has_value() ? g.result->outcome : std::string();
      const std::string aOutcome = a.result.has_value() ? a.result->outcome : std::string();
      if (gOutcome != aOutcome) {
        return Divergence{g.n, "outcome", gOutcome, aOutcome};
      }

      const std::size_t dn = std::min(g.draws.size(), a.draws.size());
      for (std::size_t j = 0; j < dn; ++j) {
        const std::string gd = renderDraw(g.draws[j]);
        const std::string ad = renderDraw(a.draws[j]);
        if (gd != ad) {
          return Divergence{g.n, "draw", gd, ad};
        }
      }
      if (g.draws.size() != a.draws.size()) {
        return Divergence{g.n, "draw", std::to_string(g.draws.size()) + " draws",
                          std::to_string(a.draws.size()) + " draws"};
      }

      const std::size_t en = std::min(g.events.size(), a.events.size());
      for (std::size_t j = 0; j < en; ++j) {
        const std::string ge = renderEvent(g.events[j]);
        const std::string ae = renderEvent(a.events[j]);
        if (ge != ae) {
          return Divergence{g.n, "event", ge, ae};
        }
      }
      if (g.events.size() != a.events.size()) {
        return Divergence{g.n, "event", std::to_string(g.events.size()) + " events",
                          std::to_string(a.events.size()) + " events"};
      }
    }
    if (golden.log.size() != actual.log.size()) {
      const int moveNumber = static_cast<int>(n) + 1;
      return Divergence{moveNumber, "outcome", std::to_string(golden.log.size()) + " moves",
                        std::to_string(actual.log.size()) + " moves"};
    }
    return std::nullopt;
  }

  std::string
  unifiedDiff(const std::string& expected, const std::string& actual, std::size_t maxLines)
  {
    const std::vector<std::string> a = splitLines(expected);
    const std::vector<std::string> b = splitLines(actual);
    const std::size_t n = a.size();
    const std::size_t m = b.size();

    std::vector<std::vector<int>> lcs(n + 1, std::vector<int>(m + 1, 0));
    for (std::size_t i = n; i-- > 0;) {
      for (std::size_t j = m; j-- > 0;) {
        lcs[i][j] = a[i] == b[j] ? lcs[i + 1][j + 1] + 1 : std::max(lcs[i + 1][j], lcs[i][j + 1]);
      }
    }

    std::vector<std::string> out;
    std::size_t i = 0;
    std::size_t j = 0;
    while (i < n && j < m && out.size() < maxLines) {
      if (a[i] == b[j]) {
        out.push_back("  " + a[i]);
        ++i;
        ++j;
      }
      else if (lcs[i + 1][j] >= lcs[i][j + 1]) {
        out.push_back("- " + a[i]);
        ++i;
      }
      else {
        out.push_back("+ " + b[j]);
        ++j;
      }
    }
    while (i < n && out.size() < maxLines) {
      out.push_back("- " + a[i]);
      ++i;
    }
    while (j < m && out.size() < maxLines) {
      out.push_back("+ " + b[j]);
      ++j;
    }
    if (i < n || j < m) {
      out.push_back("... (diff truncated)");
    }

    std::string result;
    for (const std::string& line : out) {
      result += line;
      result += '\n';
    }
    return result;
  }

  std::filesystem::path
  actualPathFor(const std::filesystem::path& goldenPath)
  {
    std::string name = goldenPath.filename().string();
    const std::string goldenSuffix = ".golden.xml";
    if (name.size() > goldenSuffix.size() &&
        0 == name.compare(name.size() - goldenSuffix.size(), goldenSuffix.size(), goldenSuffix)) {
      name = name.substr(0, name.size() - goldenSuffix.size()) + ".actual.xml";
    }
    else {
      name = goldenPath.stem().string() + ".actual" + goldenPath.extension().string();
    }
    return goldenPath.parent_path() / name;
  }

  GoldenReport
  buildGoldenReport(const SaveModel& golden, const SaveModel& actual, const std::filesystem::path& goldenPath)
  {
    const std::filesystem::path actualPath = actualPathFor(goldenPath);
    writeCanonical(actual, actualPath);

    const std::string goldenText = canonicalText(golden);
    const std::string actualText = canonicalText(actual);

    GoldenReport report;
    report.actualWritten = actualPath;
    report.matchP = goldenText == actualText;
    if (!report.matchP) {
      report.first = compareLogs(golden, actual);
      report.unifiedDiff = unifiedDiff(goldenText, actualText);
    }
    return report;
  }

  std::string
  formatGoldenReport(const GoldenReport& report, std::string_view game, std::string_view slug)
  {
    std::string out;
    if (report.matchP) {
      out += "golden match: ";
      out += game;
      out += " ";
      out += slug;
      out += "\n";
      return out;
    }
    out += "golden mismatch: ";
    out += game;
    out += " ";
    out += slug;
    out += "\n";
    if (report.first.has_value()) {
      out += "  move " + std::to_string(report.first->moveNumber) + " " + report.first->what + ": golden=\"" +
             report.first->expected + "\" actual=\"" + report.first->actual + "\"\n";
    }
    out += report.unifiedDiff;
    out += "re-bless: tools\\bless-goldens.ps1 -Game ";
    out += game;
    out += " -Name ";
    out += slug;
    out += "\n";
    return out;
  }

}  // namespace HexRecord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
