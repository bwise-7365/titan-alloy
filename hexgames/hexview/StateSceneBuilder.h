// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] The play state over the map: control, stacks of counter faces, markers and
// the ViewState's highlights, into the Control, Units, Markers and Highlights layers. Concealment
// follows the rules (HexRules::Concealment): a hidden unit the viewer may not look at is drawn with its
// back face -- with conceals="values" its type still shows, with conceals="identity" only the back --
// and hidden-from="all" hides it from both players. Pure: the same position and view give the same
// scene.
// ----------------------------------------------
#pragma once
#include "hexmodel/Position.h"
#include "hexrules/Package.h"
#include "hexview/FaceModel.h"
#include "hexview/MapFrame.h"
#include "hexview/Scene.h"
#include "hexview/ViewState.h"

namespace HexView {

  class StateSceneBuilder {
  public:
    StateSceneBuilder(const HexRules::GameDefinition&, const MapFrame&, const FaceResolver&, const FaceLayout&);

    // Adds the play-state layers for `position` as `view` sees it. Throws std::invalid_argument naming
    // the unit when a counter has no face in the counters document.
    void build(const HexModel::Position& position, const ViewState& view, Scene& scene) const;

  private:
    const HexRules::GameDefinition& definition_;
    const MapFrame& frame_;
    const FaceResolver& faces_;
    const FaceLayout& layout_;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
