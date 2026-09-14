// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The TRC Combat Results Table (13.0-14.2). Factors: each unit's printed factor, halved and
// rounded up without combat supply (17.3.1), then the defenders doubled -- never more than once --
// on a mountain, in a major city, behind a river (14.1.1) or across the Kerch Strait (8.5). The
// odds are rounded in the defender's favour and shifted along the ratio ladder by air power,
// capped at three; the die row and odds column give the result, and woods turn AR and DR into C.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"
#include "TrcSupply.h"

namespace Trc {

  class TrcCombat : public HexEngine::CombatResolver {
  public:
    TrcCombat(const TrcFacts&, const TrcSupply&);
    std::span<const std::string_view> claims() const override;
    std::vector<HexEngine::CombatEffect> resolve(const Ctx&, const HexEngine::CombatContext&,
                                                 HexEngine::PrngStreams&) const override;
    HexEngine::CombatReport report(const Ctx&, const HexEngine::CombatContext&, HexEngine::PrngStreams&) const override;

    struct Factors {
      Strength attack;
      Strength defence;
    };
    Factors factors(const Ctx&, const std::vector<UnitId>& attackers, HexIndex target,
                    const std::vector<UnitId>& defenders) const;
    // The odds before any shift; BelowMinimum under 1-6, AboveMaximum over 9-1.
    HexModel::OddsOutcome odds(const Factors&) const;
    int shift(const std::vector<ModifierId>&) const;
    bool doubledP(const Ctx&, const std::vector<UnitId>& attackers, HexIndex target) const;

    // The ratio ladder the table's columns cover, 1-6 up to 9-1, and the column of each rung.
    static const std::vector<HexModel::Odds>& ladder();
    static std::size_t columnOf(HexModel::Odds);
    static std::string text(HexModel::Odds);

  private:
    Strength factorOf(const Ctx&, UnitId) const;
    const TrcFacts& facts_;
    const TrcSupply& supply_;
    const HexRules::Table& table_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
