// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Compiled into the hexrules static library (see hexrules/CMakeLists.txt): this translation unit is
// the one place in hexmodel that needs the complete HexRules::RuleSet type.
// ----------------------------------------------
#include "hexmodel/BoardBuilder.h"

#include "hexrules/RuleSet.h"

#include <algorithm>
#include <map>
#include <stdexcept>

namespace HexModel {

  namespace {

    using HexXml::PackageDoc;
    using HexXml::SheetDoc;

    HexCoord::Orientation
    parseOrientation(const std::string& gridId, const std::string& text)
    {
      if ("flat" == text) {
        return HexCoord::Orientation::Flat;
      }
      if ("pointy" == text) {
        return HexCoord::Orientation::Pointy;
      }
      throw std::invalid_argument("grid '" + gridId + "': orientation '" + text +
                                  "' is neither flat nor pointy");
    }

    HexCoord::Parity
    parseParity(const std::string& gridId, const std::string& text)
    {
      if ("odd" == text) {
        return HexCoord::Parity::Odd;
      }
      if ("even" == text) {
        return HexCoord::Parity::Even;
      }
      throw std::invalid_argument("grid '" + gridId + "': offset '" + text + "' is neither odd nor even");
    }

    HexCoord::GridSpec
    toSpec(const HexXml::SheetGridDoc& g)
    {
      HexCoord::GridSpec spec;
      spec.id = g.id.value_or("");
      spec.orientation = parseOrientation(spec.id, g.orientation);
      spec.offset = parseParity(spec.id, g.offset);
      spec.cols = g.cols;
      spec.rows = g.rows;
      spec.size = g.size;
      spec.ox = g.ox;
      spec.oy = g.oy;
      spec.idFormat = g.idFormat;
      spec.colStart = g.colStart;
      spec.colStep = g.colStep;
      spec.rowStart = g.rowStart;
      spec.rowStep = g.rowStep;
      spec.clip = g.clip.value_or("");
      return spec;
    }

    SpaceKind
    parseSpaceKind(const std::string& kind)
    {
      if ("box" == kind) {
        return SpaceKind::Box;
      }
      if ("pool" == kind) {
        return SpaceKind::Pool;
      }
      if ("track" == kind) {
        return SpaceKind::Track;
      }
      if ("display" == kind) {
        return SpaceKind::Display;
      }
      throw std::invalid_argument("BoardBuilder: unknown space kind '" + kind + "'");
    }

    TerrainId
    resolveHexTerrain(const std::map<std::string, std::string>& bySheetTerrain, const std::string& sheetId,
                       const HexRules::RuleSet& rules)
    {
      const auto it = bySheetTerrain.find(sheetId);
      const std::string rulesId = bySheetTerrain.end() != it ? it->second : sheetId;
      return rules.terrain(rulesId);
    }

  }  // namespace

  // Needs friend access to Board's private edge storage (declared as a BoardBuilder member for
  // that reason; unresolved hex ids are skipped rather than thrown, since the sampled sheet data
  // may reference a hex the grid's own clip has removed).
  void
  BoardBuilder::recordEdge(Board& board, const std::vector<HexCoord::Orientation>& orientationOf,
                            const std::string& token, EdgeTerrainId terrain)
  {
    const std::size_t colon = token.rfind(':');
    if (std::string::npos == colon) {
      throw std::invalid_argument("BoardBuilder: malformed hexside reference '" + token + "'");
    }
    const HexCoord::HexId hexId{token.substr(0, colon)};
    const std::string dirText = token.substr(colon + 1);

    const std::optional<HexIndex> hi = board.find(hexId);
    if (!hi) {
      return;
    }
    const HexCoord::Direction d = HexCoord::fromCompass(dirText, orientationOf[hi->value]);
    board.edges_[hi->value * 6 + static_cast<std::size_t>(d)].push_back(terrain);
    if (const std::optional<HexIndex> nb = board.neighbour(*hi, d)) {
      const HexCoord::Direction opp = HexCoord::opposite(d);
      board.edges_[nb->value * 6 + static_cast<std::size_t>(opp)].push_back(terrain);
    }
    return;
  }

