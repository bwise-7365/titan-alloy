// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Victory (15.0). The German Player scores each Victory Point hex he controls -- a German unit in it,
// or the last to pass through it (the hex's control) -- with a line to the west edge (15.11), Smolensk
// less two a turn from turn 5 onward counted from the turn he took it (amendments 15.11), plus one
// point for each of the first five South-Western Front divisions and two for each of the next five
// (14.23). The Soviet Player scores one per whole German division eliminated, none for the cavalry or
// an independent regiment (15.12), and two each time he occupies at the end of his player-turn a city
// the German Player held with a line west at the end of the last German player-turn. After turn 12 the
// Soviet total is taken from the German and the level read off the rules' victory-levels list (15.2).
// ----------------------------------------------
#pragma once
#include "PggFacts.h"
#include "PggSupply.h"

#include "hexengine/Command.h"

namespace Pgg {

  class PggVictory : public HexEngine::VictoryCheck {
  public:
    PggVictory(const PggFacts&, const PggSupply&);
    std::span<const std::string_view> claims() const override;
    std::optional<HexEngine::Outcome> check(const Ctx&) const override;

    int germanPoints(const Ctx&, HexSearch::SearchScratch&) const;
    int sovietPoints(const Ctx&) const;
    int hexPoints(const Ctx&, const VictoryHex&) const;
    bool divisionEliminatedP(const Ctx&, Division) const;

    Position declare(const Ctx&, HexSearch::SearchScratch&, HexEngine::EventSink&) const;
    Position recordGermanCities(const Ctx&, HexSearch::SearchScratch&) const;
    Position scoreRecaptures(const Ctx&, HexEngine::EventSink&) const;
    Position noteControl(const Ctx&, const HexEngine::Command&, HexEngine::EventSink&) const;

  private:
    const PggFacts& facts_;
    const PggSupply& supply_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
