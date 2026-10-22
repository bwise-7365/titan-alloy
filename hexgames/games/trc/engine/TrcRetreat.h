// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// TRC retreat (13.4-13.7): into an adjacent land hex, not across a blocked hexside, not onto an
// enemy unit or into an enemy zone of control (a partisan's included). Units in woods stay (14.2);
// leaders and workers surrender instead of retreating (11.3, 22.2). The engine's combat plan does
// the rest: the attacker routes, a walk may not double back, and a route into elimination is
// refused while another exists.
// ----------------------------------------------
#pragma once
#include "TrcFacts.h"
#include "TrcZoc.h"

namespace Trc {

  class TrcRetreat : public HexEngine::RetreatPolicy {
  public:
    TrcRetreat(const TrcFacts&, const TrcZoc&);
    std::span<const std::string_view> claims() const override;
    std::vector<HexIndex> candidates(const Ctx&, UnitId, HexIndex origin) const override;
    HexEngine::RetreatFate fate(const Ctx&, UnitId) const override;

  private:
    const TrcFacts& facts_;
    const TrcZoc& zoc_;
  };

}  // namespace Trc
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
