// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] What the viewer sees and has chosen, apart from the game: who is looking,
// the selection and hover, the highlights the interaction has computed, the zoom and the overlays. A
// value, copied freely; the InteractionMachine returns a new one for every intent.
// ----------------------------------------------
#pragma once
#include "hexmodel/Ids.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace HexView {

  // Level of detail, chosen from the zoom.
  enum class Lod : std::uint8_t {
    Overview,  // terrain, networks and cities only; stacks as coloured markers
    Map,       // hex ids, labels and glyphs; stacks as top counters
    Counters,  // full counter faces
    Detail     // stack fans, step marks, every marker
  };
  // Throws std::invalid_argument unless pixelsPerHex is positive.
  Lod lodFor(double pixelsPerHex);

  struct ViewState {
    std::optional<HexModel::SideId> viewer;  // nullopt: an all-seeing view (replay, referee)
    std::vector<HexModel::UnitId> selection;
    std::optional<HexModel::HexIndex> hover;
    std::vector<HexModel::HexIndex> reachable;  // from EngineFacade::reachable for the selection
    std::vector<HexModel::HexIndex> targets;    // from EngineFacade::attackTargets for the selection
    std::vector<HexModel::HexIndex> draftPath;  // the path being drawn, first hex the stack's own
    double pixelsPerHex = 40.0;
    bool showHexIdsP = true;
    bool showControlP = false;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
