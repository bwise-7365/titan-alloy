// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Canonical echo of a parsed GAMS-subset program (GP1). The output is NOT
// byte-identical to the source (comments and alignment are gone, macros are
// expanded, descriptions are quoted, expressions fully parenthesized); the
// round-trip guarantee is idempotence: parsing the echo yields an AST equal
// to the AST that produced it.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VIMCP_GMS_GMSECHO_HPP
#define VIMCP_GMS_GMSECHO_HPP

#include "gmsast.hpp"

#include <string>

namespace VIMCP::Gms {

  using std::string;

  string echoExpr(const Expr& expr);
  string echoProgram(const Program& program);

} // namespace VIMCP::Gms

#endif // VIMCP_GMS_GMSECHO_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
