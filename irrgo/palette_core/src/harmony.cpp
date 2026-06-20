// Copyright Ben Paul Wise. All Rights Reserved.
#include "palette/harmony.hpp"

#include <cmath>

namespace palette {

double normalizeHue(double deg) {
    double h = std::fmod(deg, 360.0);
    if (h < 0.0) {
        h += 360.0;
    }
    return h;
}

double hueDistance(double a, double b) {
    double d = std::fabs(normalizeHue(a) - normalizeHue(b));
    if (d > 180.0) {
        d = 360.0 - d;
    }
    return d;
}

double circularMidpoint(double a, double b) {
    const double na = normalizeHue(a);
    double nb = normalizeHue(b);
    // Move b onto the same side as a so the average is along the shorter arc.
    if (std::fabs(na - nb) > 180.0) {
        if (nb < na) {
            nb += 360.0;
        } else {
            nb -= 360.0;
        }
    }
    return normalizeHue((na + nb) / 2.0);
}

double complementHue(double h) {
    return normalizeHue(h + 180.0);
}

HuePair pieceHuesForBackground(double bgHue, Harmony t, double alphaDeg) {
    const double comp = complementHue(bgHue);
    switch (t) {
        case Harmony::Complement: {
            // Both pieces opposite the board; distinction comes from luminance.
            return HuePair{comp, comp};
        }
        case Harmony::SplitComplement: {
            return HuePair{normalizeHue(comp - alphaDeg),
                           normalizeHue(comp + alphaDeg)};
        }
        case Harmony::Triad: {
            return HuePair{normalizeHue(bgHue + 120.0),
                           normalizeHue(bgHue - 120.0)};
        }
        case Harmony::Analogous: {
            // Not recommended against the board (pieces blend with it), but
            // honored if explicitly requested.
            return HuePair{normalizeHue(bgHue - alphaDeg),
                           normalizeHue(bgHue + alphaDeg)};
        }
        case Harmony::Tetrad: {
            // The off-board diagonal pair of the rectangle; 180 apart from each
            // other, both 90 off the board hue.
            return HuePair{normalizeHue(bgHue + 90.0),
                           normalizeHue(bgHue + 270.0)};
        }
    }
    return HuePair{comp, comp};
}

double partnerHueForPiece(double pieceHue, Harmony t, double alphaDeg) {
    switch (t) {
        case Harmony::Complement: {
            return complementHue(pieceHue);
        }
        case Harmony::SplitComplement: {
            return normalizeHue(pieceHue + 180.0 + alphaDeg);
        }
        case Harmony::Triad: {
            return normalizeHue(pieceHue + 120.0);
        }
        case Harmony::Analogous: {
            return normalizeHue(pieceHue + alphaDeg);
        }
        case Harmony::Tetrad: {
            return normalizeHue(pieceHue + 90.0);
        }
    }
    return complementHue(pieceHue);
}

} // namespace palette
// Copyright Ben Paul Wise. All Rights Reserved.
