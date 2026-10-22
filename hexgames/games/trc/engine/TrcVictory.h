// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC victory (25.0): the Russian wins on controlling Berlin; the Axis on controlling Moscow with
// Stalin eliminated, or when the May/June 1945 turn ends without Russian Berlin. A sudden death
// check, run at the Sudden Death phase of turns 5, 11, 17 and 23, records "<condition> <side>" in
// the Axis flag "sudden-death" and the check reads it back.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"

namespace Trc {

  class TrcVictory : public HexEngine::VictoryCheck {
  public:
    explicit TrcVictory(const TrcFacts&);
    std::span<const std::string_view> claims() const override;
    std::optional<HexEngine::Outcome> check(const Ctx&) const override;

    // The Sudden Death phase: the condition one side meets this turn, if any.
    std::optional<HexEngine::Outcome> suddenDeath(const Ctx&) const;
    Position recordSuddenDeath(const Ctx&, HexEngine::EventSink&) const;

  private:
    bool holdsAllP(const Ctx&, SideId, const std::vector<std::string_view>& names) const;
    bool holdsAnyP(const Ctx&, SideId, const std::vector<std::string_view>& names) const;
    bool surrenderedP(const Ctx&, std::string_view nation) const;
    const TrcFacts& facts_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
