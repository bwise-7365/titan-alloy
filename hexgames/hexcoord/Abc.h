// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The ABC coordinate algebra: three unit vectors A, B, C with A + B + C = 0, naming hex centres and
// hex vertices alike; QRS names centre-to-centre steps (Q = C - B, R = A - C, S = B - A).
// Semantics reproduce panj/hexmap/libsrc/tricoord.{h,cpp} exactly; the algebra is constexpr and
// lives here in full because the lattice constants below need it at compile time.
// ----------------------------------------------
#pragma once
#include <compare>
#include <iosfwd>

namespace HexCoord {

  constexpr int
  iAbs(int v)
  {
    return v < 0 ? -v : v;
  }

  // Floor modulus: iMod(-1, 6) == 5. The one arithmetic helper the algebra needs.
  constexpr int
  iMod(int k, int n)
  {
    const int m = k % n;
    return m < 0 ? m + n : m;
  }

  namespace Detail {
    // tricoord's reduce(): among the three ways of zeroing one component, keep the one of least
    // height, breaking ties in favour of zeroing the first, then the second, then the third.
    struct Triple {
      int x, y, z;
    };
    constexpr Triple
    reduce(int x, int y, int z)
    {
      const int hx = iAbs(y - x) + iAbs(z - x);
      const int hy = iAbs(x - y) + iAbs(z - y);
      const int hz = iAbs(x - z) + iAbs(y - z);
      if (hy < hx && hy <= hz) {
        return {x - y, 0, z - y};
      }
      if (hz < hx && hz < hy) {
        return {x - z, y - z, 0};
      }
      return {0, y - x, z - x};
    }
  }  // namespace Detail

  // A point of the vertex lattice, stored as its canonical representative.
  //
  // (a, b, c) and (a + d, b + d, c + d) are the same point because A + B + C = 0. The constructor
  // reduces, so equality of two Abc values is structural equality of the stored triples, and
  // hvCode() -- which is invariant under +(d, d, d) -- may be computed on the stored triple.
  class Abc {
  public:
    constexpr Abc() = default;
    constexpr Abc(int a, int b, int c)
    {
      const Detail::Triple t = Detail::reduce(a, b, c);
      a_ = t.x;
      b_ = t.y;
      c_ = t.z;
    }

    constexpr int a() const { return a_; }
    constexpr int b() const { return b_; }
    constexpr int c() const { return c_; }

    // |a| + |b| + |c| of the canonical representative; tricoord's height().
    constexpr int height() const { return iAbs(a_) + iAbs(b_) + iAbs(c_); }

    // iMod(2a - (b + c), 6): 0 or 3 for a hex centre (even or odd column in the flat offset
    // frame), 1, 2, 4, 5 for a vertex (which side it opens to, and to which column parity).
    constexpr int hvCode() const { return iMod(2 * a_ - (b_ + c_), 6); }

    constexpr auto operator<=>(const Abc&) const = default;

    friend constexpr Abc operator+(Abc l, Abc r) { return Abc{l.a_ + r.a_, l.b_ + r.b_, l.c_ + r.c_}; }
    friend constexpr Abc operator-(Abc l, Abc r) { return Abc{l.a_ - r.a_, l.b_ - r.b_, l.c_ - r.c_}; }
    friend constexpr Abc operator*(Abc l, int f) { return Abc{l.a_ * f, l.b_ * f, l.c_ * f}; }

  private:
    int a_ = 0;
    int b_ = 0;
    int c_ = 0;
  };

  // A centre-to-centre displacement, stored reduced the same way.
  class Qrs {
  public:
    constexpr Qrs() = default;
    constexpr Qrs(int q, int r, int s)
    {
      const Detail::Triple t = Detail::reduce(q, r, s);
      q_ = t.x;
      r_ = t.y;
      s_ = t.z;
    }

    constexpr int q() const { return q_; }
    constexpr int r() const { return r_; }
    constexpr int s() const { return s_; }

    constexpr int height() const { return iAbs(q_) + iAbs(r_) + iAbs(s_); }

    // (r - s, s - q, q - r): every Qrs is a hex centre in ABC (hvCode 0 or 3).
    constexpr Abc toAbc() const { return Abc{r_ - s_, s_ - q_, q_ - r_}; }

    constexpr auto operator<=>(const Qrs&) const = default;

    friend constexpr Qrs operator+(Qrs l, Qrs r) { return Qrs{l.q_ + r.q_, l.r_ + r.r_, l.s_ + r.s_}; }
    friend constexpr Qrs operator-(Qrs l, Qrs r) { return Qrs{l.q_ - r.q_, l.r_ - r.r_, l.s_ - r.s_}; }
    friend constexpr Qrs operator*(Qrs l, int f) { return Qrs{l.q_ * f, l.r_ * f, l.s_ * f}; }

  private:
    int q_ = 0;
    int r_ = 0;
    int s_ = 0;
  };

  inline constexpr Abc AVec{1, 0, 0};
  inline constexpr Abc BVec{0, 1, 0};
  inline constexpr Abc CVec{0, 0, 1};
  inline constexpr Qrs QVec{1, 0, 0};
  inline constexpr Qrs RVec{0, 1, 0};
  inline constexpr Qrs SVec{0, 0, 1};

  // Distance in centre-to-centre steps; tricoord's hexDist.
  constexpr int
  hexDist(Qrs l, Qrs r)
  {
    return (l - r).height();
  }

  // Distance along hex edges between two lattice points; tricoord's edgeDist.
  // For any two centres, 1.5 * hexDist <= edgeDist <= 2 * hexDist, with equality at 2 on straight
  // Q, R or S lines.
  constexpr int
  edgeDist(Abc l, Abc r)
  {
    return (l - r).height();
  }

  std::ostream& operator<<(std::ostream&, Abc);  // "[ABC   1,   0,   0]"
  std::ostream& operator<<(std::ostream&, Qrs);  // "[QRS   1,   0,   0]"

}  // namespace HexCoord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
