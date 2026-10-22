// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once

#include "Game.h"

#include <iosfwd>
#include <string>

// Qt-free writer for the saved-game format in doc/latrunculi.xsd -- the same format
// latrunculi_gui writes with QXmlStreamWriter, so a file written here loads in the GUI.
//
// It lives in latrunculi_lib rather than in the bench driver because the headless batch
// runner cannot use the GUI's writer: linking Qt into a driver defeats the MB2 memory
// tracker (see absgame/MemTrack.h). The engine itself still performs no I/O of its own --
// the caller supplies the stream and owns it.
namespace Latrunculi {

// The three colours the format requires (doc/latrunculi.xsd makes <colors> mandatory
// with all three attributes). Defaults mirror latgui::kDefaultSideA / kDefaultSideB /
// kDefaultBackground in latrunculi_gui/DisplayConstants.h, so a batch-written file opens
// looking like a GUI-written one. They are duplicated as literals rather than included
// from there because that header is Qt, and this translation unit must stay Qt-free.
struct XmlColors {
    std::string sideA = "#000000";
    std::string sideB = "#80c0a0";
    std::string background = "#f5e8c7";
};

// Write `game` to `out` as a complete XML document. The stream is written with the
// classic locale so a comma decimal separator can never turn komi into "1,5" and break
// the schema's half-integer pattern.
//
// Throws std::invalid_argument if any colour is not #rrggbb, and std::runtime_error if
// the stream is in a failed state after writing -- a save that silently produced a
// truncated file would be worse than none at all, given these are the inputs to the
// analysis run.
void writeGameXml(std::ostream& out, const Game& game, const XmlColors& colors = {});

}  // namespace Latrunculi
// Copyright Ben Paul Wise. All Rights Reserved.
