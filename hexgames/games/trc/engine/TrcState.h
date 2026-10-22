// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC's own state between commands (M6b, replacing the string side flags of M6): the weather and
// its running DRM, each side's per-turn counts and lists, the leader-lost and Warsaw-garrison
// state machines, the surrendered minor allies and a sudden-death result. TrcStateCodec is the only
// place these become hexsave <flag name value/> strings: the Axis side carries weather,
// weather-drm, surrendered, helsinki-russian, sudden-death and garrison-warsaw; every side may carry
// the per-side flags. An absent count is written as no flag, and zero is not absence.
// ----------------------------------------------
#pragma once
#include "TrcCalendar.h"
#include "TrcFacts.h"

#include "hexmodel/GameState.h"

#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace Trc {

  enum class LeaderLost : std::uint8_t { Pending, Active, Spent };  // 11.3: pending -> active -> spent
  enum class WarsawGarrison : std::uint8_t { Due, Done };           // 23.2

  struct SuddenDeathMet {
    std::string condition;  // the rules document's victory condition id
    SideId winner;
  };

  // One side's state for the current player turn (the end phase clears most of it).
  struct TrcSideState {
    std::optional<int> railMoves;           // rail-moves: units moved by rail this turn (9.2)
    std::set<HexCoord::HexId> railTouched;  // rail-touched: rail hexes entered, for the end phase (9.4)
    std::optional<int> airUsed;             // air-used: air units committed this impulse (15.2)
    std::optional<LeaderLost> leaderLost;   // leader-lost (11.3)
    std::optional<int> southEntry;          // south-entry: units entered from the south edge (20.5)
    std::set<SeaArea> seaUsed;              // sea-used: sea areas moved in this turn (10.0)
    std::set<std::string> replaced;         // replaced: Axis replacement categories used (21.0)
    std::optional<int> replacementPoints;   // replacement-points: Russian points left (22.0)
    std::optional<int> replacedArmour;      // replaced-armour
    std::optional<int> replacedGuards;      // replaced-guards
  };

  class TrcState : public HexModel::Cloneable<TrcState, HexModel::GameState> {
  public:
    explicit TrcState(std::size_t sides);

    // Throws std::invalid_argument for a side outside the rules.
    const TrcSideState& side(SideId) const;
    TrcSideState& side(SideId);
    std::size_t sideCount() const { return sides_.size(); }

    void appendDigest(std::string&) const override;

    std::optional<Weather> weather;               // the rolled state of this turn (4.1.1)
    std::optional<int> weatherDrm;                // the running weather DRM
    std::set<Nation> surrendered;                 // minor allies out of the war (24.0)
    bool helsinkiRussianP = false;                // Helsinki is Russian for good (24.0)
    std::optional<SuddenDeathMet> suddenDeath;    // a condition met in a Sudden Death phase
    std::optional<WarsawGarrison> garrisonWarsaw;

  private:
    std::vector<TrcSideState> sides_;
  };

  // The position's TRC state; throws std::invalid_argument when it holds none.
  const TrcState& stateOf(const Position&);
  TrcState& stateOf(Position&);

  // An absent count counts as zero once something is added.
  void addCount(std::optional<int>& count, int by);

  std::string_view seaAreaName(SeaArea);  // "baltic", "black-sea", "caspian"
  Nation nationNamed(std::string_view);   // the inverse of nationName; throws naming the text

  class TrcStateCodec : public HexModel::GameStateCodec {
  public:
    explicit TrcStateCodec(const HexRules::RuleSet&);
    HexModel::Polymorphic<HexModel::GameState> decode(const HexModel::SideFlags&) const override;
    HexModel::SideFlags encode(const HexModel::GameState&) const override;

  private:
    void decodeAxisFlag(TrcState&, const HexModel::SideFlag&) const;
    void decodeSideFlag(TrcSideState&, const HexModel::SideFlag&) const;

    const HexRules::RuleSet& rules_;
    SideId axis_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
