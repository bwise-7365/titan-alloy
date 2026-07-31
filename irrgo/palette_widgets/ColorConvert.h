// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include <QColor>

#include <algorithm>

#include "palette/color.h"

// The single QColor<->Srgb boundary (DESIGN.md §6). Everything Qt-side speaks
// QColor; everything core-side speaks palette::Srgb; conversion happens only
// here.
namespace palette_widgets {

inline palette::Srgb toSrgb(const QColor& c) {
    return palette::Srgb{c.redF(), c.greenF(), c.blueF()};
}

inline QColor toQColor(const palette::Srgb& s) {
    return QColor::fromRgbF(static_cast<float>(std::clamp(s.r, 0.0, 1.0)),
                            static_cast<float>(std::clamp(s.g, 0.0, 1.0)),
                            static_cast<float>(std::clamp(s.b, 0.0, 1.0)));
}

} // namespace palette_widgets
// Copyright Ben Paul Wise. All Rights Reserved.
