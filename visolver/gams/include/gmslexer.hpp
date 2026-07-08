// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Lexer for the GAMS subset (GP1): produces the final token stream with
// comments removed, $ONSYMLIST ignored, $include spliced in place, and
// $macro definitions captured then expanded at token level (nested macros
// supported via rescanning, with a depth guard). Anything outside the
// censused footprint throws std::invalid_argument with file:line:col.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// code is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this code is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VINCP_GMS_GMSLEXER_HPP
#define VINCP_GMS_GMSLEXER_HPP

#include "gmstoken.hpp"

#include <string>
#include <vector>

namespace VINCP::Gms {

  using std::string;
  using std::vector;

  struct LexResult {
    vector<Token> tokens;        // ends with EndOfInput
    vector<string> macroNames;   // $macro definitions, in definition order
  };

  // Lex a top-level file (resolving $include relative to it).
  LexResult lexGmsFile(const string& path);

  // Lex from a string; `pseudoFile` names it in error messages. $include is
  // resolved relative to `includeDir` (empty: the current directory).
  LexResult lexGmsString(const string& text, const string& pseudoFile,
                         const string& includeDir = string());

} // namespace VINCP::Gms

#endif // VINCP_GMS_GMSLEXER_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
