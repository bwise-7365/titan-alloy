// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexengine/AdjudicatorsDetail.h"

#include <algorithm>

namespace HexEngine::Adjudicators::Detail {

  std::optional<HexIndex>
  hexOf(const Position& position, UnitId unit)
  {
    const HexModel::UnitState& state = position.unit(unit);
    if (!state.where || !std::holds_alternative<HexIndex>(*state.where)) {
      return std::nullopt;
    }
    return std::get<HexIndex>(*state.where);
  }

  std::optional<SpaceId>
  boxFor(const Ctx& ctx, SideId side, bool returnsP)
  {
    for (std::size_t i = 0; i < ctx.rules.spaces().size(); ++i) {
      const HexRules::SpaceSpec& spec = ctx.rules.spaces()[i];
      if (spec.sides.test(side.value) && spec.returnsP == returnsP) {
        return SpaceId{static_cast<std::uint32_t>(i)};
      }
    }
    return std::nullopt;
  }

  void
  removeToBox(Position& position, UnitId unit, std::optional<SpaceId> box, EventSink& sink)
  {
    if (box) {
      position.place(unit, *box);
    } else {
      position.remove(unit);
    }
    sink.onEvent(UnitEliminated{unit, box});
    return;
  }

  void
  loseStep(const Ctx&, Position& position, UnitId unit, std::optional<SpaceId> box, EventSink& sink)
  {
    HexModel::UnitState& state = position.state(unit);
    const std::optional<HexModel::Steps> reduced = state.steps.reduced();
    if (!reduced) {
      removeToBox(position, unit, box, sink);
      return;
    }
    state.steps = *reduced;
    state.face = HexModel::Face::Back;
    sink.onEvent(UnitReduced{unit, state.steps.current()});
    return;
  }

  std::vector<UnitId>
  lossOrder(const Ctx& ctx, const Position& position, const std::vector<UnitId>& involved, SideId side)
  {
    std::vector<UnitId> mine;
    for (UnitId unit : involved) {
      if (side == ctx.roster.unit(unit).side && position.unit(unit).where.has_value()) {
        mine.push_back(unit);
      }
    }
    std::sort(mine.begin(), mine.end(), [&](UnitId l, UnitId r) {
      const int left = position.unit(l).steps.current();
      const int right = position.unit(r).steps.current();
      if (left != right) {
        return left < right;
      }
      return ctx.roster.unit(l).counter.text < ctx.roster.unit(r).counter.text;
    });
    return mine;
  }

}  // namespace HexEngine::Adjudicators::Detail
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
