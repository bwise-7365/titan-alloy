// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcState.h"

#include <array>
#include <stdexcept>

namespace Trc {

  namespace {

    void
    appendCount(std::string& s, const std::optional<int>& count)
    {
      s += count ? std::to_string(*count) : std::string("-");
      s += ';';
      return;
    }

    template <class E>
    void
    appendEnum(std::string& s, const std::optional<E>& value)
    {
      s += value ? std::to_string(static_cast<int>(*value)) : std::string("-");
      s += ';';
      return;
    }

  }  // namespace

  TrcState::TrcState(std::size_t sides) : sides_(sides)
  {
  }

  const TrcSideState&
  TrcState::side(SideId id) const
  {
    if (sides_.size() <= id.value) {
      throw std::invalid_argument("TrcState: side index " + std::to_string(id.value) + " outside the rules");
    }
    return sides_[id.value];
  }

  TrcSideState&
  TrcState::side(SideId id)
  {
    if (sides_.size() <= id.value) {
      throw std::invalid_argument("TrcState: side index " + std::to_string(id.value) + " outside the rules");
    }
    return sides_[id.value];
  }

  void
  TrcState::appendDigest(std::string& s) const
  {
    appendEnum(s, weather);
    appendCount(s, weatherDrm);
    for (Nation nation : surrendered) {
      s += std::to_string(static_cast<int>(nation)) + ",";
    }
    s += helsinkiRussianP ? "h;" : "-;";
    s += suddenDeath ? suddenDeath->condition + " " + std::to_string(suddenDeath->winner.value) + ";" : "-;";
    appendEnum(s, garrisonWarsaw);
    for (const TrcSideState& one : sides_) {
      appendCount(s, one.railMoves);
      for (const HexCoord::HexId& id : one.railTouched) {
        s += id.text + ",";
      }
      appendCount(s, one.airUsed);
      appendEnum(s, one.leaderLost);
      appendCount(s, one.southEntry);
      for (SeaArea area : one.seaUsed) {
        s += std::to_string(static_cast<int>(area)) + ",";
      }
      for (const std::string& category : one.replaced) {
        s += category + ",";
      }
      appendCount(s, one.replacementPoints);
      appendCount(s, one.replacedArmour);
      appendCount(s, one.replacedGuards);
      s += '|';
    }
    return;
  }

  const TrcState&
  stateOf(const Position& position)
  {
    return position.gameState<TrcState>();
  }

  TrcState&
  stateOf(Position& position)
  {
    return position.gameState<TrcState>();
  }

  void
  addCount(std::optional<int>& count, int by)
  {
    count = count.value_or(0) + by;
    return;
  }

  std::string_view
  seaAreaName(SeaArea area)
  {
    switch (area) {
      case SeaArea::Baltic:
        return "baltic";
      case SeaArea::BlackSea:
        return "black-sea";
      case SeaArea::Caspian:
        return "caspian";
    }
    throw std::invalid_argument("Trc::seaAreaName: sea area outside the three");
  }

  Nation
  nationNamed(std::string_view text)
  {
    constexpr std::array<Nation, 6> kAll{Nation::German,   Nation::Finnish, Nation::Hungarian,
                                         Nation::Rumanian, Nation::Italian, Nation::Russian};
    for (Nation nation : kAll) {
      if (nationName(nation) == text) {
        return nation;
      }
    }
    throw std::invalid_argument("Trc::nationNamed: '" + std::string(text) + "' is not a TRC nation");
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
