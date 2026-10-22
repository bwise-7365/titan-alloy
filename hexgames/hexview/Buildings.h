// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// hexview-internal: an urban hex drawn as a few scattered buildings, after hexsheet2svg.py's
// buildings() (itself after panj/tempest's drawBuildings): 4 or 5 axis-aligned rectangles of six
// sizes around the glyph point; two may overlap a little (an L-shaped building) but neither may
// cover 15% of the smaller, and their centres stay 0.30 hex sizes apart.
//
// The layout is the reference from 2026-09-23 on (Ben): the Python generator (random.Random seeded
// by a CRC) cannot be reproduced in C++, so the C++ draws its own, portably:
//   seed = splitMix64(fnv1a64(sheetId) ^ splitMix64(fnv1a64(hexId)))  (FNV-1a over the UTF-8 bytes)
//   std::mt19937_64 gen(seed); a uniform real is (gen() >> 11) * 2^-53, a choice among n is
//   gen() % n; no <random> distribution is used, since their output differs between libraries.
// Draws, in order: the count (4 + choice of 2); then per attempt (at most 200) a size (choice of
// 6), the centre's x offset (uniform in +-0.55 sizes) and its y offset (uniform in +-0.48 sizes).
// ----------------------------------------------
#pragma once
#include "hexview/Scene.h"

#include <cstdint>
#include <string_view>
#include <vector>

namespace HexView {

  struct Box {
    double x0, y0, x1, y1;
  };

  std::uint64_t fnv1a64(std::string_view);
  std::uint64_t splitMix64(std::uint64_t);

  std::vector<Box> scatterBuildings(std::string_view sheetId, std::string_view hexId, Pixel at,
                                    double size);

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
