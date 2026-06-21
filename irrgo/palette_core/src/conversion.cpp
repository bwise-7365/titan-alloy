// Copyright Ben Paul Wise. All Rights Reserved.
#include "palette/conversion.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace palette {

namespace {

constexpr double kUnitEps = 1e-9;

void requireUnit(double v, const char* what) {
    if (v < -kUnitEps || v > 1.0 + kUnitEps) {
        throw std::out_of_range(what);
    }
}

double clampUnit(double v) {
    if (v < 0.0) {
        return 0.0;
    }
    if (v > 1.0) {
        return 1.0;
    }
    return v;
}

struct Vec3 {
    double x;
    double y;
    double z;
};

// RYB cube corners in sRGB (ArtColors RYB.cpp Xform_RYB2RGB, "artist's color
// wheel" set), indexed by idx = (r?1:0) + (y?2:0) + (b?4:0). 000 = black,
// 111 = white. These ported constants define the FORWARD map; the inverse below
// is a true numerical inversion of this same cube (DESIGN.md §7/§8 #5), so that
// srgbToRyb is a consistent inverse of rybToSrgb and hues round-trip.
const Vec3 kForwardCorners[8] = {
    {0.0, 0.0, 0.0},  // 000 black
    {1.0, 0.0, 0.0},  // 100 red
    {0.9, 0.9, 0.0},  // 010 yellow
    {1.0, 0.6, 0.0},  // 110 orange (red + yellow)
    {0.0, 0.36, 1.0}, // 001 blue
    {0.6, 0.0, 1.0},  // 101 purple (red + blue)
    {0.0, 0.9, 0.2},  // 011 green  (yellow + blue)
    {1.0, 1.0, 1.0},  // 111 white
};

// Trilinear (multilinear) interpolation of the cube at (r,y,b).
Vec3 forwardCube(double r, double y, double b) {
    Vec3 out{0.0, 0.0, 0.0};
    for (int i = 0; i < 2; ++i) {
        for (int j = 0; j < 2; ++j) {
            for (int k = 0; k < 2; ++k) {
                const double w =
                    (i ? r : 1.0 - r) * (j ? y : 1.0 - y) * (k ? b : 1.0 - b);
                const Vec3& corner = kForwardCorners[i + 2 * j + 4 * k];
                out.x += w * corner.x;
                out.y += w * corner.y;
                out.z += w * corner.z;
            }
        }
    }
    return out;
}

double dot3(const double a[3], const double b[3]) {
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Solve the symmetric 3x3 system A d = g via Cramer's rule. Returns false if A
// is (near) singular.
bool solve3x3(const double a[3][3], const double g[3], double d[3]) {
    const double det =
        a[0][0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
        a[0][1] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
        a[0][2] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]);
    if (std::fabs(det) < 1e-18) {
        return false;
    }
    const double inv = 1.0 / det;
    d[0] = inv * (g[0] * (a[1][1] * a[2][2] - a[1][2] * a[2][1]) -
                  a[0][1] * (g[1] * a[2][2] - a[1][2] * g[2]) +
                  a[0][2] * (g[1] * a[2][1] - a[1][1] * g[2]));
    d[1] = inv * (a[0][0] * (g[1] * a[2][2] - a[1][2] * g[2]) -
                  g[0] * (a[1][0] * a[2][2] - a[1][2] * a[2][0]) +
                  a[0][2] * (a[1][0] * g[2] - g[1] * a[2][0]));
    d[2] = inv * (a[0][0] * (a[1][1] * g[2] - g[1] * a[2][1]) -
                  a[0][1] * (a[1][0] * g[2] - g[1] * a[2][0]) +
                  g[0] * (a[1][0] * a[2][1] - a[1][1] * a[2][0]));
    return true;
}

} // namespace

Srgb rybToSrgb(const Ryb& v) {
    requireUnit(v.r, "Ryb.r outside [0,1]");
    requireUnit(v.y, "Ryb.y outside [0,1]");
    requireUnit(v.b, "Ryb.b outside [0,1]");
    const Vec3 c = forwardCube(clampUnit(v.r), clampUnit(v.y), clampUnit(v.b));
    return Srgb{c.x, c.y, c.z};
}