  Board
  BoardBuilder::build(const SheetDoc& sheet, const HexRules::RuleSet& rules, const PackageDoc& package)
  {
    Board board;

    // ---- bindings -------------------------------------------------------------------------------
    std::map<std::string, std::string> bySheetTerrain;  // sheet terrain id -> rules terrain id
    std::map<std::string, std::string> bySymbol;        // glyph symbol -> rules terrain id
    for (const HexXml::PackageTerrainBindingDoc& b : package.terrain) {
      if (b.sheet) {
        bySheetTerrain[*b.sheet] = b.rules;
      }
      if (b.symbol) {
        bySymbol[*b.symbol] = b.rules;
      }
    }
    std::map<std::string, std::string> hexsideByLine;
    std::map<std::string, std::string> hexsideByKind;
    for (const HexXml::PackageHexsideBindingDoc& b : package.hexside) {
      if (b.line) {
        hexsideByLine[*b.line] = b.rules;
      }
      if (b.kind) {
        hexsideByKind[*b.kind] = b.rules;
      }
    }
    std::map<std::string, std::string> networkKindToRules;
    for (const HexXml::PackageNetworkBindingDoc& b : package.network) {
      networkKindToRules[b.kind] = b.rules;
    }
    std::map<std::string, std::string> layerSheetToRules;
    for (const HexXml::PackageLayerBindingDoc& b : package.layer) {
      layerSheetToRules[b.sheet] = b.rules;
    }

    // Every sheet terrain must resolve, even one the grid never actually uses.
    for (const HexXml::SheetTerrainDoc& t : sheet.terrains) {
      (void)resolveHexTerrain(bySheetTerrain, t.id, rules);
    }

    // ---- grids and hex interning ------------------------------------------------------------------
    std::vector<HexCoord::Grid> grids;
    for (const HexXml::SheetGridDoc& g : sheet.grids) {
      const HexCoord::GridSpec spec = toSpec(g);
      if (grids.empty()) {
        grids.emplace_back(spec);
      } else {
        grids.emplace_back(spec, grids.front());
      }
    }

    std::vector<HexCoord::Orientation> orientationOf;
    for (std::size_t gi2 = 0; gi2 < grids.size(); ++gi2) {
      const HexCoord::Grid& g = grids[gi2];
      const TerrainId gridDefault = resolveHexTerrain(bySheetTerrain, sheet.grids[gi2].terrain, rules);
      for (const HexCoord::HexId& id : g.ids()) {
        const HexCoord::GridIndex gi = *g.find(id);
        const HexCoord::HexCentre centre = g.centreOf(gi);
        const HexIndex hi{static_cast<std::uint32_t>(board.centres_.size())};
        board.centres_.push_back(centre);
        board.ids_.push_back(id);
        board.terrain_.push_back(gridDefault);
        board.byCentre_[centre] = hi;
        board.byId_[id] = hi;
        orientationOf.push_back(g.spec().orientation);
      }
    }
    board.grids_ = grids;
    const std::size_t hexCount = board.centres_.size();

    // ---- neighbours, computed once so edge binding can use them -----------------------------------
    board.neighbours_.assign(hexCount * 6, std::nullopt);
    for (std::size_t i = 0; i < hexCount; ++i) {
      for (int d = 0; d < 6; ++d) {
        const HexCoord::HexCentre nc = board.centres_[i].neighbour(static_cast<HexCoord::Direction>(d));
        const auto it = board.byCentre_.find(nc);
        if (board.byCentre_.end() != it) {
          board.neighbours_[i * 6 + static_cast<std::size_t>(d)] = it->second;
        }
      }
    }
    board.edges_.assign(hexCount * 6, {});

    // ---- bulk and per-hex terrain, and features from bound glyph symbols --------------------------
    for (const HexXml::SheetHexesDoc& bulk : sheet.hexesBulk) {
      const TerrainId t = resolveHexTerrain(bySheetTerrain, bulk.terrain, rules);
      for (const std::string& idText : bulk.ids) {
        const auto it = board.byId_.find(HexCoord::HexId{idText});
        if (board.byId_.end() != it) {
          board.terrain_[it->second.value] = t;
        }
      }
    }

    board.features_.assign(hexCount, {});
    for (const HexXml::SheetHexDoc& h : sheet.hexes) {
      const auto it = board.byId_.find(HexCoord::HexId{h.id});
      if (board.byId_.end() == it) {
        continue;
      }
      const HexIndex hi = it->second;
      if (h.terrain) {
        board.terrain_[hi.value] = resolveHexTerrain(bySheetTerrain, *h.terrain, rules);
      }
      for (const HexXml::SheetGlyphDoc& glyph : h.glyphs) {
        if (!glyph.symbol) {
          continue;  // a legend mark; the package binds symbols only (TODO(decide): bind marks too)
        }
        const auto symIt = bySymbol.find(*glyph.symbol);
        if (bySymbol.end() != symIt) {
          Feature f;
          f.terrain = rules.terrain(symIt->second);
          f.name = h.name;
          board.features_[hi.value].push_back(std::move(f));
        }
      }
    }

    // ---- hexside terrain from edges and paths ------------------------------------------------------
    for (const HexXml::SheetEdgeDoc& e : sheet.edges) {
      std::optional<std::string> rulesId;
      if (e.line) {
        const auto it = hexsideByLine.find(*e.line);
        if (hexsideByLine.end() != it) {
          rulesId = it->second;
        }
      }
      if (!rulesId && e.symbol) {
        const auto it = bySymbol.find(*e.symbol);
        if (bySymbol.end() != it) {
          rulesId = it->second;
        }
      }
      if (!rulesId) {
        continue;
      }
      recordEdge(board, orientationOf, e.at, rules.edgeTerrain(*rulesId));
    }
    for (const HexXml::SheetPathDoc& p : sheet.paths) {
      std::optional<std::string> rulesId;
      const auto lineIt = hexsideByLine.find(p.line);
      if (hexsideByLine.end() != lineIt) {
        rulesId = lineIt->second;
      }
      if (!rulesId) {
        const auto kindIt = hexsideByKind.find(p.kind);
        if (hexsideByKind.end() != kindIt) {
          rulesId = kindIt->second;
        }
      }
      if (!rulesId) {
        continue;
      }
      const EdgeTerrainId t = rules.edgeTerrain(*rulesId);
      for (const std::string& tok : p.edges) {
        recordEdge(board, orientationOf, tok, t);
      }
    }

    // ---- networks, from <link> elements --------------------------------------------------------
    board.networks_.assign(rules.networks().size(), LinkNetwork{});
    for (std::size_t i = 0; i < rules.networks().size(); ++i) {
      board.networkByName_[rules.networks()[i].id] = NetworkId{static_cast<std::uint32_t>(i)};
    }
    const bool explicitP = "explicit" == sheet.junctions;
    for (LinkNetwork& net : board.networks_) {
      net.explicit_ = explicitP;
    }
    // chain id -> (network, chain index), for the junction elements below
    std::map<std::string, std::pair<NetworkId, std::size_t>> chainById;
    for (const HexXml::SheetLinkDoc& link : sheet.links) {
      const auto it = networkKindToRules.find(link.kind);
      if (networkKindToRules.end() == it) {
        continue;
      }
      if (explicitP && !link.id) {
        throw std::invalid_argument(sheet.id + ":" + std::to_string(link.sourceLine) +
                                     ": link without id on a sheet with junctions=\"explicit\"");
      }
      const NetworkId netId = board.networkId(it->second);
      LinkNetwork& net = board.networks_[netId.value];
      const std::size_t chain = net.chains_.size();
      net.chains_.push_back(LinkNetwork::Chain{link.id.value_or(""), link.name});
      if (link.id) {
        if (!chainById.emplace(*link.id, std::make_pair(netId, chain)).second) {
          throw std::invalid_argument(sheet.id + ":" + std::to_string(link.sourceLine) +
                                       ": duplicate link id '" + *link.id + "'");
        }
      }
      std::optional<HexIndex> prev;
      for (const std::string& hexIdText : link.hexes) {
        const auto hit = board.byId_.find(HexCoord::HexId{hexIdText});
        if (board.byId_.end() == hit) {
          prev.reset();
          continue;
        }
        const HexIndex cur = hit->second;
        if (prev) {
          net.links_.push_back(LinkNetwork::Link{*prev, cur, link.kind, chain});
        }
        prev = cur;
      }
    }
    for (LinkNetwork& net : board.networks_) {
      net.byHex_.assign(hexCount, {});
      net.junctionsByHex_.assign(hexCount, {});
      for (std::size_t li = 0; li < net.links_.size(); ++li) {
        net.byHex_[net.links_[li].a.value].push_back(li);
        net.byHex_[net.links_[li].b.value].push_back(li);
      }
    }
    // ---- nodes, from <junction> elements (links only; a path junction is a river confluence, which
    // the Board's hexside terrain does not model) -------------------------------------------------
    for (const HexXml::SheetJunctionDoc& j : sheet.junctionElements) {
      if (j.links.empty()) {
        continue;
      }
      const auto hit = board.byId_.find(HexCoord::HexId{j.at});
      if (board.byId_.end() == hit) {
        throw std::invalid_argument(sheet.id + ":" + std::to_string(j.sourceLine) +
                                     ": junction at unknown hex '" + j.at + "'");
      }
      const HexIndex at = hit->second;
      std::optional<NetworkId> netId;
      LinkNetwork::Junction members;
      for (const std::string& linkId : j.links) {
        const auto cit = chainById.find(linkId);
        if (chainById.end() == cit) {
          throw std::invalid_argument(sheet.id + ":" + std::to_string(j.sourceLine) +
                                       ": junction names unknown link '" + linkId + "'");
        }
        if (netId && *netId != cit->second.first) {
          throw std::invalid_argument(sheet.id + ":" + std::to_string(j.sourceLine) +
                                       ": junction at '" + j.at + "' mixes networks (link '" + linkId + "')");
        }
        netId = cit->second.first;
        members.push_back(cit->second.second);
      }
      LinkNetwork& net = board.networks_[netId->value];
      std::sort(members.begin(), members.end());
      members.erase(std::unique(members.begin(), members.end()), members.end());
      if (members.size() < 2) {
        throw std::invalid_argument(sheet.id + ":" + std::to_string(j.sourceLine) +
                                     ": junction at '" + j.at + "' needs two distinct links");
      }
      for (std::size_t chain : members) {
        bool touchesP = false;
        for (std::size_t li : net.byHex_[at.value]) {
          touchesP = touchesP || net.links_[li].chain == chain;
        }
        if (!touchesP) {
          throw std::invalid_argument(sheet.id + ":" + std::to_string(j.sourceLine) + ": link '" +
                                       net.chains_[chain].id + "' does not pass through junction hex '" + j.at + "'");
        }
        for (const LinkNetwork::Junction& other : net.junctionsByHex_[at.value]) {
          if (std::binary_search(other.begin(), other.end(), chain)) {
            throw std::invalid_argument(sheet.id + ":" + std::to_string(j.sourceLine) + ": link '" +
                                         net.chains_[chain].id + "' is in two junctions of hex '" + j.at + "'");
          }
        }
      }
      net.junctionsByHex_[at.value].push_back(members);
    }

    // ---- region layers, from <region> elements (may legitimately be empty) ------------------------
    board.layers_.assign(rules.layers().size(), RegionLayer{});
    board.regionByNamePerLayer_.assign(rules.layers().size(), {});
    for (std::size_t li = 0; li < rules.layers().size(); ++li) {
      const HexRules::RegionLayerSpec& spec = rules.layers()[li];
      board.layerByName_[spec.id] = LayerId{static_cast<std::uint32_t>(li)};
      RegionLayer& layer = board.layers_[li];
      layer.partition_ = spec.partitionP;
      layer.names_ = spec.regionNames.empty() ? spec.regionIds : spec.regionNames;
      layer.members_.assign(spec.regionIds.size(), {});
      layer.byHex_.assign(hexCount, {});
      for (std::size_t ri = 0; ri < spec.regionIds.size(); ++ri) {
        board.regionByNamePerLayer_[li][spec.regionIds[ri]] = RegionId{static_cast<std::uint32_t>(ri)};
      }
    }
    for (const HexXml::SheetRegionDoc& region : sheet.regions) {
      const auto lb = layerSheetToRules.find(region.layer);
      const std::string rulesLayerId = layerSheetToRules.end() != lb ? lb->second : region.layer;
      const auto layerIt = board.layerByName_.find(rulesLayerId);
      if (board.layerByName_.end() == layerIt) {
        continue;
      }
      const LayerId layerId = layerIt->second;
      const HexRules::RegionLayerSpec& spec = rules.layers()[layerId.value];
      std::optional<std::size_t> ri;
      const auto nameIt = std::find(spec.regionNames.begin(), spec.regionNames.end(), region.name);
      if (spec.regionNames.end() != nameIt) {
        ri = static_cast<std::size_t>(std::distance(spec.regionNames.begin(), nameIt));
      } else {
        const auto idIt = std::find(spec.regionIds.begin(), spec.regionIds.end(), region.name);
        if (spec.regionIds.end() != idIt) {
          ri = static_cast<std::size_t>(std::distance(spec.regionIds.begin(), idIt));
        }
      }
      if (!ri) {
        continue;
      }
      RegionLayer& layer = board.layers_[layerId.value];
      for (const std::string& hexIdText : region.hexes) {
        const auto hit = board.byId_.find(HexCoord::HexId{hexIdText});
        if (board.byId_.end() == hit) {
          continue;
        }
        const HexIndex hi = hit->second;
        layer.members_[*ri].push_back(hi);
        layer.byHex_[hi.value].push_back(RegionId{static_cast<std::uint32_t>(*ri)});
      }
    }

    // ---- spaces, from the rules; a space of kind "track" also gets a TrackId ----------------------
    std::uint32_t nextTrack = 0;
    for (const HexRules::SpaceSpec& spec : rules.spaces()) {
      Space sp;
      sp.name = spec.name;
      sp.kind = parseSpaceKind(spec.kind);
      sp.sides = spec.sides;
      sp.returnsP = spec.returnsP;
      const SpaceId sid{static_cast<std::uint32_t>(board.spaces_.size())};
      board.spaceByName_[spec.id] = sid;
      board.spaces_.push_back(sp);
      board.trackOfSpace_.push_back(SpaceKind::Track == sp.kind ? std::optional<TrackId>(TrackId{nextTrack++})
                                                                  : std::nullopt);
    }
    board.trackCount_ = nextTrack;

    return board;
  }

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
