// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// The printing of the two coordinate triples, in tricoord's format: a three-character field for each
// component, so that columns of coordinates line up in a diagnostic listing.
// ----------------------------------------------
#include "Abc.h"

#include <iomanip>
#include <ostream>

namespace HexCoord {

  namespace {
    constexpr int kFieldWidth = 3;
  }  // namespace

  std::ostream&
  operator<<(std::ostream& os, Abc v)
  {
    os << "[ABC " << std::setw(kFieldWidth) << v.a() << ", " << std::setw(kFieldWidth) << v.b()
       << ", " << std::setw(kFieldWidth) << v.c() << "]";
    return os;
  }

  std::ostream&
  operator<<(std::ostream& os, Qrs v)
  {
    os << "[QRS " << std::setw(kFieldWidth) << v.q() << ", " << std::setw(kFieldWidth) << v.r()
       << ", " << std::setw(kFieldWidth) << v.s() << "]";
    return os;
  }

}  // namespace HexCoord
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
