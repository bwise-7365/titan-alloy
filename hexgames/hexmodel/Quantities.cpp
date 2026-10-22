// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/Quantities.h"

#include <charconv>
#include <cmath>
#include <stdexcept>

namespace HexModel {

  namespace {

    int
    parseInt(std::string_view text)
    {
      if (text.empty()) {
        throw std::invalid_argument("TurnSelector::parse: malformed selector (empty number)");
      }
      int value = 0;
      const auto [ptr, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
      if (std::errc() != ec || ptr != text.data() + text.size()) {
        throw std::invalid_argument("TurnSelector::parse: malformed selector: '" + std::string(text) + "'");
      }
      return value;
    }

    double
    asRatio(Odds o)
    {
      return static_cast<double>(o.attacker) / static_cast<double>(o.defender);
    }

  }  // namespace

  Steps::Steps(int current, int maximum) : current_(current), maximum_(maximum)
  {
    if (current < 1 || current > maximum) {
      throw std::invalid_argument("Steps: current must satisfy 1 <= current <= maximum");
    }
    return;
  }

  std::optional<Steps>
  Steps::reduced() const
  {
    if (1 == current_) {
      return std::nullopt;
    }
    return Steps(current_ - 1, maximum_);
  }

  OddsOutcome
  makeOdds(Strength attack, Strength defence, Rounding rounding, Odds minimum, Odds maximum)
  {
    if (0 == attack.value || 0 == defence.value) {
      throw std::invalid_argument("makeOdds: attack and defence strengths must both be nonzero");
    }

    Odds odds;
    if (attack.value == defence.value) {
      odds = Odds{1, 1};
    } else if (attack.value > defence.value) {
      const double ratio = static_cast<double>(attack.value) / static_cast<double>(defence.value);
      int n = 1;
      switch (rounding) {
        case Rounding::Defender:
          n = static_cast<int>(std::floor(ratio));
          break;
        case Rounding::Attacker:
          n = static_cast<int>(std::ceil(ratio));
          break;
        case Rounding::None:
          n = static_cast<int>(std::lround(ratio));
          break;
      }
      odds = Odds{n < 1 ? 1 : n, 1};
    } else {
      const double ratio = static_cast<double>(defence.value) / static_cast<double>(attack.value);
      int m = 1;
      switch (rounding) {
        case Rounding::Defender:
          m = static_cast<int>(std::ceil(ratio));
          break;
        case Rounding::Attacker:
          m = static_cast<int>(std::floor(ratio));
          break;
        case Rounding::None:
          m = static_cast<int>(std::lround(ratio));
          break;
      }
      odds = Odds{1, m < 1 ? 1 : m};
    }

    const double value = asRatio(odds);
    if (value < asRatio(minimum)) {
      return BelowMinimum{};
    }
    if (value > asRatio(maximum)) {
      return AboveMaximum{};
    }
    return odds;
  }

  TurnSelector
  TurnSelector::parse(std::string_view text)
  {
    if (text.empty()) {
      throw std::invalid_argument("TurnSelector::parse: empty selector");
    }

    TurnSelector selector;

    if ('+' == text.back()) {
      const int first = parseInt(text.substr(0, text.size() - 1));
      selector.ranges_.push_back(Range{first, std::nullopt});
      return selector;
    }

    std::size_t pos = 0;
    while (true) {
      const std::size_t comma = text.find(',', pos);
      const std::string_view token =
          text.substr(pos, std::string_view::npos == comma ? std::string_view::npos : comma - pos);
      const std::size_t dash = token.find('-');
      if (std::string_view::npos == dash) {
        const int n = parseInt(token);
        selector.ranges_.push_back(Range{n, n});
      } else {
        const int first = parseInt(token.substr(0, dash));
        const int last = parseInt(token.substr(dash + 1));
        if (last < first) {
          throw std::invalid_argument("TurnSelector::parse: malformed range (end before start): '" +
                                       std::string(token) + "'");
        }
        selector.ranges_.push_back(Range{first, last});
      }
      if (std::string_view::npos == comma) {
        break;
      }
      pos = comma + 1;
    }

    return selector;
  }

  TurnSelector
  TurnSelector::all()
  {
    TurnSelector selector;
    selector.ranges_.push_back(Range{0, std::nullopt});
    return selector;
  }

  bool
  TurnSelector::containsP(int turn) const
  {
    for (const Range& r : ranges_) {
      if (turn >= r.first && (!r.last || turn <= *r.last)) {
        return true;
      }
    }
    return false;
  }

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
