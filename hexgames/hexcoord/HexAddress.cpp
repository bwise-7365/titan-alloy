// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Centres, vertices and hexsides. Everything here rests on one fact from tricoord's testHexVertex: a
// vertex of class 1 or 4 has its three hexes at +A, +B, +C, and one of class 2 or 5 at -A, -B, -C.
// The six vertices of a centre are therefore its six neighbours in the ABC lattice, and the two that
// bound the hexside in direction d are the two halves of that direction's step vector.
// ----------------------------------------------
#include "HexAddress.h"

#include <string>

namespace HexCoord {

  namespace {

    // The six vertex offsets of a centre, clockwise from the leading corner of Direction::D0 (which
    // is +B for both orientations: rotating the sheet rotates the picture, not the algebra).
    constexpr std::array<Abc, 6> kVertexOffsets = {BVec,      CVec * -1, AVec,
                                                   BVec * -1, CVec,      AVec * -1};

    constexpr bool
    plusVertexP(int hvCode)
    {
      if (1 == hvCode || 4 == hvCode) {
        return true;
      }
      if (2 == hvCode || 5 == hvCode) {
        return false;
      }
      throw std::invalid_argument("HexVertex: hvCode " + std::to_string(hvCode) +
                                  " is not a vertex class");
    }

  }  // namespace

  std::array<HexVertex, 6>
  HexCentre::vertices() const
  {
    return {HexVertex{abc_ + kVertexOffsets[0]}, HexVertex{abc_ + kVertexOffsets[1]},
            HexVertex{abc_ + kVertexOffsets[2]}, HexVertex{abc_ + kVertexOffsets[3]},
            HexVertex{abc_ + kVertexOffsets[4]}, HexVertex{abc_ + kVertexOffsets[5]}};
  }

  HexEdge
  HexCentre::edge(Direction d) const
  {
    return HexEdge::between(*this, d);
  }

  std::array<HexCentre, 3>
  HexVertex::hexes() const
  {
    const int sign = plusVertexP(abc_.hvCode()) ? 1 : -1;
    return {HexCentre{abc_ + AVec * sign}, HexCentre{abc_ + BVec * sign},
            HexCentre{abc_ + CVec * sign}};
  }

  std::array<HexVertex, 3>
  HexVertex::neighbours() const
  {
    const int sign = plusVertexP(abc_.hvCode()) ? -1 : 1;
    return {HexVertex{abc_ + AVec * sign}, HexVertex{abc_ + BVec * sign},
            HexVertex{abc_ + CVec * sign}};
  }

  // The corners of hexside d are the two consecutive vertices that start at that side: vertex k and
  // vertex k + 1 bound the side in direction Dk.
  HexEdge
  HexEdge::between(HexCentre centre, Direction direction)
  {
    const std::array<HexVertex, 6> corners = centre.vertices();
    const int k = static_cast<int>(direction);
    return of(corners[k], corners[iMod(k + 1, kDirections)]);
  }

  HexEdge
  HexEdge::of(HexVertex a, HexVertex b)
  {
    if (1 != edgeDist(a.abc(), b.abc())) {
      throw std::invalid_argument("HexEdge::of: vertices are " +
                                  std::to_string(edgeDist(a.abc(), b.abc())) +
                                  " edges apart, not adjacent");
    }
    if (a < b) {
      return HexEdge{a, b};
    }
    return HexEdge{b, a};
  }

  std::pair<HexCentre, HexCentre>
  HexEdge::hexes() const
  {
    const std::array<HexCentre, 3> ofLo = lo_.hexes();
    const std::array<HexCentre, 3> ofHi = hi_.hexes();
    std::vector<HexCentre> shared;
    for (const HexCentre& l : ofLo) {
      for (const HexCentre& r : ofHi) {
        if (l == r) {
          shared.push_back(l);
        }
      }
    }
    if (2 != shared.size()) {
      throw std::invalid_argument("HexEdge::hexes: " + std::to_string(shared.size()) +
                                  " hexes share this hexside, not two");
    }
    return {shared[0], shared[1]};
  }

  std::array<HexCentre, 6>
  neighbours(HexCentre centre)
  {
    return {centre.neighbour(Direction::D0), centre.neighbour(Direction::D1),
            centre.neighbour(Direction::D2), centre.neighbour(Direction::D3),
            centre.neighbour(Direction::D4), centre.neighbour(Direction::D5)};
  }

  // Out to the ring's leading corner, then six runs of `radius` steps, each run turning 60 degrees
  // further clockwise; the walk closes on its starting hex.
  std::vector<HexCentre>
  ring(HexCentre centre, int radius)
  {
    if (1 > radius) {
      throw std::invalid_argument("ring: radius " + std::to_string(radius) + " is less than one");
    }
    std::vector<HexCentre> out;
    HexCentre walker = centre + step(Direction::D0) * radius;
    for (int leg = 0; leg < kDirections; ++leg) {
      const Qrs along = step(rotate(Direction::D2, leg));
      for (int i = 0; i < radius; ++i) {
        out.push_back(walker);
        walker = walker + along;
      }
    }
    return out;
  }

  std::vector<HexCentre>
  disc(HexCentre centre, int radius)
  {
    if (0 > radius) {
      throw std::invalid_argument("disc: radius " + std::to_string(radius) + " is negative");
    }
    std::vector<HexCentre> out{centre};
    for (int r = 1; r <= radius; ++r) {
      const std::vector<HexCentre> shell = ring(centre, r);
      out.insert(out.end(), shell.begin(), shell.end());
    }
    return out;
  }

}  // namespace HexCoord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
