// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcControl.h"

#include "TrcMarkers.h"
#include "TrcState.h"

#include <array>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 4> kClaims{"zoc-contested", "control-last-toucher", "rail-junction",
                                                      "control-frozen-ex"};

  }  // namespace

  TrcControl::TrcControl(const TrcFacts& facts, const TrcZoc& zoc) : facts_(facts), zoc_(zoc)
  {
  }

  std::span<const std::string_view>
  TrcControl::claims()
  {
    return kClaims;
  }

  std::optional<SideId>
  TrcControl::ownerOf(const Ctx& ctx, HexIndex hex) const
  {
    if (facts_.named("HELSINKI") == hex && stateOf(ctx.position).helsinkiRussianP) {
      return facts_.russian();  // 24.0: permanently Russian once Finland surrenders
    }
    for (UnitId occupant : ctx.position.unitsAt(hex)) {
      if (!facts_.typeP(occupant, "partisan")) {
        return ctx.roster.unit(occupant).side;
      }
    }
    const bool axisZocP = zoc_.enemyZocP(ctx, hex, facts_.russian(), false);
    const bool russianZocP = zoc_.enemyZocP(ctx, hex, facts_.axis(), false);
    if (axisZocP && russianZocP) {
      return std::nullopt;
    }
    if (axisZocP) {
      return facts_.axis();
    }
    if (russianZocP) {
      return facts_.russian();
    }
    return ctx.position.control(hex);
  }

  Position
  TrcControl::update(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    if (!next.resolution().empty()) {
      return next;  // 13.3: frozen until the battle's losses are all taken
    }
    for (std::size_t h = 0; h < ctx.board.hexCount(); ++h) {
      const HexIndex hex{static_cast<std::uint32_t>(h)};
      if (!facts_.controlPointP(hex)) {
        continue;
      }
      const std::optional<SideId> owner = ownerOf(ctx, hex);
      if (owner != next.control(hex)) {
        next.setControl(hex, owner);
        sink.onEvent(HexEngine::ControlChanged{hex, owner});
      }
    }
    return next;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
