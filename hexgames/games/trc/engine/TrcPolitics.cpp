// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "TrcPolitics.h"

#include "TrcFlags.h"
#include "TrcUnits.h"

#include <array>

namespace Trc {

  namespace {

    constexpr std::array<std::string_view, 8> kClaims{"surrender-hungary", "surrender-finland-rumania", "surrender-italy",
                                                      "surrender-finland-1944", "surrender-effect", "partisan-cycle",
                                                      "order-partisan-removal", "garrison-warsaw"};

  }  // namespace

  TrcPolitics::TrcPolitics(const TrcFacts& facts, const TrcZoc& zoc) : facts_(facts), zoc_(zoc)
  {
  }

  std::span<const std::string_view>
  TrcPolitics::claims()
  {
    return kClaims;
  }

  Position
  TrcPolitics::surrender(const Ctx& ctx, Nation nation, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    const std::string name(nationName(nation));
    if (Flags::listedP(ctx.position, facts_.axis(), Flags::kSurrendered, name)) {
      return next;
    }
    Flags::add(next, facts_.axis(), Flags::kSurrendered, name);
    sink.onEvent(HexEngine::GameEvent{"surrender", name});
    for (UnitId unit : ctx.roster.ofSide(facts_.axis())) {
      if (nation == facts_.nation(unit) && !Units::inSpaceP(next, unit, facts_.surrendered(facts_.axis()))) {
        Units::remove(next, unit, facts_.surrendered(facts_.axis()), sink);
      }
    }
    return next;
  }

  Position
  TrcPolitics::russianEndPhase(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    int inHungary = 0;
    for (UnitId unit : Units::onMap(ctx, facts_.russian())) {
      inHungary += facts_.inCountryP(*Units::hexOf(ctx.position, unit), "hungary") ? 1 : 0;
    }
    const auto step = [&](bool triggeredP, Nation nation) {
      if (triggeredP) {
        next = surrender(Ctx{ctx.board, ctx.rules, ctx.roster, next}, nation, sink);
      }
      return;
    };
    step(5 <= inHungary, Nation::Hungarian);
    step(facts_.russian() == ctx.position.control(facts_.named("Helsinki")), Nation::Finnish);
    step(facts_.russian() == ctx.position.control(facts_.named("Bucharest")), Nation::Rumanian);
    return next;
  }

  Position
  TrcPolitics::italy(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    if (15 != ctx.position.clock().turn) {
      return ctx.position;
    }
    return surrender(ctx, Nation::Italian, sink);
  }

  Position
  TrcPolitics::finland1944(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    if (21 > ctx.position.clock().turn || facts_.axis() == ctx.position.control(facts_.named("Leningrad"))) {
      return ctx.position;
    }
    Position next = surrender(ctx, Nation::Finnish, sink);
    if (!next.flag(facts_.axis(), Flags::kHelsinkiRussian)) {
      next.setFlag(facts_.axis(), Flags::kHelsinkiRussian, "true");
      const HexIndex helsinki = facts_.named("Helsinki");
      next.setControl(helsinki, facts_.russian());
      sink.onEvent(HexEngine::ControlChanged{helsinki, facts_.russian()});
    }
    const Ctx now{ctx.board, ctx.rules, ctx.roster, next};
    for (UnitId unit : Units::onMap(now, facts_.axis())) {
      if (facts_.inCountryP(*Units::hexOf(next, unit), "finland")) {
        Units::remove(next, unit, facts_.pool(facts_.axis()), sink);
      }
    }
    return next;
  }

  Position
  TrcPolitics::removeExposedPartisans(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    for (UnitId partisan : Units::onMap(ctx, facts_.russian())) {
      if (!facts_.typeP(partisan, "partisan")) {
        continue;
      }
      const HexIndex hex = *Units::hexOf(ctx.position, partisan);
      bool exposedP = zoc_.enemyZocP(ctx, hex, facts_.russian(), false);
      for (UnitId axisUnit : Units::onMap(ctx, facts_.axis())) {
        exposedP = exposedP || ("ss" == ctx.roster.unit(axisUnit).nationality &&
                                Units::withinP(ctx, *Units::hexOf(ctx.position, axisUnit), hex, 5));
      }
      if (exposedP) {
        Units::remove(next, partisan, std::nullopt, sink);  // never permanently eliminated: placed again
      }
    }
    return next;
  }

  Position
  TrcPolitics::liftPartisans(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    for (UnitId partisan : Units::onMap(ctx, facts_.russian())) {
      if (facts_.typeP(partisan, "partisan")) {
        Units::remove(next, partisan, std::nullopt, sink);
      }
    }
    return next;
  }

  Position
  TrcPolitics::noteWarsaw(const Ctx& ctx, const HexEngine::MoveUnit& move) const
  {
    Position next = ctx.position;
    if (17 > ctx.position.clock().turn || facts_.russian() != ctx.roster.unit(move.units.front()).side ||
        next.flag(facts_.axis(), Flags::kGarrisonWarsaw)) {
      return next;
    }
    if (Units::withinP(ctx, move.path.back(), facts_.named("Warsaw"), 2)) {
      next.setFlag(facts_.axis(), Flags::kGarrisonWarsaw, "due");
    }
    return next;
  }

  Position
  TrcPolitics::garrisonWarsaw(const Ctx& ctx, HexEngine::EventSink& sink) const
  {
    Position next = ctx.position;
    const HexIndex warsaw = facts_.named("Warsaw");
    if ("due" != ctx.position.flag(facts_.axis(), Flags::kGarrisonWarsaw).value_or("")) {
      return next;
    }
    next.setFlag(facts_.axis(), Flags::kGarrisonWarsaw, "done");
    if (facts_.axis() != ctx.position.control(warsaw)) {
      sink.onEvent(HexEngine::GameEvent{"garrison", "Warsaw is not Axis; the summons lapses"});
      return next;
    }
    for (const char* id : {"g-ss-4-armour", "g-lu-hg-mechanized"}) {
      const UnitId unit = facts_.counter(id);
      const std::optional<Location>& where = ctx.position.unit(unit).where;
      if (!where || Units::inSpaceP(ctx.position, unit, facts_.omb())) {
        Units::place(next, unit, warsaw, sink);
      }
    }
    return next;
  }

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
