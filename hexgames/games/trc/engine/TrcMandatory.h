// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Mandatory attacks (12.1, 12.5, 16.3). At the end of a combat phase, a unit of the acting side
// standing in an enemy zone of control, next to an enemy that has not been attacked, owes an
// attack. If it could still make one -- alone, at 1-6 or better, and not barred by rail movement
// or an automatic victory -- ending the phase is refused; if it cannot, it surrenders. Units that
// took part in a first-impulse automatic victory are exempt in that impulse and face the trap in
// the second (16.3). A partisan's zone forces no attack, and nothing reaches across the Kerch Strait.
// ----------------------------------------------
#pragma once
#include "TrcCombat.h"
#include "TrcFacts.h"
#include "TrcZoc.h"

namespace Trc {

  class TrcMandatory {
  public:
    TrcMandatory(const TrcFacts&, const TrcZoc&, const TrcCombat&);
    static std::span<const std::string_view> claims();

    // Throws naming the first unit that still owes an attack it can make.
    void check(const Ctx&) const;
    // The units that owe an attack and cannot make one: they surrender.
    Position surrenderDebtors(const Ctx&, HexEngine::EventSink&) const;

    bool mayAttackP(const Ctx&, UnitId) const;

  private:
    std::vector<HexIndex> unattackedNeighbours(const Ctx&, UnitId) const;
    bool owesP(const Ctx&, UnitId) const;
    bool canAttackP(const Ctx&, UnitId) const;
    const TrcFacts& facts_;
    const TrcZoc& zoc_;
    const TrcCombat& combat_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
