// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexrecord/RecordResolution.h"

#include <type_traits>

namespace HexRecord::Detail {

  namespace {

    void
    battleInto(SaveOwe& owe, const HexModel::Battle& battle, const HexEngine::GameNames& names)
    {
      if (battle.router) {
        owe.router = names.side(*battle.router);
      }
      for (HexModel::UnitId unit : battle.involved) {
        owe.involved.push_back(names.counter(unit));
      }
      return;
    }

    std::optional<SaveAsk>
    askOf(const HexModel::PendingDecision& decision, const HexEngine::GameNames& names)
    {
      std::optional<SaveAsk> out;
      std::visit(
          [&](auto&& d) {
            using T = std::decay_t<decltype(d)>;
            if constexpr (std::is_same_v<T, HexModel::ChooseLoss>) {
              SaveAsk ask{.what = "loss", .side = names.side(d.side), .count = d.count};
              for (HexModel::UnitId unit : d.candidates) {
                ask.candidates.push_back(names.counter(unit));
              }
              out = ask;
            } else if constexpr (std::is_same_v<T, HexModel::ChooseRetreat>) {
              SaveAsk ask{.what = "retreat", .side = names.side(d.side), .unit = names.counter(d.unit),
                          .mayStopP = d.mayStopP};
              for (HexModel::HexIndex hex : d.candidates) {
                ask.candidates.push_back(names.hex(hex));
              }
              out = ask;
            } else if constexpr (std::is_same_v<T, HexModel::ChooseCard>) {
              out = SaveAsk{.what = "card", .candidates = d.candidates, .deck = names.randomizer(d.deck)};
            } else if constexpr (std::is_same_v<T, HexModel::GameChoice>) {
              SaveAsk ask{.what = "choice", .verb = d.verb, .options = d.options};
              if (d.side) {
                ask.side = names.side(*d.side);
              }
              out = ask;
            }
          },
          decision);
      return out;
    }

    SaveOwe
    oweOf(const HexModel::Resolution& entry, const HexEngine::GameNames& names, const HexModel::ObligationCodec& codec)
    {
      SaveOwe out;
      std::visit(
          [&](auto&& o) {
            using T = std::decay_t<decltype(o)>;
            if constexpr (std::is_same_v<T, HexModel::OwedLoss>) {
              out.kind = "loss";
              out.side = names.side(o.side);
              out.count = o.count;
              battleInto(out, o.battle, names);
            } else if constexpr (std::is_same_v<T, HexModel::OwedRetreat>) {
              out.kind = "retreat";
              out.side = names.side(o.side);
              out.fewest = o.fewest;
              out.most = o.most;
              battleInto(out, o.battle, names);
            } else if constexpr (std::is_same_v<T, HexModel::UnitRetreat>) {
              out.kind = "unit-retreat";
              out.unit = names.counter(o.unit);
              out.from = names.hex(o.from);
              for (HexModel::HexIndex hex : o.path) {
                out.path.push_back(names.hex(hex));
              }
              out.fewest = o.fewest;
              out.most = o.most;
              battleInto(out, o.battle, names);
            } else {
              out.kind = "game";
              out.name = std::string(o.base().kind());
              for (const HexModel::ObligationArg& arg : codec.encode(o.base())) {
                out.args.push_back(SaveArg{arg.name, arg.value});
              }
            }
          },
          entry.owed);
      out.ask = askOf(entry.asked, names);
      return out;
    }

  }  // namespace

  std::vector<SaveOwe>
  resolutionOf(const HexModel::Position& position, const HexEngine::GameNames& names,
               const HexModel::ObligationCodec& codec)
  {
    std::vector<SaveOwe> out;
    for (const HexModel::Resolution& entry : position.resolution()) {
      out.push_back(oweOf(entry, names, codec));
    }
    return out;
  }

  HexXml::SaveOweDoc
  asOweDoc(const SaveOwe& owe)
  {
    HexXml::SaveOweDoc out;
    out.kind = owe.kind;
    out.side = owe.side;
    out.count = owe.count;
    out.fewest = owe.fewest;
    out.most = owe.most;
    out.unit = owe.unit;
    out.from = owe.from;
    out.path = owe.path;
    out.router = owe.router;
    out.involved = owe.involved;
    out.name = owe.name;
    for (const SaveArg& arg : owe.args) {
      out.args.push_back(HexXml::SaveArgDoc{arg.name, arg.value});
    }
    if (owe.ask) {
      out.ask = HexXml::SaveAskDoc{owe.ask->what,     owe.ask->side,  owe.ask->unit, owe.ask->candidates, owe.ask->count,
                                   owe.ask->mayStopP, owe.ask->deck,  owe.ask->verb, owe.ask->options};
    }
    return out;
  }

}  // namespace HexRecord::Detail
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
