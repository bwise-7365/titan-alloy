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

double lerp(double a, double b, double t) {
    return a + (b - a) * t;
}

Srgb lerp(const Srgb& a, const Srgb& b, double t) {
    return Srgb{lerp(a.r, b.r, t), lerp(a.g, b.g, t), lerp(a.b, b.b, t)};
}

Ryb lerp(const Ryb& a, const Ryb& b, double t) {
    return Ryb{lerp(a.r, b.r, t), lerp(a.y, b.y, t), lerp(a.b, b.b, t)};
}

// Increase RYB saturation toward the most-saturated color of the same hue.
// Ported from ArtColors Saturate() (positive-sat branch only, as the inverse
// transform calls it with sat = 0.5).
Ryb saturateRyb(const Ryb& in, double sat) {
    if (std::fabs(sat) < 0.004) {
        return in;
    }
    if (in.r == 0.0 && in.y == 0.0 && in.b == 0.0) {
        return in; // avoid division by zero on black
    }
    const double mx = std::max(std::max(in.r, in.y), in.b);
    const Ryb maxsat{in.r / mx, in.y / mx, in.b / mx};
    return lerp(in, maxsat, sat);
}

} // namespace

Srgb rybToSrgb(const Ryb& v) {
    requireUnit(v.r, "Ryb.r outside [0,1]");
    requireUnit(v.y, "Ryb.y outside [0,1]");
    requireUnit(v.b, "Ryb.b outside [0,1]");
    const double rin = clampUnit(v.r);
    const double yin = clampUnit(v.y);
    const double bin = clampUnit(v.b);

    // RYB-cube corners in sRGB (ArtColors RYB.cpp Xform_RYB2RGB, "artist's
    // color wheel" set). 000 = black, 111 = white.
    const Srgb CG000{0.0, 0.0, 0.0};  // black
    const Srgb CG100{1.0, 0.0, 0.0};  // red
    const Srgb CG010{0.9, 0.9, 0.0};  // yellow
    const Srgb CG001{0.0, 0.36, 1.0}; // blue
    const Srgb CG011{0.0, 0.9, 0.2};  // green   (yellow + blue)
    const Srgb CG110{1.0, 0.6, 0.0};  // orange  (red + yellow)
    const Srgb CG101{0.6, 0.0, 1.0};  // purple  (red + blue)
    const Srgb CG111{1.0, 1.0, 1.0};  // white

    const Srgb c00 = lerp(CG000, CG100, rin);
    const Srgb c01 = lerp(CG001, CG101, rin);
    const Srgb c10 = lerp(CG010, CG110, rin);
    const Srgb c11 = lerp(CG011, CG111, rin);
    const Srgb c0 = lerp(c00, c10, yin);
    const Srgb c1 = lerp(c01, c11, yin);
    return lerp(c0, c1, bin);
}

Ryb srgbToRyb(const Srgb& c) {
    requireUnit(c.r, "Srgb.r outside [0,1]");
    requireUnit(c.g, "Srgb.g outside [0,1]");
    requireUnit(c.b, "Srgb.b outside [0,1]");
    const double rin = clampUnit(c.r);
    const double gin = clampUnit(c.g);
    const double bin = clampUnit(c.b);

    // RGB-cube corners in RYB (ArtColors RYB.cpp Xform_RGB2RYB, fitted inverse).
    const Ryb CG000{0.0, 0.0, 0.0};     // black
    const Ryb CG100{0.891, 0.0, 0.0};   // red
    const Ryb CG010{0.0, 0.714, 0.374}; // green = RYB yellow + blue
    const Ryb CG001{0.07, 0.08, 0.893}; // blue
    const Ryb CG011{0.0, 0.116, 0.313}; // cyan  = RYB green + blue
    const Ryb CG110{0.0, 0.915, 0.0};   // yellow
    const Ryb CG101{0.554, 0.0, 0.1};   // magenta = RYB red + blue
    const Ryb CG111{1.0, 1.0, 1.0};     // white

    const Ryb c00 = lerp(CG000, CG100, rin);
    const Ryb c01 = lerp(CG001, CG101, rin);
    const Ryb c10 = lerp(CG010, CG110, rin);
    const Ryb c11 = lerp(CG011, CG111, rin);
    const Ryb c0 = lerp(c00, c10, gin);
    const Ryb c1 = lerp(c01, c11, gin);
    const Ryb mixed = lerp(c0, c1, bin);
    return saturateRyb(mixed, 0.5);
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
