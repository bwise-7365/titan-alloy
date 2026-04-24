// Copyright Ben Paul Wise. All Rights Reserved.
#include "Node.h"
#include <cstdio>

namespace IrrGo {

Node::Node(int id, int row, int col, float x, float y)
    : id(id), row(row), col(col), x(x), y(y)
{
    char buf[8];
    std::snprintf(buf, sizeof(buf), "R%02dC%02d", row, col);
    label = buf;
}

} // namespace IrrGo
// Copyright Ben Paul Wise. All Rights Reserved.
