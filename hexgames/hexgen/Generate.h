// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The generator: settings in, sheet out, the same sheet for the same settings. A pure function with
// no state between calls, so the command and the editor share it and may run it on any thread.
// Steps, each drawing from its own stream (Random.h): the height field; the cover field; terrain by
// role; altitude, if asked; the road network; the roads of each edge, top, bottom, left, right.
// ----------------------------------------------
#pragma once
#include "hexgen/GenMap.h"
#include "hexgen/Settings.h"

#include "hexxml/SheetDoc.h"

namespace HexGen {

  // Throws std::invalid_argument if the settings fail validate().
  GenMap generateMap(const Settings&);
  HexXml::SheetDoc generate(const Settings&);

}  // namespace HexGen
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
