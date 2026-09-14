// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Compiled into the hexrules static library (see BoardBuilder.cpp for why).
// ----------------------------------------------
#include "hexmodel/PositionBuilder.h"

#include "hexrules/RuleSet.h"

#include <stdexcept>

namespace HexModel {

  namespace {

    int
    parseRegisterValue(const std::string& track, const std::string& text)
    {
      try {
        std::size_t consumed = 0;
        const int value = std::stoi(text, &consumed);
        if (consumed != text.size()) {
          throw std::invalid_argument("");
        }
        return value;
      } catch (const std::exception&) {
        throw std::invalid_argument("PositionBuilder: register '" + track + "' has a non-numeric value '" +
                                     text + "'");
      }
    }

    template <class T>
    const T&
    need(const std::optional<T>& value, std::size_t entry, const char* attribute)
    {
      if (!value) {
        throw std::invalid_argument("PositionBuilder: resolution entry " + std::to_string(entry + 1) + " needs @" +
                                    attribute);
      }
      return *value;
    }

    UnitId
    unitNamed(const Roster& roster, const std::string& id)
    {
      const std::optional<UnitId> found = roster.find(CounterId{id});
      if (!found) {
        throw std::invalid_argument("PositionBuilder: resolution names unknown counter '" + id + "'");
      }
      return *found;
    }

    std::vector<UnitId>
    unitsNamed(const Roster& roster, const std::vector<std::string>& ids)
    {
      std::vector<UnitId> out;
      for (const std::string& id : ids) {
        out.push_back(unitNamed(roster, id));
      }
      return out;
    }

    std::vector<HexIndex>
    hexesNamed(const Board& board, const std::vector<std::string>& ids)
    {
      std::vector<HexIndex> out;
      for (const std::string& id : ids) {
        out.push_back(board.indexOf(HexCoord::HexId{id}));
      }
      return out;
    }

    RandomizerId
    randomizerNamed(const HexRules::RuleSet& rules, const std::string& id)
    {
      for (std::size_t i = 0; i < rules.randomizers().size(); ++i) {
        if (id == rules.randomizers()[i].id) {
          return RandomizerId{static_cast<std::uint32_t>(i)};
        }
      }
      throw std::invalid_argument("PositionBuilder: resolution names unknown randomizer '" + id + "'");
    }

    Battle
    battleOf(const HexXml::SaveOweDoc& owe, const Roster& roster, const HexRules::RuleSet& rules)
    {
      Battle battle;
      battle.router = owe.router ? std::optional<SideId>(rules.side(*owe.router)) : std::nullopt;
      battle.involved = unitsNamed(roster, owe.involved);
      return battle;
    }

    Obligation
    obligationOf(const HexXml::SaveOweDoc& owe, std::size_t entry, const Board& board, const Roster& roster,
                 const HexRules::RuleSet& rules, const ObligationCodec& codec)
    {
      if ("loss" == owe.kind) {
        return OwedLoss{battleOf(owe, roster, rules), rules.side(need(owe.side, entry, "side")),
                        need(owe.count, entry, "count")};
      }
      if ("retreat" == owe.kind) {
        return OwedRetreat{battleOf(owe, roster, rules), rules.side(need(owe.side, entry, "side")),
                           need(owe.fewest, entry, "fewest"), need(owe.most, entry, "most")};
      }
      if ("unit-retreat" == owe.kind) {
        return UnitRetreat{battleOf(owe, roster, rules), unitNamed(roster, need(owe.unit, entry, "unit")),
                           board.indexOf(HexCoord::HexId{need(owe.from, entry, "from")}),
                           need(owe.fewest, entry, "fewest"), need(owe.most, entry, "most"),
                           hexesNamed(board, owe.path)};
      }
      if ("game" == owe.kind) {
        std::vector<ObligationArg> args;
        for (const HexXml::SaveArgDoc& arg : owe.args) {
          args.push_back(ObligationArg{arg.name, arg.value});
        }
        return codec.decode(need(owe.name, entry, "name"), args);
      }
      throw std::invalid_argument("PositionBuilder: resolution entry " + std::to_string(entry + 1) + " has kind '" +
                                  owe.kind + "'");
    }

    PendingDecision
    decisionOf(const HexXml::SaveAskDoc& ask, std::size_t entry, const Board& board, const Roster& roster,
               const HexRules::RuleSet& rules)
    {
      if ("loss" == ask.what) {
        return ChooseLoss{rules.side(need(ask.side, entry, "side")), unitsNamed(roster, ask.candidates),
                          need(ask.count, entry, "count")};
      }
      if ("retreat" == ask.what) {
        return ChooseRetreat{rules.side(need(ask.side, entry, "side")), unitNamed(roster, need(ask.unit, entry, "unit")),
                             hexesNamed(board, ask.candidates), need(ask.mayStop, entry, "may-stop")};
      }
      if ("card" == ask.what) {
        return ChooseCard{randomizerNamed(rules, need(ask.deck, entry, "deck")), ask.candidates};
      }
      if ("choice" == ask.what) {
        return GameChoice{need(ask.verb, entry, "verb"), ask.options};
      }
      throw std::invalid_argument("PositionBuilder: resolution entry " + std::to_string(entry + 1) + " asks '" +
                                  ask.what + "'");
    }

  }  // namespace

