// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TODO(decide): PROVISIONAL. The Movement Allowance Chart is on a player-aid card that no input
// carries (game_rules/the-russian-campaign.md 2.4 records its structure, not its numbers). What the
// rules document does say is honoured: clear-weather first-impulse movement is the printed factor;
// armour moves in the second impulse; Russian infantry has none. The other cells are the engine's
// placeholder and are listed as an open question in tasks/05-trc-engine.md.
// ----------------------------------------------
#include "TrcMovement.h"

#include <stdexcept>

namespace Trc {

  namespace {

    int
    half(int printed)
    {
      return (printed + 1) / 2;
    }

  }  // namespace

  int
  chartAllowance(const TrcFacts& facts, UnitId unit, Weather weather, Impulse impulse, int printed)
  {
    const bool mobileP = facts.mobileP(unit);
    const bool russianP = Nation::Russian == facts.nation(unit);
    switch (impulse) {
      case Impulse::First:
        switch (weather) {
          case Weather::Clear:
            return printed;
          case Weather::LightMud:
            return half(printed);
          case Weather::Mud:
            return std::min(printed, 2);
          case Weather::Snow:
            return russianP ? printed : half(printed);
        }
        break;
      case Impulse::Second:
        switch (weather) {
          case Weather::Clear:
            if (mobileP) {
              return printed;
            }
            return russianP ? 0 : printed / 2;
          case Weather::LightMud:
            return mobileP ? half(printed) : 0;
          case Weather::Mud:
            return 0;
          case Weather::Snow:
            return mobileP && russianP ? half(printed) : 0;
        }
        break;
    }
    throw std::invalid_argument("Trc::chartAllowance: impulse or weather outside the chart");
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
