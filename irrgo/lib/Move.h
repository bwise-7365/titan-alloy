// Copyright Ben Paul Wise. All Rights Reserved.
#pragma once
#include "Node.h"

namespace IrrGo {

// Color == Empty means a pass move; row and col are -1 in that case.
struct Move {
    int   turn   = 0;
    Color color  = Color::Empty;
    int   nodeId = -1;
    int   row    = -1;
    int   col    = -1;
};

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.
