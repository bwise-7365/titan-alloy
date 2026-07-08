// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GmsLexer implementation: raw scanning, directive handling ($ONSYMLIST /
// $include / $macro), and token-level macro expansion.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// code is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this code is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#include "gmslexer.hpp"

#include <cctype>
#include <cstdlib>
#include <fstream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace VINCP::Gms {

  using std::map;

  string
  describePos(const SourcePos& pos)
  {
    std::ostringstream out;
    out << pos.file << ":" << pos.line << ":" << pos.column;
    return out.str();
  }

  namespace {

    [[noreturn]] void
    fail(const SourcePos& pos, const string& message)
    {
      throw std::invalid_argument(describePos(pos) + ": " + message);
    }

    string
    toLower(const string& text)
    {
      string low = text;
      for (char& c : low) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
      }
      return low;
    }

    bool
    identStartP(char c)
    {
      return 0 != std::isalpha(static_cast<unsigned char>(c)) || '_' == c;
    }

    bool
    identCharP(char c)
    {
      return 0 != std::isalnum(static_cast<unsigned char>(c)) || '_' == c;
    }

    bool
    digitP(char c)
    {
      return 0 != std::isdigit(static_cast<unsigned char>(c));
    }

    // A scanning cursor over one file's text.
    struct Cursor {
      const string& text;
      string file;
      size_t i = 0;
      int line = 1;
      int column = 1;

      bool doneP() const { return text.size() <= i; }
      char at() const { return text[i]; }
      char at(size_t off) const
      {
        return (text.size() <= i + off) ? '\0' : text[i + off];
      }
      void step()
      {
        if ('\n' == text[i]) {
          ++line;
          column = 1;
        }
        else {
          ++column;
        }
        ++i;
      }
      SourcePos pos() const { return SourcePos{file, line, column}; }
    };

    string
    readFileText(const string& path)
    {
      std::ifstream in(path, std::ios::binary);
      if (!in) {
        throw std::invalid_argument("cannot open GMS file: " + path);
      }
      std::ostringstream buffer;
      buffer << in.rdbuf();
      string text = buffer.str();
      // Strip a UTF-8 BOM if present.
      if (3 <= text.size() && '\xEF' == text[0] && '\xBB' == text[1]
          && '\xBF' == text[2]) {
        text.erase(0, 3);
      }
      return text;
    }

    string
    directoryOf(const string& path)
    {
      const size_t cut = path.find_last_of("/\\");
      if (string::npos == cut) {
        return string();
      }
      return path.substr(0, cut + 1);
    }

    // One raw token starting at non-space; the caller has handled newlines,
    // comments, and directives.
    Token
    scanToken(Cursor& cur)
    {
      Token token;
      token.pos = cur.pos();
      const char c = cur.at();

      if (identStartP(c)) {
        string text;
        while (!cur.doneP() && identCharP(cur.at())) {
          text += cur.at();
          cur.step();
        }
        token.kind = TokenKind::Identifier;
        token.text = text;
        token.key = toLower(text);
        return token;
      }

      if (digitP(c) || ('.' == c && digitP(cur.at(1)))) {
        string text;
        while (!cur.doneP() && digitP(cur.at())) {
          text += cur.at();
          cur.step();
        }
        if (!cur.doneP() && '.' == cur.at() && digitP(cur.at(1))) {
          text += '.';
          cur.step();
          while (!cur.doneP() && digitP(cur.at())) {
            text += cur.at();
            cur.step();
          }
        }
        if (!cur.doneP() && ('e' == cur.at() || 'E' == cur.at())) {
          const char sign = cur.at(1);
          if (digitP(sign)
              || (('+' == sign || '-' == sign) && digitP(cur.at(2)))) {
            text += cur.at();
            cur.step();
            if ('+' == cur.at() || '-' == cur.at()) {
              text += cur.at();
              cur.step();
            }
            while (!cur.doneP() && digitP(cur.at())) {
              text += cur.at();
              cur.step();
            }
          }
        }
        token.kind = TokenKind::Number;
        token.text = text;
        token.value = std::strtod(text.c_str(), nullptr);
        return token;
      }

      if ('\'' == c || '"' == c) {
        const char quote = c;
        cur.step();
        string text;
        while (!cur.doneP() && quote != cur.at() && '\n' != cur.at()) {
          text += cur.at();
          cur.step();
        }
        if (cur.doneP() || quote != cur.at()) {
          fail(token.pos, "unterminated quoted text");
        }
        cur.step();   // closing quote
        token.kind = TokenKind::QuotedText;
        token.text = text;
        return token;
      }

      switch (c) {
      case '+':
        cur.step();
        token.kind = TokenKind::Plus;
        token.text = "+";
        return token;
      case '-':
        cur.step();
        token.kind = TokenKind::Minus;
        token.text = "-";
        return token;
      case '*':
        cur.step();
        if (!cur.doneP() && '*' == cur.at()) {
          cur.step();
          token.kind = TokenKind::Power;
          token.text = "**";
          return token;
        }
        token.kind = TokenKind::Star;
        token.text = "*";
        return token;
      case '/':
        cur.step();
        token.kind = TokenKind::Slash;
        token.text = "/";
        return token;
      case '(':
        cur.step();
        token.kind = TokenKind::LParen;
        token.text = "(";
        return token;
      case ')':
        cur.step();
        token.kind = TokenKind::RParen;
        token.text = ")";
        return token;
      case ',':
        cur.step();
        token.kind = TokenKind::Comma;
        token.text = ",";
        return token;
      case ';':
        cur.step();
        token.kind = TokenKind::Semicolon;
        token.text = ";";
        return token;
      case '.':
        cur.step();
        if (!cur.doneP() && '.' == cur.at()) {
          cur.step();
          token.kind = TokenKind::DotDot;
          token.text = "..";
          return token;
        }
        token.kind = TokenKind::Dot;
        token.text = ".";
        return token;
      case '=': {
        // =e= / =g= / =l= (case-insensitive), else plain assignment.
        const char rel = cur.at(1);
        if ('=' == cur.at(2)
            && ('e' == std::tolower(static_cast<unsigned char>(rel))
                || 'g' == std::tolower(static_cast<unsigned char>(rel))
                || 'l' == std::tolower(static_cast<unsigned char>(rel)))) {
          const char low =
              static_cast<char>(std::tolower(static_cast<unsigned char>(rel)));
          cur.step();
          cur.step();
          cur.step();
          token.kind = ('e' == low)   ? TokenKind::RelEq
                       : ('g' == low) ? TokenKind::RelGe
                                      : TokenKind::RelLe;
          token.text = string("=") + low + "=";
          return token;
        }
        cur.step();
        token.kind = TokenKind::Assign;
        token.text = "=";
        return token;
      }
      default:
        break;
      }
      fail(token.pos,
           string("unexpected character '") + c
               + "' (outside the supported GAMS subset)");
    }

    struct Macro {
      vector<string> params;   // lower-cased parameter keys
      vector<Token> body;
    };

    // Pass 1 state shared across $include recursion.
    struct Prescan {
      map<string, Macro> macros;   // by lower-cased name
      vector<string> macroNames;   // original spellings, definition order
      int includeDepth = 0;
    };

    void prescanText(const string& text, const string& file,
                     const string& includeDir, Prescan& state,
                     vector<Token>& out);

    // Skip the rest of the current line WITHOUT consuming the newline.
    void
    skipToEol(Cursor& cur)
    {
      while (!cur.doneP() && '\n' != cur.at()) {
        cur.step();
      }
      return;
    }

    string
    restOfLineTrimmed(Cursor& cur)
    {
      string text;
      while (!cur.doneP() && '\n' != cur.at()) {
        text += cur.at();
        cur.step();
      }
      const size_t first = text.find_first_not_of(" \t\r");
      if (string::npos == first) {
        return string();
      }
      const size_t last = text.find_last_not_of(" \t\r");
      return text.substr(first, last - first + 1);
    }

    // $macro NAME(p1,p2,...) body-to-end-of-line
    void
    scanMacroDefinition(Cursor& cur, Prescan& state)
    {
      while (!cur.doneP() && (' ' == cur.at() || '\t' == cur.at())) {
        cur.step();
      }
      if (cur.doneP() || !identStartP(cur.at())) {
        fail(cur.pos(), "$macro: expected a macro name");
      }
      Token nameToken = scanToken(cur);
      Macro macro;
      if (cur.doneP() || '(' != cur.at()) {
        fail(cur.pos(), "$macro " + nameToken.text
                            + ": expected a parenthesized parameter list");
      }
      cur.step();   // (
      for (;;) {
        while (!cur.doneP() && (' ' == cur.at() || '\t' == cur.at())) {
          cur.step();
        }
        if (cur.doneP() || !identStartP(cur.at())) {
          fail(cur.pos(), "$macro " + nameToken.text
                              + ": expected a parameter name");
        }
        const Token param = scanToken(cur);
        macro.params.push_back(param.key);
        while (!cur.doneP() && (' ' == cur.at() || '\t' == cur.at())) {
          cur.step();
        }
        if (!cur.doneP() && ',' == cur.at()) {
          cur.step();
          continue;
        }
        if (!cur.doneP() && ')' == cur.at()) {
          cur.step();
          break;
        }
        fail(cur.pos(), "$macro " + nameToken.text
                            + ": expected ',' or ')' in the parameter list");
      }
      // Body: raw tokens to end of line.
      for (;;) {
        while (!cur.doneP()
               && (' ' == cur.at() || '\t' == cur.at() || '\r' == cur.at())) {
          cur.step();
        }
        if (cur.doneP() || '\n' == cur.at()) {
          break;
        }
        macro.body.push_back(scanToken(cur));
      }
      if (macro.body.empty()) {
        fail(nameToken.pos, "$macro " + nameToken.text + ": empty body");
      }
      state.macros[nameToken.key] = macro;
      state.macroNames.push_back(nameToken.text);
      return;
    }

    void
    handleDirective(Cursor& cur, const string& includeDir, Prescan& state,
                    vector<Token>& out)
    {
      const SourcePos pos = cur.pos();
      cur.step();   // $
      string word;
      while (!cur.doneP() && identCharP(cur.at())) {
        word += cur.at();
        cur.step();
      }
      const string key = toLower(word);
      if ("onsymlist" == key) {
        skipToEol(cur);
        return;
      }
      if ("include" == key) {
        const string name = restOfLineTrimmed(cur);
        if (name.empty()) {
          fail(pos, "$include: missing file name");
        }
        if (10 < state.includeDepth) {
          fail(pos, "$include nesting too deep (cycle?)");
        }
        const string path = includeDir + name;
        ++state.includeDepth;
        prescanText(readFileText(path), path, directoryOf(path), state, out);
        --state.includeDepth;
        return;
      }
      if ("macro" == key) {
        scanMacroDefinition(cur, state);
        return;
      }
      fail(pos, "$" + word + ": unsupported directive "
                    "(outside the censused GAMS subset)");
    }

    // Pass 1: raw tokens with comments removed, directives resolved, macro
    // definitions captured (NOT yet expanded). Appends to `out`; does not
    // append EndOfInput (the top-level caller does).
    void
    prescanText(const string& text, const string& file,
                const string& includeDir, Prescan& state, vector<Token>& out)
    {
      Cursor cur{text, file};
      bool lineStartP = true;
      while (!cur.doneP()) {
        const char c = cur.at();
        if ('\n' == c) {
          Token nl;
          nl.kind = TokenKind::Newline;
          nl.pos = cur.pos();
          out.push_back(nl);
          cur.step();
          lineStartP = true;
          continue;
        }
        if (' ' == c || '\t' == c || '\r' == c) {
          // Whitespace ENDS line-start status: comment '*' and directives
          // must sit in column 1 exactly. This matters — deploy_v09 has
          // expression continuation lines that begin (after indentation)
          // with '*' as multiplication.
          cur.step();
          lineStartP = false;
          continue;
        }
        if (lineStartP && '*' == c) {
          skipToEol(cur);   // comment line; its newline is emitted next pass
          continue;
        }
        if (lineStartP && '$' == c) {
          handleDirective(cur, includeDir, state, out);
          continue;
        }
        out.push_back(scanToken(cur));
        lineStartP = false;
      }
      return;
    }

    // Pass 2: token-level macro expansion with rescanning.
    void
    expandInto(const vector<Token>& in, const map<string, Macro>& macros,
               int depth, vector<Token>& out)
    {
      if (100 < depth) {
        throw std::invalid_argument(
            "macro expansion nested more than 100 deep (cycle?)");
      }
      size_t i = 0;
      while (i < in.size()) {
        const Token& token = in[i];
        const bool macroP = TokenKind::Identifier == token.kind
                            && 0 < macros.count(token.key);
        if (!macroP) {
          out.push_back(token);
          ++i;
          continue;
        }
        const Macro& macro = macros.at(token.key);
        if (in.size() <= i + 1 || TokenKind::LParen != in[i + 1].kind) {
          fail(token.pos, "macro " + token.text
                              + " used without an argument list");
        }
        // Collect argument token runs (balanced parens, top-level commas).
        vector<vector<Token>> args(1);
        size_t j = i + 2;
        int parenDepth = 1;
        for (; j < in.size(); ++j) {
          const Token& t = in[j];
          if (TokenKind::LParen == t.kind) {
            ++parenDepth;
          }
          else
          if (TokenKind::RParen == t.kind) {
            --parenDepth;
            if (0 == parenDepth) {
              break;
            }
          }
          else
          if (TokenKind::Comma == t.kind && 1 == parenDepth) {
            args.emplace_back();
            continue;
          }
          if (TokenKind::Newline != t.kind) {
            args.back().push_back(t);
          }
        }
        if (in.size() <= j) {
          fail(token.pos, "macro " + token.text
                              + ": unterminated argument list");
        }
        if (args.size() != macro.params.size()) {
          std::ostringstream msg;
          msg << "macro " << token.text << ": expected "
              << macro.params.size() << " arguments, got " << args.size();
          fail(token.pos, msg.str());
        }
        // Substitute parameters into the body, then rescan for nested macros.
        vector<Token> substituted;
        for (const Token& bodyToken : macro.body) {
          bool paramP = false;
          if (TokenKind::Identifier == bodyToken.kind) {
            for (size_t p = 0; p < macro.params.size(); ++p) {
              if (macro.params[p] == bodyToken.key) {
                substituted.insert(substituted.end(), args[p].begin(),
                                   args[p].end());
                paramP = true;
                break;
              }
            }
          }
          if (!paramP) {
            substituted.push_back(bodyToken);
          }
        }
        expandInto(substituted, macros, depth + 1, out);
        i = j + 1;   // past the closing ')'
      }
      return;
    }

    LexResult
    lexCommon(const string& text, const string& file, const string& includeDir)
    {
      Prescan state;
      vector<Token> raw;
      prescanText(text, file, includeDir, state, raw);

      LexResult result;
      result.macroNames = state.macroNames;
      expandInto(raw, state.macros, 0, result.tokens);

      Token end;
      end.kind = TokenKind::EndOfInput;
      end.pos = SourcePos{file, 0, 0};
      result.tokens.push_back(end);
      return result;
    }

  } // namespace

  LexResult
  lexGmsFile(const string& path)
  {
    return lexCommon(readFileText(path), path, directoryOf(path));
  }

  LexResult
  lexGmsString(const string& text, const string& pseudoFile,
               const string& includeDir)
  {
    return lexCommon(text, pseudoFile, includeDir);
  }

} // namespace VINCP::Gms
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
