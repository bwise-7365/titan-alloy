// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "Game.h"
#include "Graph.h"

#include <iosfwd>

// Qt-free writer for the saved-game format in doc/irrgo.xsd -- the same format
// irrgo_gui writes with QXmlStreamWriter, so a file written here loads in the GUI.
//
// It lives in irrgo_lib rather than in the bench driver because the headless batch
// runner cannot use the GUI's writer: linking Qt into a driver defeats the MB2 memory
// tracker (see absgame/MemTrack.h). The engine itself still performs no I/O of its own --
// the caller supplies the stream and owns it.
namespace IrrGo {

// Write `game` on `graph` to `out` as a complete XML document.
//
// `setupCount` is how many leading entries of game.moveHistory() were setup placements
// rather than played moves; they go in <Position type="setup"> and the rest in
// <move_list>, exactly as the GUI splits them by its setupPlaced_. A batch game with no
// pre-placed stones passes 0.
//
// `graph` must be the graph `game` was constructed over -- the writer emits the node
// labels and edges from it and indexes it with the move history's node ids. Throws
// std::invalid_argument if setupCount is negative or exceeds the history length, and
// std::runtime_error if the stream is in a failed state after writing: a save that
// silently produced a truncated file would be worse than none at all, given these are
// the inputs to the analysis run.
void writeGameXml(std::ostream& out, const Game& game, const Graph& graph,
                  int setupCount = 0);

}  // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.
