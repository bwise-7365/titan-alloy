// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The Board: everything about a map that never changes during play. Built once by BoardBuilder from
// the sheet and rules documents, then shared read-only by every Session and thread.
// ----------------------------------------------
#pragma once
#include "hexcoord/Direction.h"
#include "hexcoord/Grid.h"
#include "hexcoord/HexAddress.h"
#include "hexmodel/Ids.h"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace HexModel {

  using HexCoord::Direction;
  using HexCoord::HexCentre;
  using HexCoord::HexId;

  // A drawn feature that the rules read as a property of the hex (city, port, oil field, river hex).
  struct Feature {
    TerrainId terrain;             // the rules hex-terrain it stands for
    std::optional<std::string> name;  // "Moscow", "Dnieper"
  };

  // A centre-to-centre network: rail, road, position connectors.
  class LinkNetwork {
  public:
    struct Link {
      HexIndex a;
      HexIndex b;
      std::string kind;  // sheet link kind, e.g. "rail"
    };
    const std::vector<Link>& links() const { return links_; }           // insertion order
    const std::vector<std::size_t>& linksAt(HexIndex) const;            // indices into links()
    bool connectedP(HexIndex, HexIndex) const;
    std::size_t linkCount() const { return links_.size(); }

  private:
    friend class BoardBuilder;
    std::vector<Link> links_;
    std::vector<std::vector<std::size_t>> byHex_;
  };

  // One labelling of hexes: a partition (countries) or overlapping (naval zones).
  class RegionLayer {
  public:
    bool partitionP() const { return partition_; }
    std::size_t regionCount() const { return names_.size(); }
    const std::vector<RegionId>& regionsOf(HexIndex) const;
    const std::vector<HexIndex>& members(RegionId) const;
    const std::string& name(RegionId) const;

  private:
    friend class BoardBuilder;
    bool partition_ = true;
    std::vector<std::string> names_;
    std::vector<std::vector<HexIndex>> members_;
    std::vector<std::vector<RegionId>> byHex_;
  };

  enum class SpaceKind : std::uint8_t { Box, Pool, Track, Display };

  struct Space {
    std::string name;
    SpaceKind kind = SpaceKind::Box;
    SideMask sides;
    bool returnsP = false;  // counters placed here can come back into play
  };

  class Board {
  public:
    std::size_t hexCount() const { return centres_.size(); }
    HexCentre centre(HexIndex) const;
    std::optional<HexIndex> index(HexCentre) const;
    const HexId& id(HexIndex) const;
    HexIndex indexOf(const HexId&) const;  // throws std::invalid_argument for an unknown id
    std::optional<HexIndex> find(const HexId&) const;

    // Precomputed; nullopt off the map.
    std::optional<HexIndex> neighbour(HexIndex, Direction) const;
    int distance(HexIndex, HexIndex) const;

    TerrainId terrain(HexIndex) const;
    const std::vector<Feature>& features(HexIndex) const;
    // The hexside terrains on this side of this hex (a river and a border may share a hexside).
    const std::vector<EdgeTerrainId>& edge(HexIndex, Direction) const;

    const LinkNetwork& network(NetworkId) const;
    const RegionLayer& layer(LayerId) const;
    const Space& space(SpaceId) const;
    std::size_t spaceCount() const { return spaces_.size(); }
    std::size_t networkCount() const { return networks_.size(); }
    std::size_t layerCount() const { return layers_.size(); }
    std::size_t trackCount() const { return trackCount_; }

    // Name lookups for builders and loaders; each throws std::invalid_argument naming the id. The
    // dense ids are minted by BoardBuilder in the declaration order of the rules document's own
    // networks()/layers()/spaces() lists, so these are the Board-side mirror of RuleSet's own
    // side()/unitType()/terrain() lookups.
    NetworkId networkId(const std::string& id) const;
    LayerId layerId(const std::string& id) const;
    RegionId regionId(LayerId, const std::string& id) const;
    SpaceId spaceId(const std::string& id) const;
    // nullopt if the space is not of SpaceKind::Track.
    std::optional<TrackId> trackOf(SpaceId) const;
    TrackId trackId(const std::string& id) const;  // spaceId(id) then trackOf(), throws if not a track

    // The sheet's grids, for pixels and for printed ids; several when a game has two map sheets.
    const std::vector<HexCoord::Grid>& grids() const { return grids_; }
    HexCoord::Orientation orientation() const;

  private:
    friend class BoardBuilder;
    std::vector<HexCentre> centres_;
    std::vector<HexId> ids_;
    std::vector<TerrainId> terrain_;
    std::vector<std::vector<Feature>> features_;
    std::vector<std::vector<EdgeTerrainId>> edges_;        // hexCount * 6
    std::vector<std::optional<HexIndex>> neighbours_;      // hexCount * 6
    std::vector<LinkNetwork> networks_;
    std::vector<RegionLayer> layers_;
    std::vector<Space> spaces_;
    std::vector<HexCoord::Grid> grids_;

    // Lookup indices, built once by BoardBuilder alongside the vectors above; ordered maps, never
    // unordered_*, per house rule even though nothing here iterates them.
    std::map<HexCentre, HexIndex> byCentre_;
    std::map<HexId, HexIndex> byId_;
    std::map<std::string, NetworkId> networkByName_;
    std::map<std::string, LayerId> layerByName_;
    std::vector<std::map<std::string, RegionId>> regionByNamePerLayer_;  // one map per LayerId
    std::map<std::string, SpaceId> spaceByName_;
    std::vector<std::optional<TrackId>> trackOfSpace_;  // one entry per SpaceId
    std::size_t trackCount_ = 0;
  };

}  // namespace HexModel
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
