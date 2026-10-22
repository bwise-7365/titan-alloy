// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The three addressable objects of a hex map -- centre, vertex, edge -- as distinct types whose
// invariants are checked once, in their explicit constructors, and never again.
// ----------------------------------------------
#pragma once
#include "Abc.h"
#include "Direction.h"

#include <array>
#include <stdexcept>
#include <utility>
#include <vector>

namespace HexCoord {

  class HexVertex;
  class HexEdge;

  constexpr bool
  isCentreCode(int hvCode)
  {
    return 0 == hvCode || 3 == hvCode;
  }

  // A hex centre: an Abc with hvCode 0 or 3.
  class HexCentre {
  public:
    // Throws std::invalid_argument unless abc.hvCode() is 0 or 3.
    constexpr explicit HexCentre(Abc abc) : abc_(abc)
    {
      if (!isCentreCode(abc.hvCode())) {
        throw std::invalid_argument("HexCentre: not a hex centre");
      }
    }
    // Total: every Qrs is a centre.
    static constexpr HexCentre fromQrs(Qrs) noexcept;

    constexpr Abc abc() const { return abc_; }
    // The QRS triple of this centre: the three differences (c-b, a-c, b-a), shifted by the one
    // residue that makes all three divisible by three (the invariant guarantees one exists).
    constexpr Qrs qrs() const;

    // Total operations: a centre plus a step is a centre; two centres differ by a step vector.
    constexpr HexCentre operator+(Qrs) const noexcept;
    constexpr Qrs operator-(HexCentre) const noexcept;

    constexpr HexCentre neighbour(Direction) const noexcept;
    HexEdge edge(Direction) const;
    // Clockwise, starting at the leading corner of Direction::D0.
    std::array<HexVertex, 6> vertices() const;

    constexpr auto operator<=>(const HexCentre&) const = default;

  private:
    constexpr explicit HexCentre(Abc abc, int) noexcept : abc_(abc) {}  // unchecked
    Abc abc_;
  };

  // A hex vertex: an Abc with hvCode 1, 2, 4 or 5.
  class HexVertex {
  public:
    // Throws std::invalid_argument unless abc.hvCode() is 1, 2, 4 or 5.
    constexpr explicit HexVertex(Abc abc) : abc_(abc)
    {
      if (isCentreCode(abc.hvCode())) {
        throw std::invalid_argument("HexVertex: not a hex vertex");
      }
    }

    constexpr Abc abc() const { return abc_; }
    // The three hexes meeting here: v + A, v + B, v + C for codes 1 and 4; v - A, v - B, v - C for
    // codes 2 and 5 (tricoord testHexVertex).
    std::array<HexCentre, 3> hexes() const;
    // The three vertices one edge away (the opposite signs).
    std::array<HexVertex, 3> neighbours() const;

    constexpr auto operator<=>(const HexVertex&) const = default;

  private:
    Abc abc_;
  };

  // A hexside: two adjacent vertices, kept in canonical order so equality is structural.
  class HexEdge {
  public:
    // The hexside of `centre` facing `direction`.
    static HexEdge between(HexCentre centre, Direction direction);
    // Throws std::invalid_argument unless edgeDist(a, b) == 1.
    static HexEdge of(HexVertex a, HexVertex b);

    constexpr std::pair<HexVertex, HexVertex> vertices() const;
    // The two hexes this edge separates, in no particular order.
    std::pair<HexCentre, HexCentre> hexes() const;
    // v1 + v2: twice the midpoint, a half-lattice point used by the pixel mapping.
    constexpr Abc twiceMidpoint() const;

    constexpr auto operator<=>(const HexEdge&) const = default;

  private:
    constexpr HexEdge(HexVertex lo, HexVertex hi) noexcept : lo_(lo), hi_(hi) {}
    HexVertex lo_;
    HexVertex hi_;
  };

  // Neighbourhood queries on the unbounded lattice; off-map filtering is the Board's job.
  std::array<HexCentre, 6> neighbours(HexCentre);                 // clockwise from D0
  std::vector<HexCentre> ring(HexCentre, int radius);             // radius >= 1, clockwise
  std::vector<HexCentre> disc(HexCentre, int radius);             // radius >= 0, centre first
  constexpr int hexDist(HexCentre, HexCentre) noexcept;

  // ---- definitions of the constexpr declarations above ----------------------------------------

  constexpr HexCentre
  HexCentre::fromQrs(Qrs v) noexcept
  {
    return HexCentre{v.toAbc(), 0};
  }

  // The ABC triple of a centre sums to a multiple of three, so shifting it by the one representative
  // that makes each difference divisible by three recovers the QRS triple exactly.
  constexpr Qrs
  HexCentre::qrs() const
  {
    const int u = abc_.c() - abc_.b();
    const int v = abc_.a() - abc_.c();
    const int w = abc_.b() - abc_.a();
    const int t = iMod(-u, 3);
    return Qrs{(u + t) / 3, (v + t) / 3, (w + t) / 3};
  }

  constexpr HexCentre
  HexCentre::operator+(Qrs d) const noexcept
  {
    return HexCentre{abc_ + d.toAbc(), 0};
  }

  constexpr Qrs
  HexCentre::operator-(HexCentre other) const noexcept
  {
    return HexCentre{abc_ - other.abc_, 0}.qrs();
  }

  constexpr HexCentre
  HexCentre::neighbour(Direction d) const noexcept
  {
    return *this + step(d);
  }

  constexpr std::pair<HexVertex, HexVertex>
  HexEdge::vertices() const
  {
    return {lo_, hi_};
  }

  constexpr Abc
  HexEdge::twiceMidpoint() const
  {
    return lo_.abc() + hi_.abc();
  }

  constexpr int
  hexDist(HexCentre l, HexCentre r) noexcept
  {
    return (l - r).height();
  }

}  // namespace HexCoord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
