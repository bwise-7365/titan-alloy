// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include <QColor>

// Display-only constants for the Latrunculi GUI: default piece/background colors, the
// board overlay colors (suggestion, selection, hover, last-move), the overlay geometry
// (fractions of one square's pixel size, pen widths, translucency) and the window/panel
// sizing. Game rules/structure/score constants live in latrunculi_game/Game.h instead.
namespace latgui {

// ── Default colors (the user can recolor at runtime via the Board menu) ───────────
inline const QColor kDefaultSideA{0x00, 0x00, 0x00};      // dark olive: {0x1F, 0x20, 0x14}
inline const QColor kDefaultSideB{0x80, 0xC0, 0xA0};      // brick red: {0x85, 0x25, 0x32}
inline const QColor kDefaultBackground{0xF5, 0xE8, 0xC7}; // warm cream: {0xF5, 0xE8, 0xC7}

// ── Board overlay colors ──────────────────────────────────────────────────────────
inline const QColor kBoardSurround{"#33332f"};   // dark border painted behind the board
inline const QColor kSuggestionRing{"#27c24c"};  // green: engine's suggested move
inline const QColor kCaptiveRing{"#e23b3b"};     // red: enemy captive selected for removal
inline const QColor kSelectionBlue{"#2d7bf0"};   // blue: selected origin + legal destinations
inline const QColor kHoverOutline{"#ff8c00"};    // orange: hover preview outline
inline const QColor kLastMoveDotLight{255, 255, 255};  // dot on a dark piece
inline const QColor kLastMoveDotDark{25, 25, 25};      // dot on a light piece
inline const QColor kStopButtonBg{"#ffcc99"};    // Stop button background
inline const QColor kSwatchBorder{"#555555"};    // tally color-swatch border

// ── Overlay geometry ────────────────────────────────────────────────────────────────
// Radii are fractions of one square's pixel size (scale_).
inline constexpr double kRingRadiusFrac     = 0.46;  // suggestion/selection ring radius
inline constexpr double kDestinationDotFrac = 0.18;  // legal-destination dot radius
inline constexpr double kLastMoveDotFrac    = 0.15;  // last-move dot radius
inline constexpr double kHoverDiscFrac      = 0.40;  // hover ghost-disc radius
inline constexpr double kThinPenPx  = 2.5;           // suggestion ring + hover outline
inline constexpr double kThickPenPx = 3.0;           // captive + origin rings
inline constexpr int    kOverlayAlpha = 110;         // translucency of overlay fills
inline constexpr double kPieceDarkThreshold = 0.5;   // lightnessF below this -> light dot

// ── Window / panel sizing ─────────────────────────────────────────────────────────
inline constexpr int kPanelWidth          = 350;   // right column width (px)
inline constexpr int kBannerPointSize     = 28;    // banner font (default family)
inline constexpr int kBannerCustomPtSize  = 26;    // banner font (bundled Roman family)
inline constexpr int kSuggestedLogHeight  = 48;    // "Suggested:" text box height
inline constexpr int kBoardHintW = 850, kBoardHintH = 680;  // BoardWidget sizeHint
inline constexpr int kBoardMinW  = 360, kBoardMinH  = 360;  // BoardWidget minimumSizeHint

// ── Starting board (the Board-menu spinbox defaults) ────────────────────────────────
inline constexpr int kStartRows    = 8;
inline constexpr int kStartColumns = 10;
// Discs per side at startup. Set explicitly rather than taken from
// games::board::stones_per_side (which returns a fixed fraction of the board area, 25 on
// this board): 20 a side is the intended Latrunculi setup. Changing the size spinboxes
// still recomputes the count from the area -- only the startup value is pinned here.
inline constexpr int kStartPerSide = 20;

}  // namespace latgui
// Copyright Ben Paul Wise. All Rights Reserved.
