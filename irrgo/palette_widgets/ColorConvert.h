// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <QColor>

#include "palette/color.h"

// The single QColor<->Srgb boundary (DESIGN.md §6). Everything Qt-side speaks
// QColor; everything core-side speaks palette::Srgb; conversion happens only
// here.
namespace palette_widgets {

inline palette::Srgb toSrgb(const QColor& c) {
    return palette::Srgb{c.redF(), c.greenF(), c.blueF()};
}

inline QColor toQColor(const palette::Srgb& s) {
    const auto clamp = [](double v) {
        if (v < 0.0) {
            return 0.0;
        }
        if (v > 1.0) {
            return 1.0;
        }
        return v;
    };
    return QColor::fromRgbF(static_cast<float>(clamp(s.r)),
                            static_cast<float>(clamp(s.g)),
                            static_cast<float>(clamp(s.b)));
}

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
