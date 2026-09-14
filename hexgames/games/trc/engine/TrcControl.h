// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC control of cities, oil wells and rail junctions (17.2.1, 7.4, 9.4.2): the occupier controls
// a point; a vacant point in one side's zone of control only passes to that side; a vacant point
// in both zones is controlled by neither; otherwise the last owner keeps it. A partisan occupies
// and projects nothing for this purpose (19.2). Control is left alone while a battle's losses are
// still being taken (13.3) and re-evaluated once the battle is over.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"
#include "TrcZoc.h"

namespace Trc {

  class TrcControl {
  public:
    TrcControl(const TrcFacts&, const TrcZoc&);
    static std::span<const std::string_view> claims();
    Position update(const Ctx&, HexEngine::EventSink&) const;

  private:
    std::optional<SideId> ownerOf(const Ctx&, HexIndex) const;
    const TrcFacts& facts_;
    const TrcZoc& zoc_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
