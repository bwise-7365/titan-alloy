// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#include "hexmodel/Board.h"

#include <algorithm>
#include <stdexcept>

namespace HexModel {

  namespace {

    template <class Vec, class Index>
    const auto&
    at(const Vec& v, Index i, const char* what)
    {
      if (i.value >= v.size()) {
        throw std::invalid_argument(std::string("Board: ") + what + " index out of range");
      }
      return v[i.value];
    }

  }  // namespace

  const std::vector<std::size_t>&
  LinkNetwork::linksAt(HexIndex h) const
  {
    if (h.value >= byHex_.size()) {
      throw std::invalid_argument("LinkNetwork::linksAt: hex index out of range");
    }
    return byHex_[h.value];
  }

  bool
  LinkNetwork::connectedP(HexIndex a, HexIndex b) const
  {
    for (std::size_t idx : linksAt(a)) {
      const Link& link = links_[idx];
      if ((link.a == a && link.b == b) || (link.a == b && link.b == a)) {
        return true;
      }
    }
    return false;
  }

  const std::vector<LinkNetwork::Junction>&
  LinkNetwork::junctionsAt(HexIndex h) const
  {
    if (h.value >= junctionsByHex_.size()) {
      throw std::invalid_argument("LinkNetwork::junctionsAt: hex index out of range");
    }
    return junctionsByHex_[h.value];
  }

  bool
  LinkNetwork::switchP(HexIndex h, std::size_t from, std::size_t to) const
  {
    if (from >= chains_.size() || to >= chains_.size()) {
      throw std::invalid_argument("LinkNetwork::switchP: chain index out of range");
    }
    if (from == to) {
      return true;
    }
    bool fromTouchesP = false;
    bool toTouchesP = false;
    for (std::size_t idx : linksAt(h)) {
      fromTouchesP = fromTouchesP || links_[idx].chain == from;
      toTouchesP = toTouchesP || links_[idx].chain == to;
    }
    if (!fromTouchesP || !toTouchesP) {
      return false;
    }
    if (!explicit_) {
      return true;
    }
    for (const Junction& j : junctionsAt(h)) {
      const bool hasFromP = std::binary_search(j.begin(), j.end(), from);
      const bool hasToP = std::binary_search(j.begin(), j.end(), to);
      if (hasFromP && hasToP) {
        return true;
      }
    }
    return false;
  }

  const std::vector<RegionId>&
  RegionLayer::regionsOf(HexIndex h) const
  {
    if (h.value >= byHex_.size()) {
      throw std::invalid_argument("RegionLayer::regionsOf: hex index out of range");
    }
    return byHex_[h.value];
  }

  const std::vector<HexIndex>&
  RegionLayer::members(RegionId r) const
  {
    if (r.value >= members_.size()) {
      throw std::invalid_argument("RegionLayer::members: region id out of range");
    }
    return members_[r.value];
  }

  const std::string&
  RegionLayer::name(RegionId r) const
  {
    if (r.value >= names_.size()) {
      throw std::invalid_argument("RegionLayer::name: region id out of range");
    }
    return names_[r.value];
  }

  HexCentre
  Board::centre(HexIndex h) const
  {
    return at(centres_, h, "hex");
  }

  std::optional<HexIndex>
  Board::index(HexCentre c) const
  {
    const auto it = byCentre_.find(c);
    if (byCentre_.end() == it) {
      return std::nullopt;
    }
    return it->second;
  }

  const HexId&
  Board::id(HexIndex h) const
  {
    return at(ids_, h, "hex");
  }

  HexIndex
  Board::indexOf(const HexId& id) const
  {
    const auto it = byId_.find(id);
    if (byId_.end() == it) {
      throw std::invalid_argument("Board::indexOf: unknown hex id '" + id.text + "'");
    }
    return it->second;
  }

  std::optional<HexIndex>
  Board::find(const HexId& id) const
  {
    const auto it = byId_.find(id);
    if (byId_.end() == it) {
      return std::nullopt;
    }
    return it->second;
  }

  std::optional<HexIndex>
  Board::neighbour(HexIndex h, Direction d) const
  {
    const std::size_t idx = h.value * 6 + static_cast<std::size_t>(d);
    if (idx >= neighbours_.size()) {
      throw std::invalid_argument("Board::neighbour: hex index out of range");
    }
    return neighbours_[idx];
  }

  int
  Board::distance(HexIndex a, HexIndex b) const
  {
    return HexCoord::hexDist(centre(a), centre(b));
  }

  TerrainId
  Board::terrain(HexIndex h) const
  {
    return at(terrain_, h, "hex");
  }

  const std::vector<Feature>&
  Board::features(HexIndex h) const
  {
    return at(features_, h, "hex");
  }

  const std::vector<EdgeTerrainId>&
  Board::edge(HexIndex h, Direction d) const
  {
    const std::size_t idx = h.value * 6 + static_cast<std::size_t>(d);
    if (idx >= edges_.size()) {
      throw std::invalid_argument("Board::edge: hex index out of range");
    }
    return edges_[idx];
  }

  const LinkNetwork&
  Board::network(NetworkId n) const
  {
    return at(networks_, n, "network");
  }

  const RegionLayer&
  Board::layer(LayerId l) const
  {
    return at(layers_, l, "layer");
  }

  const Space&
  Board::space(SpaceId s) const
  {
    return at(spaces_, s, "space");
  }

  NetworkId
  Board::networkId(const std::string& id) const
  {
    const auto it = networkByName_.find(id);
    if (networkByName_.end() == it) {
      throw std::invalid_argument("Board::networkId: unknown network '" + id + "'");
    }
    return it->second;
  }

  LayerId
  Board::layerId(const std::string& id) const
  {
    const auto it = layerByName_.find(id);
    if (layerByName_.end() == it) {
      throw std::invalid_argument("Board::layerId: unknown layer '" + id + "'");
    }
    return it->second;
  }

  RegionId
  Board::regionId(LayerId l, const std::string& id) const
  {
    if (l.value >= regionByNamePerLayer_.size()) {
      throw std::invalid_argument("Board::regionId: layer id out of range");
    }
    const std::map<std::string, RegionId>& byName = regionByNamePerLayer_[l.value];
    const auto it = byName.find(id);
    if (byName.end() == it) {
      throw std::invalid_argument("Board::regionId: unknown region '" + id + "'");
    }
    return it->second;
  }

  SpaceId
  Board::spaceId(const std::string& id) const
  {
    const auto it = spaceByName_.find(id);
    if (spaceByName_.end() == it) {
      throw std::invalid_argument("Board::spaceId: unknown space '" + id + "'");
    }
    return it->second;
  }

  std::optional<TrackId>
  Board::trackOf(SpaceId s) const
  {
    if (s.value >= trackOfSpace_.size()) {
      throw std::invalid_argument("Board::trackOf: space id out of range");
    }
    return trackOfSpace_[s.value];
  }

  TrackId
  Board::trackId(const std::string& id) const
  {
    const SpaceId s = spaceId(id);
    const std::optional<TrackId> t = trackOf(s);
    if (!t) {
      throw std::invalid_argument("Board::trackId: space '" + id + "' is not a track");
    }
    return *t;
  }

  HexCoord::Orientation
  Board::orientation() const
  {
    if (grids_.empty()) {
      throw std::invalid_argument("Board::orientation: no grids");
    }
    return grids_.front().spec().orientation;
  }

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
