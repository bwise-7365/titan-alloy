// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// Token and source-position types for the GAMS-subset front end (GP1 of
// doc/2026-07-08-gams-frontend-plan.md). Identifiers carry both the original
// spelling (for echo) and a lower-cased key (GAMS is case-insensitive).
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// code is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this code is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#ifndef VINCP_GMS_GMSTOKEN_HPP
#define VINCP_GMS_GMSTOKEN_HPP

#include <string>

namespace VINCP::Gms {

  using std::string;

  struct SourcePos {
    string file;
    int line = 0;
    int column = 0;
  };

  enum class TokenKind {
    Identifier,   // name; text = original spelling, key = lower-cased
    Number,       // numeric literal; text = original spelling, value = parsed
    QuotedText,   // 'single' or "double" quoted; text = contents
    Plus,
    Minus,
    Star,
    Slash,
    Power,        // **
    LParen,
    RParen,
    Comma,
    Dot,
    DotDot,       // .. (equation definition)
    Semicolon,
    Assign,       // =
    RelEq,        // =e=
    RelGe,        // =g=
    RelLe,        // =l=
    Newline,      // significant only in list contexts; parser skips elsewhere
    EndOfInput,
  };

  struct Token {
    TokenKind kind = TokenKind::EndOfInput;
    string text;
    string key;          // lower-cased text (identifiers only)
    double value = 0.0;  // numbers only
    SourcePos pos;
  };

  // "file:line:col" for error messages.
  string describePos(const SourcePos& pos);

} // namespace VINCP::Gms

#endif // VINCP_GMS_GMSTOKEN_HPP
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
