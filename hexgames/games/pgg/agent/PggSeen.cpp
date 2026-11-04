// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "PggSeen.h"

#include "PggUnits.h"

namespace Pgg {

  PggSeen::PggSeen(const PggPolicySet& set, const Ctx& ctx)
    : set_(set), ctx_(ctx), rifle_(meansOf(Arm::SovietRifle)), armour_(meansOf(Arm::SovietArmour))
  {
  }

  PggSeen::Means
  PggSeen::meansOf(Arm arm) const
  {
    Means means;
    int count = 0;
    for (UnitId unit : ctx_.roster.ofSide(set_.facts().soviet())) {
      const HexModel::UnitState& state = ctx_.position.unit(unit);
      if (arm == set_.facts().arm(unit) && state.where && !state.flags.revealedP) {
        means.attack += Units::attackOf(ctx_, unit);
        means.defence += Units::defenceOf(ctx_, unit);
        count += 1;
      }
    }
    if (0 < count) {
      means.attack /= count;
      means.defence /= count;
    }
    return means;
  }

  double
  PggSeen::doubled(UnitId unit) const
  {
    return set_.combat().integratedP(ctx_, unit) ? 2.0 : 1.0;
  }

  double
  PggSeen::attack(UnitId unit) const
  {
    if (!ctx_.position.unit(unit).flags.revealedP) {
      return Arm::SovietArmour == set_.facts().arm(unit) ? armour_.attack : rifle_.attack;
    }
    return Units::attackOf(ctx_, unit) * doubled(unit);
  }

  double
  PggSeen::defence(UnitId unit) const
  {
    if (!ctx_.position.unit(unit).flags.revealedP) {
      return Arm::SovietArmour == set_.facts().arm(unit) ? armour_.defence : rifle_.defence;
    }
    return Units::defenceOf(ctx_, unit) * doubled(unit);
  }

  std::vector<HexAi::Piece>
  PggSeen::pieces(SideId side) const
  {
    std::vector<HexAi::Piece> out;
    for (UnitId unit : Units::onMap(ctx_, set_.facts(), side)) {
      out.push_back(HexAi::Piece{*Units::hexOf(ctx_.position, unit), attack(unit)});
    }
    return out;
  }

}  // namespace Pgg
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
