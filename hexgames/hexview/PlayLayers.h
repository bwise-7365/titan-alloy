// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The play state as plain shapes, until counter faces are built (FaceModel, StateSceneBuilder): each unit a
// square in its side's colour showing its strengths, up to four to a row in a hex, and what one command did
// marked on top. Untried units show "?" unless the viewer asks to see through them. Works for any game.
// ----------------------------------------------
#pragma once
#include "hexengine/Event.h"
#include "hexmodel/Board.h"
#include "hexmodel/Position.h"
#include "hexmodel/Roster.h"
#include "hexview/MapFrame.h"
#include "hexview/Scene.h"

#include <span>

namespace HexView {

  struct PlayLayerOptions {
    bool revealAllP = false;
  };

  void addUnits(Scene&, const MapFrame&, const HexModel::Board&, const HexModel::Roster&, const HexModel::Position&,
                const PlayLayerOptions&);
  // Paths moved and retreated along, battles and entries, from the events of one command.
  void addStepMarks(Scene&, const MapFrame&, const HexModel::Board&, std::span<const HexEngine::Event>);

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
