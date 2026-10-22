// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
#pragma once
// ScenePainter -- a hexview Scene into a QGraphicsScene, layer by layer, primitive by primitive: the
// one place in the editor where hexview's paths, texts and symbol uses become Qt items. It decides
// nothing about what is drawn; that is the Scene's, and so the same for the SVG writer and the HTML
// client. Grid-layer texts (the printed ids) can be left out, the editor's "show ids" toggle.

#include "hexview/Scene.h"
#include "hexview/SymbolLibrary.h"

class QGraphicsScene;

namespace HexQt {

  struct PaintOptions {
    bool idsVisibleP = true;
  };

  // Adds every primitive of `scene` to `target` in layer order. Throws std::invalid_argument naming
  // the symbol when a SymbolUse names one the library lacks.
  void paintScene(const HexView::Scene& scene, const HexView::SymbolLibrary& symbols, QGraphicsScene& target,
                  const PaintOptions& options);

}  // namespace HexQt
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
