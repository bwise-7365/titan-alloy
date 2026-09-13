// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Quantities the rules reason about, each its own type so that a movement budget cannot be added to
// a combat strength, and each carrying its rules-level invariant.
// ----------------------------------------------
#pragma once
#include <compare>
#include <cstdint>
#include <optional>
#include <string_view>
#include <variant>
#include <vector>

namespace HexModel {

  struct Strength {
    int value = 0;
    constexpr auto operator<=>(const Strength&) const = default;
    friend constexpr Strength operator+(Strength l, Strength r) { return {l.value + r.value}; }
  };

  // Movement points in halves, because Dai Senso and PGG charge half a point for a road hexside.
  // TODO(decide): a rational if a sixth game needs thirds.
  struct MovementPoints {
    int halves = 0;
    constexpr auto operator<=>(const MovementPoints&) const = default;
    friend constexpr MovementPoints operator+(MovementPoints l, MovementPoints r) { return {l.halves + r.halves}; }
    static constexpr MovementPoints whole(int n) { return {2 * n}; }
    static constexpr MovementPoints half() { return {1}; }
  };

  struct HexCount {
    int value = 0;
    constexpr auto operator<=>(const HexCount&) const = default;
    friend constexpr HexCount operator+(HexCount l, HexCount r) { return {l.value + r.value}; }
  };

  struct Actions {
    int value = 0;
    constexpr auto operator<=>(const Actions&) const = default;
  };

  // What a move spends, chosen by movement/@budget.
  using Budget = std::variant<MovementPoints, HexCount, Actions>;

  // Step strength of a counter: 1 <= current <= maximum, checked once at construction.
  class Steps {
  public:
    // Throws std::invalid_argument unless 1 <= current <= maximum.
    Steps(int current, int maximum);
    int current() const { return current_; }
    int maximum() const { return maximum_; }
    // One step lost; nullopt means eliminated.
    std::optional<Steps> reduced() const;
    bool fullP() const { return current_ == maximum_; }
    auto operator<=>(const Steps&) const = default;

  private:
    int current_;
    int maximum_;
  };

  enum class Rounding : std::uint8_t { Defender, Attacker, None };

  // A combat odds ratio with one side equal to 1, e.g. 3:1 or 1:2.
  struct Odds {
    int attacker = 1;
    int defender = 1;
    constexpr auto operator<=>(const Odds&) const = default;
  };
  struct BelowMinimum {};
  struct AboveMaximum {};
  using OddsOutcome = std::variant<Odds, BelowMinimum, AboveMaximum>;

  // Reduce two strengths to a ratio, rounding in the named side's favour (3:2 -> 1:1 for the
  // defender), then clamp to the resolver's columns; a ratio outside them is reported, not clamped
  // silently, because the rules attach effects to it (TRC: surrender below 1-6).
  OddsOutcome makeOdds(Strength attack, Strength defence, Rounding, Odds minimum, Odds maximum);

  // hexrules move-cost: a number, or one of four words.
  struct Prohibited {};
  struct Entire {};
  struct OtherTerrain {};
  struct NoCost {};
  using MoveCost = std::variant<MovementPoints, Prohibited, Entire, OtherTerrain, NoCost>;

  // hexrules Extent: a hex count, unlimited, or authored (printed on the map, not computed).
  struct Unlimited {};
  struct Authored {};
  using Extent = std::variant<HexCount, Unlimited, Authored>;

  // hexrules turns selector: "5", "1-10", "11+", "3,5,7,9". Parsed once, queried often.
  class TurnSelector {
  public:
    // Throws std::invalid_argument on a malformed selector.
    static TurnSelector parse(std::string_view);
    static TurnSelector all();
    bool containsP(int turn) const;

  private:
    struct Range {
      int first;
      std::optional<int> last;  // nullopt: open-ended
    };
    std::vector<Range> ranges_;
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