  Position
  PositionBuilder::build(const HexXml::SaveDoc& save, const Board& board, const Roster& roster,
                          const HexRules::RuleSet& rules, const GameStateCodec& codec,
                          const ObligationCodec& obligations)
  {
    Position pos;

    // ---- units, pre-populated fresh (off-map, full strength) then overridden by the save --------
    for (const UnitSpec& spec : roster.units()) {
      pos.units_.push_back(UnitState{.where = std::nullopt,
                                      .steps = Steps(spec.maxSteps, spec.maxSteps),
                                      .face = Face::Front,
                                      .flags = UnitFlags{},
                                      .delay = std::nullopt,
                                      .markers = {}});
    }
    pos.byHex_.assign(board.hexCount(), {});
    pos.bySpace_.assign(board.spaceCount(), {});
    pos.control_.assign(board.hexCount(), std::nullopt);
    pos.linkOwners_.resize(board.networkCount());
    for (std::size_t i = 0; i < board.networkCount(); ++i) {
      pos.linkOwners_[i].assign(board.network(NetworkId{static_cast<std::uint32_t>(i)}).linkCount(),
                                 std::nullopt);
    }
    pos.regions_.resize(board.layerCount());
    for (std::size_t i = 0; i < board.layerCount(); ++i) {
      pos.regions_[i].assign(board.layer(LayerId{static_cast<std::uint32_t>(i)}).regionCount(), RegionState{});
    }
    pos.tracks_.assign(board.trackCount(), 0);

    // ---- cursor / clock -------------------------------------------------------------------------
    pos.clock_.turn = save.cursor.turn;
    pos.clock_.phase = rules.phase(save.cursor.phase);
    pos.clock_.actingSide = save.cursor.side ? std::optional<SideId>(rules.side(*save.cursor.side)) : std::nullopt;

    // ---- side registers (tracks) and flags (the game's state) ------------------------------------
    SideFlags flags(rules.sides().size());
    for (const HexXml::SaveSideDoc& side : save.sides) {
      for (const HexXml::SaveRegisterDoc& reg : side.registers) {
        const TrackId t = board.trackId(reg.track);
        pos.tracks_[t.value] = parseRegisterValue(reg.track, reg.value);
      }
      const SideId owner = rules.side(side.id);
      for (const HexXml::SaveFlagDoc& flag : side.flags) {
        flags[owner.value].push_back(SideFlag{flag.name, flag.value});
      }
    }
    pos.setGameState(codec.decode(flags));

    // ---- units ------------------------------------------------------------------------------------
    for (const HexXml::SaveUnitDoc& u : save.units) {
      const std::optional<UnitId> uid = roster.find(CounterId{u.counter});
      if (!uid) {
        throw std::invalid_argument("PositionBuilder: unit '" + u.id + "' names unknown counter '" +
                                     u.counter + "'");
      }
      if (u.hex.has_value() == u.space.has_value()) {
        throw std::invalid_argument("PositionBuilder: unit '" + u.id +
                                     "' must give exactly one of hex or space");
      }
      Location loc;
      if (u.hex) {
        loc = board.indexOf(HexCoord::HexId{*u.hex});
      } else {
        loc = board.spaceId(*u.space);
      }
      pos.place(*uid, loc);

      UnitState& state = pos.state(*uid);
      if (u.steps) {
        state.steps = Steps(*u.steps, roster.unit(*uid).maxSteps);
      }
      state.face = "back" == u.face ? Face::Back : Face::Front;
      state.flags.movedP = u.moved;
      state.flags.revealedP = u.revealed.value_or(true);
      state.delay = u.delay;
      state.markers = u.status;
    }

    // ---- control ----------------------------------------------------------------------------------
    for (const HexXml::SaveControlHexDoc& c : save.controlHexes) {
      const HexIndex hi = board.indexOf(HexCoord::HexId{c.id});
      pos.setControl(hi, rules.side(c.side));
    }
    for (const HexXml::SaveControlLinkDoc& c : save.controlLinks) {
      if (2 != c.hexes.size()) {
        throw std::invalid_argument("PositionBuilder: control link on network '" + c.network +
                                     "' must name exactly two hexes");
      }
      const NetworkId netId = board.networkId(c.network);
      const HexIndex a = board.indexOf(HexCoord::HexId{c.hexes[0]});
      const HexIndex b = board.indexOf(HexCoord::HexId{c.hexes[1]});
      const LinkNetwork& net = board.network(netId);
      std::optional<std::size_t> linkIndex;
      for (std::size_t i = 0; i < net.links().size(); ++i) {
        const LinkNetwork::Link& link = net.links()[i];
        if ((link.a == a && link.b == b) || (link.a == b && link.b == a)) {
          linkIndex = i;
          break;
        }
      }
      if (!linkIndex) {
        throw std::invalid_argument("PositionBuilder: network '" + c.network + "' has no link between '" +
                                     c.hexes[0] + "' and '" + c.hexes[1] + "'");
      }
      pos.setLinkOwner(netId, *linkIndex, rules.side(c.side));
    }

    // ---- region state -------------------------------------------------------------------------------
    for (const HexXml::SaveRegionDoc& r : save.regions) {
      const LayerId layerId = board.layerId(r.layer);
      const RegionId regionId = board.regionId(layerId, r.id);
      RegionState& state = pos.region(layerId, regionId);
      state.status = r.status;
      state.alignment = r.alignment ? std::optional<SideId>(rules.side(*r.alignment)) : std::nullopt;
      state.posture = r.posture;
      state.owner = r.owner ? std::optional<SideId>(rules.side(*r.owner)) : std::nullopt;
    }

    // ---- the resolution stack, bottom first ------------------------------------------------------------
    for (std::size_t entry = 0; entry < save.resolution.size(); ++entry) {
      const HexXml::SaveOweDoc& owe = save.resolution[entry];
      pos.push(obligationOf(owe, entry, board, roster, rules, obligations));
      if (owe.ask) {
        pos.ask(decisionOf(*owe.ask, entry, board, roster, rules));
      }
    }

    return pos;
  }

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