Ryb srgbToRyb(const Srgb& c) {
    requireUnit(c.r, "Srgb.r outside [0,1]");
    requireUnit(c.g, "Srgb.g outside [0,1]");
    requireUnit(c.b, "Srgb.b outside [0,1]");
    const double tx = clampUnit(c.r);
    const double ty = clampUnit(c.g);
    const double tz = clampUnit(c.b);

    // Levenberg-Marquardt inversion of the forward trilinear cube: find the RYB
    // in [0,1]^3 whose forward map best matches the target sRGB. For colors in
    // the forward gamut (e.g. anything rybToSrgb produced) this recovers the RYB
    // essentially exactly, so the hue round-trips.
    const auto cost = [&](double r, double y, double b) {
        const Vec3 f = forwardCube(r, y, b);
        const double dx = f.x - tx;
        const double dy = f.y - ty;
        const double dz = f.z - tz;
        return dx * dx + dy * dy + dz * dz;
    };

    double r = 0.5;
    double y = 0.5;
    double b = 0.5;
    double lambda = 1e-2;
    double curCost = cost(r, y, b);

    for (int iter = 0; iter < 80 && curCost > 1e-20; ++iter) {
        const Vec3 f = forwardCube(r, y, b);
        const double res[3] = {f.x - tx, f.y - ty, f.z - tz};

        // Analytic Jacobian columns (d forwardCube / d r, / d y, / d b).
        double jr[3] = {0.0, 0.0, 0.0};
        double jy[3] = {0.0, 0.0, 0.0};
        double jb[3] = {0.0, 0.0, 0.0};
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 2; ++j) {
                for (int k = 0; k < 2; ++k) {
                    const Vec3& corner = kForwardCorners[i + 2 * j + 4 * k];
                    const double dr = (i ? 1.0 : -1.0) * (j ? y : 1.0 - y) *
                                      (k ? b : 1.0 - b);
                    const double dy = (i ? r : 1.0 - r) * (j ? 1.0 : -1.0) *
                                      (k ? b : 1.0 - b);
                    const double db = (i ? r : 1.0 - r) * (j ? y : 1.0 - y) *
                                      (k ? 1.0 : -1.0);
                    jr[0] += dr * corner.x;
                    jr[1] += dr * corner.y;
                    jr[2] += dr * corner.z;
                    jy[0] += dy * corner.x;
                    jy[1] += dy * corner.y;
                    jy[2] += dy * corner.z;
                    jb[0] += db * corner.x;
                    jb[1] += db * corner.y;
                    jb[2] += db * corner.z;
                }
            }
        }

        // A = J^T J + lambda I ; g = J^T res.
        double a[3][3];
        a[0][0] = dot3(jr, jr) + lambda;
        a[0][1] = dot3(jr, jy);
        a[0][2] = dot3(jr, jb);
        a[1][0] = a[0][1];
        a[1][1] = dot3(jy, jy) + lambda;
        a[1][2] = dot3(jy, jb);
        a[2][0] = a[0][2];
        a[2][1] = a[1][2];
        a[2][2] = dot3(jb, jb) + lambda;
        const double g[3] = {dot3(jr, res), dot3(jy, res), dot3(jb, res)};

        double d[3];
        if (!solve3x3(a, g, d)) {
            break;
        }
        const double nr = clampUnit(r - d[0]);
        const double ny = clampUnit(y - d[1]);
        const double nb = clampUnit(b - d[2]);
        const double nc = cost(nr, ny, nb);
        if (nc < curCost) {
            r = nr;
            y = ny;
            b = nb;
            curCost = nc;
            lambda = std::max(lambda * 0.5, 1e-9);
        } else {
            lambda = std::min(lambda * 4.0, 1e6);
        }
    }

    return Ryb{r, y, b};
}

Ryb hsvRybToRyb(const HsvRyb& h) {
    requireUnit(h.sat, "HsvRyb.sat outside [0,1]");
    requireUnit(h.val, "HsvRyb.val outside [0,1]");
    const double sat = clampUnit(h.sat);
    const double val = clampUnit(h.val);

    double hue = std::fmod(h.hueDeg, 360.0);
    if (hue < 0.0) {
        hue += 360.0;
    }

    const double chroma = val * sat;
    const double x = chroma * (1.0 - std::fabs(std::fmod(hue / 60.0, 2.0) - 1.0));
    const double m = val - chroma;

    double r1 = 0.0;
    double y1 = 0.0;
    double b1 = 0.0;
    if (hue < 60.0) {
        r1 = chroma;
        y1 = x;
    } else if (hue < 120.0) {
        r1 = x;
        y1 = chroma;
    } else if (hue < 180.0) {
        y1 = chroma;
        b1 = x;
    } else if (hue < 240.0) {
        y1 = x;
        b1 = chroma;
    } else if (hue < 300.0) {
        r1 = x;
        b1 = chroma;
    } else {
        r1 = chroma;
        b1 = x;
    }
    return Ryb{r1 + m, y1 + m, b1 + m};
}

HsvRyb rybToHsvRyb(const Ryb& v) {
    requireUnit(v.r, "Ryb.r outside [0,1]");
    requireUnit(v.y, "Ryb.y outside [0,1]");
    requireUnit(v.b, "Ryb.b outside [0,1]");
    const double r = clampUnit(v.r);
    const double y = clampUnit(v.y);
    const double b = clampUnit(v.b);

    const double mx = std::max(std::max(r, y), b);
    const double mn = std::min(std::min(r, y), b);
    const double d = mx - mn;

    double hue = 0.0;
    if (d > 0.0) {
        if (mx == r) {
            hue = 60.0 * std::fmod((y - b) / d, 6.0);
        } else if (mx == y) {
            hue = 60.0 * (((b - r) / d) + 2.0);
        } else {
            hue = 60.0 * (((r - y) / d) + 4.0);
        }
    }
    if (hue < 0.0) {
        hue += 360.0;
    }

    double sat = 0.0;
    if (mx > 0.0) {
        sat = d / mx;
    }
    return HsvRyb{hue, sat, mx};
}

double hueOfSrgb(const Srgb& c) {
    return rybToHsvRyb(srgbToRyb(c)).hueDeg;
}

Srgb srgbAtHsvRyb(const HsvRyb& h) {
    return rybToSrgb(hsvRybToRyb(h));
}

} // namespace palette
// Copyright Ben Paul Wise. All Rights Reserved.
