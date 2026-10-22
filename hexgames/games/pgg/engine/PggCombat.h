// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Combat strength at the instant of combat (9.0, 12.2, 11.0). Order of operations (amendments 9.35,
// 11.37; 6.55): Divisional Integration doubles a whole Panzer or Motorized division stacked alone
// (7.3), then the defender's terrain multiple applies (9.31-9.33: Major City, Forest and River when
// every attacker crosses one each double, cumulative by adding the multiples less one), then each
// unit out of supply is halved (fractions dropped, never below one), then a Soviet Leader stacked with
// attackers adds up to his rating but no more than their strength (10.36), then an overrun halves
// the attack (6.55). A unit that will reveal as having no strength adds nothing, and nor does a unit
// retreated onto the defending stack (9.75). The result is resolved on the CRT and owed as a
// PggBattle (the engine's OweEffect), which the resolution stack works down.
// ----------------------------------------------
#pragma once
#include "PggCrt.h"
#include "PggFacts.h"
#include "PggSupply.h"

#include <vector>

namespace Pgg {

  class PggCombat : public HexEngine::CombatResolver {
  public:
    PggCombat(const PggFacts&, const PggSupply&);
    std::span<const std::string_view> claims() const override;
    std::vector<HexEngine::CombatEffect> resolve(const Ctx&, const HexEngine::CombatContext&,
                                                 HexEngine::PrngStreams&) const override;
    HexEngine::CombatReport report(const Ctx&, const HexEngine::CombatContext&, HexEngine::PrngStreams&) const override;

    struct Assessment {
      std::vector<UnitId> attackers;  // the listed attackers and the Leaders adding their points
      std::vector<UnitId> defenders;  // every unit in the target hex
      Strength attack;
      Strength defence;
    };
    Assessment assess(const Ctx&, const std::vector<UnitId>& listed, HexIndex target, bool overrunP) const;
    // Rolls (Combat stream) unless no strength attacks or defends, and owes the battle.
    HexEngine::CombatReport resolveBattle(const Ctx&, const Assessment&, HexIndex target, bool overrunP,
                                          HexEngine::PrngStreams&) const;
    int defenceMultiple(const Ctx&, const std::vector<UnitId>& attackers, HexIndex target) const;
    bool integratedP(const Ctx&, UnitId) const;

  private:
    int adjusted(const Ctx&, HexSearch::SearchScratch&, UnitId, int strength) const;
    const PggFacts& facts_;
    const PggSupply& supply_;
    const HexRules::Table& table_;
  };

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
