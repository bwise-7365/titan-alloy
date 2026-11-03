// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The two height fields of HexKrieg's generator, on a regular raster laid over the sheet and read at
// each hex centre's true position (bilinear), so the fields know nothing of offset numbering.
//   fractal: midpoint displacement (HexKrieg FractalHeight): corners uniform in [1, 100]; each new
//            point is the mean of its parents plus a uniform perturbation of +/- d * 200 / (rows +
//            cols) * roughness, d its distance from them in raster steps.
//   perlin:  six octaves of 2-D Perlin noise (HexKrieg PerlinTerrainBuilder): a shuffled 256-entry
//            table, four gradients, the quintic fade; each octave doubles the frequency and keeps
//            `roughness` of the amplitude; the sum is divided by the total amplitude.
// ----------------------------------------------
#pragma once
#include "hexgen/GenMap.h"
#include "hexgen/Settings.h"

#include "hexcoord/Grid.h"

#include <random>
#include <vector>

namespace HexGen {

  // Samples at x0 + i dx, y0 + j dy, one per hex spacing along each axis, covering every centre.
  struct Raster {
    int nx = 0;
    int ny = 0;
    double x0 = 0.0;
    double y0 = 0.0;
    double dx = 1.0;
    double dy = 1.0;
    std::vector<double> v;  // row-major: v[j * nx + i]

    double& at(int i, int j) { return v[static_cast<std::size_t>(j * nx + i)]; }
    double at(int i, int j) const { return v[static_cast<std::size_t>(j * nx + i)]; }
    double sample(HexCoord::Pixel) const;  // bilinear, clamped to the raster
  };

  Raster rasterOver(const GenMap&);

  void fillFractal(Raster&, double roughness, std::mt19937_64&);
  void fillPerlin(Raster&, double persistence, std::mt19937_64&);

  // A field made by `method`, read at every hex centre in hex order.
  std::vector<double> fieldOf(const GenMap&, Method, double roughness, std::mt19937_64&);

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
