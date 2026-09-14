// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The unit markers TRC keeps (hexsave unit/@status tokens), each named in one place so a typo
// cannot open a second, silent store. Side state lives in TrcState, not here (M6b).
// ----------------------------------------------
#pragma once
#include "hexmodel/Position.h"

#include <string>

namespace Trc::Markers {

  inline const std::string kAv = "av";             // took part in an automatic victory this impulse
  inline const std::string kAvFirst = "av-first";  // ... in the first impulse (16.2)
  inline const std::string kInvaded = "invaded";   // landed by sea invasion this turn (17.2)
  inline const std::string kDropped = "dropped";   // a paratroop corps that has dropped (18.0)
  inline const std::string kRailed = "railed";     // moved by rail this impulse (9.1)

  bool markedP(const HexModel::Position&, HexModel::UnitId, const std::string&);
  void mark(HexModel::Position&, HexModel::UnitId, const std::string&);
  void unmark(HexModel::Position&, HexModel::UnitId, const std::string&);

}  // namespace Trc::Markers
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
