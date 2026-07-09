// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Parser for the GAMS subset (GP1): token stream -> Program AST. Exactly the
// censused grammar (doc/2026-07-08-gams-subset-census.md); constructs outside
// it throw std::invalid_argument with file:line:col.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// software is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this software is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VINCP_GMS_GMSPARSER_HPP
#define VINCP_GMS_GMSPARSER_HPP

#include "gmsast.hpp"
#include "gmstoken.hpp"

#include <string>
#include <vector>

namespace VINCP::Gms {

  using std::string;
  using std::vector;

  // Parse an already-lexed token stream (must end with EndOfInput).
  Program parseGmsTokens(const vector<Token>& tokens,
                         const vector<string>& macroNames);

  // Convenience: lex + parse a file / a string.
  Program parseGmsFile(const string& path);
  Program parseGmsString(const string& text,
                         const string& pseudoFile = "<string>");

} // namespace VINCP::Gms

#endif // VINCP_GMS_GMSPARSER_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
