// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// GmsParser implementation: recursive descent over the censused GAMS subset.
// Newlines are significant only in list contexts (declaration blocks, data
// blocks, tables, model pair lists, display lists); expressions and other
// statements skip them freely.
// ----------------------------------------------
// "GAMS" is a registered trademark of GAMS Development Corporation. This
// code is not endorsed or certified by GAMS Development Corporation. The
// subset of the GMS parsed by this code is incompatible with most of the
// GAMS modeling language. The software is provided without warranty of any
// kind, express or implied, including without limitation for any particular
// purpose. The provider makes no guarantees about its performance, accuracy,
// or suitability for any specific application.
// ----------------------------------------------
#include "gmsparser.hpp"

#include "gmslexer.hpp"

#include <algorithm>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace VINCP::Gms {

  namespace {

    [[noreturn]] void
    fail(const Token& token, const string& message)
    {
      throw std::invalid_argument(describePos(token.pos) + ": " + message
                                  + " (got '" + token.text + "')");
    }

    const std::set<string> kStatementKeywords = {
        "set",      "sets",      "alias",    "scalar",   "scalars",
        "parameter","parameters","table",    "tables",   "variable",
        "variables","positive",  "equation", "equations","model",
        "models",   "option",    "options",  "solve",    "display"};

    const std::set<string> kFunctions = {"exp",  "log", "sqrt",  "sqr",
                                         "max",  "min", "round", "card",
                                         "power","abs"};

    class Parser {
    public:
      Parser(const vector<Token>& tokens, const vector<string>& macroNames)
        : tokens_(tokens)
      {
        program_.macroNames = macroNames;
      }

      Program
      run()
      {
        for (;;) {
          skipNl();
          if (atP(TokenKind::EndOfInput)) {
            break;
          }
          parseStatement();
        }
        return program_;
      }

    protected:
    private:
      const vector<Token>& tokens_;
      size_t i_ = 0;
      Program program_;

      // --- token access -----------------------------------------------------

      const Token&
      cur() const
      {
        return tokens_[std::min(i_, tokens_.size() - 1)];
      }

      bool
      atP(TokenKind kind) const
      {
        return kind == cur().kind;
      }

      void
      advance()
      {
        if (i_ + 1 < tokens_.size()) {
          ++i_;
        }
        return;
      }

      void
      skipNl()
      {
        while (atP(TokenKind::Newline)) {
          advance();
        }
        return;
      }

      // Non-consuming look past any newlines.
      const Token&
      peekSkip() const
      {
        size_t j = i_;
        while (j < tokens_.size() - 1 && TokenKind::Newline == tokens_[j].kind) {
          ++j;
        }
        return tokens_[j];
      }

      Token
      expect(TokenKind kind, const string& what)
      {
        if (!atP(kind)) {
          fail(cur(), "expected " + what);
        }
        Token token = cur();
        advance();
        return token;
      }

      Token
      expectSkip(TokenKind kind, const string& what)
      {
        skipNl();
        return expect(kind, what);
      }

      // Identifier or Number (or quoted text), used as a set-element /
      // data-key / table label; returns the original spelling.
      string
      labelText()
      {
        if (atP(TokenKind::Identifier) || atP(TokenKind::Number)
            || atP(TokenKind::QuotedText)) {
          const string text = cur().text;
          advance();
          return text;
        }
        fail(cur(), "expected a label");
      }

      // --- shared pieces ----------------------------------------------------

      // Optional immediately-following parenthesized identifier list.
      vector<string>
      parseOptionalDomain()
      {
        vector<string> domain;
        if (!atP(TokenKind::LParen)) {
          return domain;
        }
        advance();
        for (;;) {
          skipNl();
          domain.push_back(
              expect(TokenKind::Identifier, "a domain index name").text);
          skipNl();
          if (atP(TokenKind::Comma)) {
            advance();
            continue;
          }
          expect(TokenKind::RParen, "')' after the domain list");
          break;
        }
        return domain;
      }

      // Optional description: a quoted string, or bare words up to the end of
      // the line / a data block / a separator. Raw mode (newline-aware).
      string
      parseDescription()
      {
        if (atP(TokenKind::QuotedText)) {
          const string text = cur().text;
          advance();
          return text;
        }
        string text;
        while (!atP(TokenKind::Newline) && !atP(TokenKind::Slash)
               && !atP(TokenKind::Semicolon) && !atP(TokenKind::Comma)
               && !atP(TokenKind::EndOfInput)) {
          if (!text.empty()) {
            text += ' ';
          }
          text += cur().text;
          advance();
        }
        return text;
      }

      // Signed numeric literal (data and table cells may be negative).
      double
      parseSignedNumber()
      {
        bool negativeP = false;
        if (atP(TokenKind::Minus)) {
          negativeP = true;
          advance();
        }
        const Token number = expect(TokenKind::Number, "a numeric value");
        return negativeP ? -number.value : number.value;
      }

      // / entry (,|newline) entry ... / for parameters; keys... [=] value.
      vector<DataEntry>
      parseParameterData()
      {
        expectSkip(TokenKind::Slash, "'/' opening a data block");
        vector<DataEntry> entries;
        for (;;) {
          skipNl();
          if (atP(TokenKind::Comma)) {
            advance();
            continue;
          }
          if (atP(TokenKind::Slash)) {
            advance();
            break;
          }
          DataEntry entry;
          entry.keys.push_back(labelText());
          while (atP(TokenKind::Dot)) {
            advance();
            skipNl();
            entry.keys.push_back(labelText());
          }
          if (atP(TokenKind::Assign)) {
            advance();
          }
          skipNl();
          entry.value = parseSignedNumber();
          entries.push_back(entry);
        }
        return entries;
      }

      // --- statements -------------------------------------------------------

      void
      parseStatement()
      {
        if (!atP(TokenKind::Identifier)) {
          fail(cur(), "expected a statement");
        }
        const string& key = cur().key;
        if ("set" == key || "sets" == key) {
          parseSetDecl();
        }
        else
        if ("alias" == key) {
          parseAliasDecl();
        }
        else
        if ("scalar" == key || "scalars" == key) {
          parseScalarDecl();
        }
        else
        if ("parameter" == key || "parameters" == key) {
          parseParameterDecl();
        }
        else
        if ("table" == key || "tables" == key) {
          parseTableDecl();
        }
        else
        if ("positive" == key) {
          advance();
          skipNl();
          const Token kw = expect(TokenKind::Identifier, "'Variable(s)'");
          if ("variable" != kw.key && "variables" != kw.key) {
            fail(kw, "expected 'Variable(s)' after 'Positive'");
          }
          parseVariableItems(true);
        }
        else
        if ("variable" == key || "variables" == key) {
          advance();
          parseVariableItems(false);
        }
        else
        if ("equation" == key || "equations" == key) {
          parseEquationDecl();
        }
        else
        if ("model" == key || "models" == key) {
          parseModelDecl();
        }
        else
        if ("option" == key || "options" == key) {
          parseOptionStmt();
        }
        else
        if ("solve" == key) {
          parseSolveStmt();
        }
        else
        if ("display" == key) {
          parseDisplayStmt();
        }
        else {
          parseAssignmentOrEquationDef();
        }
        return;
      }

      void
      parseSetDecl()
      {
        advance();   // Set / Sets
        SetDecl decl;
        skipNl();
        const Token name = expect(TokenKind::Identifier, "a set name");
        decl.item.name = name.text;
        decl.item.key = name.key;
        decl.item.domain = parseOptionalDomain();
        decl.item.description = parseDescription();
        expectSkip(TokenKind::Slash, "'/' opening the set's element list");
        for (;;) {
          skipNl();
          if (atP(TokenKind::Comma)) {
            advance();
            continue;
          }
          if (atP(TokenKind::Slash)) {
            advance();
            break;
          }
          decl.elements.push_back(labelText());
        }
        expectSkip(TokenKind::Semicolon, "';' ending the Set statement");
        program_.statements.push_back(decl);
        return;
      }

      void
      parseAliasDecl()
      {
        advance();   // alias
        AliasDecl decl;
        expectSkip(TokenKind::LParen, "'(' after 'alias'");
        for (;;) {
          skipNl();
          const Token name = expect(TokenKind::Identifier, "an alias name");
          decl.names.push_back(name.text);
          decl.keys.push_back(name.key);
          skipNl();
          if (atP(TokenKind::Comma)) {
            advance();
            continue;
          }
          expect(TokenKind::RParen, "')' ending the alias list");
          break;
        }
        expectSkip(TokenKind::Semicolon, "';' ending the alias statement");
        program_.statements.push_back(decl);
        return;
      }

      // Shared header for one declared item: name, optional domain,
      // optional description.
      DeclItem
      parseDeclItemHeader(const string& what)
      {
        DeclItem item;
        const Token name = expect(TokenKind::Identifier, "a " + what + " name");
        item.name = name.text;
        item.key = name.key;
        item.domain = parseOptionalDomain();
        item.description = parseDescription();
        return item;
      }

      void
      parseScalarDecl()
      {
        advance();   // Scalar / Scalars
        ScalarDecl decl;
        for (;;) {
          skipNl();
          if (atP(TokenKind::Semicolon)) {
            advance();
            break;
          }
          if (atP(TokenKind::Comma)) {
            advance();
            continue;
          }
          ScalarDecl::Item item;
          item.decl = parseDeclItemHeader("scalar");
          if (TokenKind::Slash == peekSkip().kind) {
            expectSkip(TokenKind::Slash, "'/'");
            skipNl();
            item.value = parseSignedNumber();
            item.hasValueP = true;
            expectSkip(TokenKind::Slash, "'/' closing the scalar's value");
          }
          decl.items.push_back(item);
        }
        program_.statements.push_back(decl);
        return;
      }

      void
      parseParameterDecl()
      {
        advance();   // Parameter / Parameters
        ParameterDecl decl;
        for (;;) {
          skipNl();
          if (atP(TokenKind::Semicolon)) {
            advance();
            break;
          }
          if (atP(TokenKind::Comma)) {
            advance();
            continue;
          }
          ParameterDecl::Item item;
          item.decl = parseDeclItemHeader("parameter");
          if (TokenKind::Slash == peekSkip().kind) {
            item.data = parseParameterData();
            item.hasDataP = true;
          }
          decl.items.push_back(item);
        }
        program_.statements.push_back(decl);
        return;
      }

      void
      parseTableDecl()
      {
        advance();   // Table
        TableDecl decl;
        skipNl();
        decl.decl = parseDeclItemHeader("table");
        if (decl.decl.domain.empty()) {
          fail(cur(), "a Table needs a parenthesized domain");
        }
        expect(TokenKind::Newline, "end of line after the Table header");
        // Header line: column labels.
        skipNl();
        while (!atP(TokenKind::Newline) && !atP(TokenKind::EndOfInput)) {
          decl.columns.push_back(labelText());
        }
        if (decl.columns.empty()) {
          fail(cur(), "a Table needs a column-label line");
        }
        // Rows until the terminating ';'.
        for (;;) {
          skipNl();
          if (atP(TokenKind::Semicolon)) {
            advance();
            break;
          }
          TableDecl::Row row;
          row.label = labelText();
          while (!atP(TokenKind::Newline) && !atP(TokenKind::Semicolon)
                 && !atP(TokenKind::EndOfInput)) {
            row.values.push_back(parseSignedNumber());
          }
          if (row.values.size() != decl.columns.size()) {
            std::ostringstream msg;
            msg << "table row '" << row.label << "' has " << row.values.size()
                << " values for " << decl.columns.size()
                << " columns (sparse or continued tables are outside the "
                   "supported subset)";
            fail(cur(), msg.str());
          }
          decl.rows.push_back(row);
        }
        program_.statements.push_back(decl);
        return;
      }

      // Items of a Variable / Positive Variable / Equations block (no data
      // blocks allowed).
      vector<DeclItem>
      parseNoDataItems(const string& what)
      {
        vector<DeclItem> items;
        for (;;) {
          skipNl();
          if (atP(TokenKind::Semicolon)) {
            advance();
            break;
          }
          if (atP(TokenKind::Comma)) {
            advance();
            continue;
          }
          items.push_back(parseDeclItemHeader(what));
          if (TokenKind::Slash == peekSkip().kind) {
            fail(cur(), "a data block is not supported on a " + what);
          }
        }
        return items;
      }

      void
      parseVariableItems(bool positiveP)
      {
        VariableDecl decl;
        decl.positiveP = positiveP;
        decl.items = parseNoDataItems("variable");
        program_.statements.push_back(decl);
        return;
      }

      void
      parseEquationDecl()
      {
        advance();   // Equation / Equations
        EquationDecl decl;
        decl.items = parseNoDataItems("equation");
        program_.statements.push_back(decl);
        return;
      }

      void
      parseModelDecl()
      {
        advance();   // Model
        ModelDecl decl;
        skipNl();
        const Token name = expect(TokenKind::Identifier, "a model name");
        decl.name = name.text;
        decl.key = name.key;
        expectSkip(TokenKind::Slash, "'/' opening the model's pair list");
        for (;;) {
          skipNl();
          if (atP(TokenKind::Comma)) {
            advance();
            continue;
          }
          if (atP(TokenKind::Slash)) {
            advance();
            break;
          }
          ModelDecl::Pair pair;
          const Token eq = expect(TokenKind::Identifier, "an equation name");
          pair.eqName = eq.text;
          pair.eqKey = eq.key;
          expect(TokenKind::Dot, "'.' between the equation and its variable");
          const Token var = expect(TokenKind::Identifier, "a variable name");
          pair.varName = var.text;
          pair.varKey = var.key;
          decl.pairs.push_back(pair);
        }
        expectSkip(TokenKind::Semicolon, "';' ending the Model statement");
        program_.statements.push_back(decl);
        return;
      }

      void
      parseOptionStmt()
      {
        advance();   // Option / options
        OptionStmt stmt;
        skipNl();
        const Token name = expect(TokenKind::Identifier, "an option name");
        stmt.name = name.text;
        stmt.key = name.key;
        expect(TokenKind::Assign, "'=' in the Option statement");
        if (atP(TokenKind::Identifier) || atP(TokenKind::Number)) {
          stmt.value = cur().text;
          advance();
        }
        else {
          fail(cur(), "expected the option's value");
        }
        // The corpus contains one Option statement without a ';' (terminated
        // by its end of line); accept both terminators.
        if (atP(TokenKind::Semicolon)) {
          advance();
        }
        else
        if (!atP(TokenKind::Newline) && !atP(TokenKind::EndOfInput)) {
          fail(cur(), "expected ';' or end of line after the Option value");
        }
        program_.statements.push_back(stmt);
        return;
      }

      void
      parseSolveStmt()
      {
        advance();   // Solve
        SolveStmt stmt;
        skipNl();
        const Token model = expect(TokenKind::Identifier, "a model name");
        stmt.modelName = model.text;
        stmt.modelKey = model.key;
        skipNl();
        const Token usingWord = expect(TokenKind::Identifier, "'using'");
        if ("using" != usingWord.key) {
          fail(usingWord, "expected 'using' in the Solve statement");
        }
        skipNl();
        stmt.method = expect(TokenKind::Identifier, "a solve method").text;
        expectSkip(TokenKind::Semicolon, "';' ending the Solve statement");
        program_.statements.push_back(stmt);
        return;
      }

      void
      parseDisplayStmt()
      {
        advance();   // Display
        DisplayStmt stmt;
        for (;;) {
          skipNl();
          if (atP(TokenKind::Comma)) {
            advance();
            continue;
          }
          if (atP(TokenKind::Semicolon)) {
            advance();
            break;
          }
          DisplayStmt::Item item;
          const Token name = expect(TokenKind::Identifier, "a display item");
          item.name = name.text;
          item.key = name.key;
          if (atP(TokenKind::Dot)) {
            advance();
            item.attr = expect(TokenKind::Identifier, "an attribute").text;
          }
          stmt.items.push_back(item);
        }
        program_.statements.push_back(stmt);
        return;
      }

      void
      parseAssignmentOrEquationDef()
      {
        const Token name = expect(TokenKind::Identifier, "a statement");
        string attr;
        string attrKey;
        if (atP(TokenKind::Dot)) {
          advance();
          const Token attrToken =
              expect(TokenKind::Identifier, "an attribute name");
          attr = attrToken.text;
          attrKey = attrToken.key;
        }
        vector<string> indices;
        if (atP(TokenKind::LParen)) {
          advance();
          for (;;) {
            skipNl();
            indices.push_back(labelText());
            skipNl();
            if (atP(TokenKind::Comma)) {
              advance();
              continue;
            }
            expect(TokenKind::RParen, "')' after the index list");
            break;
          }
        }
        skipNl();
        if (atP(TokenKind::DotDot)) {
          if (!attr.empty()) {
            fail(cur(), "an equation definition cannot carry an attribute");
          }
          advance();
          EquationDef def;
          def.name = name.text;
          def.key = name.key;
          def.domain = indices;
          def.lhs = parseExpr();
          skipNl();
          if (atP(TokenKind::RelEq) || atP(TokenKind::RelGe)
              || atP(TokenKind::RelLe)) {
            def.relation = cur().text;
            advance();
          }
          else {
            fail(cur(), "expected '=e=', '=g=', or '=l='");
          }
          def.rhs = parseExpr();
          expectSkip(TokenKind::Semicolon,
                     "';' ending the equation definition");
          program_.statements.push_back(def);
          return;
        }
        if (atP(TokenKind::Assign)) {
          advance();
          Assignment assign;
          assign.name = name.text;
          assign.key = name.key;
          assign.attr = attr;
          assign.attrKey = attrKey;
          assign.indices = indices;
          assign.value = parseExpr();
          expectSkip(TokenKind::Semicolon, "';' ending the assignment");
          program_.statements.push_back(assign);
          return;
        }
        fail(cur(), "expected '..' (equation definition) or '=' (assignment) "
                    "after the left-hand side");
      }

      // --- expressions (newline-insensitive) --------------------------------

      Expr
      parseExpr()
      {
        Expr left = parseTerm();
        for (;;) {
          skipNl();
          if (atP(TokenKind::Plus) || atP(TokenKind::Minus)) {
            Expr node;
            node.kind = Expr::Kind::Binary;
            node.op = cur().text;
            advance();
            node.args.push_back(left);
            node.args.push_back(parseTerm());
            left = node;
            continue;
          }
          return left;
        }
      }

      Expr
      parseTerm()
      {
        Expr left = parseFactor();
        for (;;) {
          skipNl();
          if (atP(TokenKind::Star) || atP(TokenKind::Slash)) {
            Expr node;
            node.kind = Expr::Kind::Binary;
            node.op = cur().text;
            advance();
            node.args.push_back(left);
            node.args.push_back(parseFactor());
            left = node;
            continue;
          }
          return left;
        }
      }

      // '**' is right-associative here and, like the corpus, is never used
      // with an unparenthesized unary operand, so the unary/power binding
      // question does not arise in practice.
      Expr
      parseFactor()
      {
        Expr left = parseUnary();
        skipNl();
        if (atP(TokenKind::Power)) {
          Expr node;
          node.kind = Expr::Kind::Binary;
          node.op = "**";
          advance();
          node.args.push_back(left);
          node.args.push_back(parseFactor());
          return node;
        }
        return left;
      }

      Expr
      parseUnary()
      {
        skipNl();
        if (atP(TokenKind::Minus) || atP(TokenKind::Plus)) {
          Expr node;
          node.kind = Expr::Kind::Unary;
          node.op = cur().text;
          advance();
          node.args.push_back(parseUnary());
          return node;
        }
        return parsePrimary();
      }

      Expr
      parsePrimary()
      {
        skipNl();
        if (atP(TokenKind::Number)) {
          Expr node;
          node.kind = Expr::Kind::Number;
          node.number = cur().value;
          advance();
          return node;
        }
        if (atP(TokenKind::LParen)) {
          advance();
          Expr inner = parseExpr();
          expectSkip(TokenKind::RParen, "')'");
          return inner;
        }
        if (!atP(TokenKind::Identifier)) {
          fail(cur(), "expected an expression");
        }
        const Token name = cur();
        advance();
        if ("sum" == name.key) {
          return parseSum(name);
        }
        if (0 < kFunctions.count(name.key)) {
          Expr node;
          node.kind = Expr::Kind::Call;
          node.name = name.text;
          node.key = name.key;
          expectSkip(TokenKind::LParen, "'(' after " + name.text);
          for (;;) {
            node.args.push_back(parseExpr());
            skipNl();
            if (atP(TokenKind::Comma)) {
              advance();
              continue;
            }
            expect(TokenKind::RParen, "')' closing the call");
            break;
          }
          return node;
        }
        // Symbol or attribute reference.
        Expr node;
        node.kind = Expr::Kind::SymbolRef;
        node.name = name.text;
        node.key = name.key;
        if (atP(TokenKind::Dot)) {
          advance();
          const Token attrToken =
              expect(TokenKind::Identifier, "an attribute name");
          node.kind = Expr::Kind::AttrRef;
          node.attr = attrToken.text;
          node.attrKey = attrToken.key;
        }
        if (atP(TokenKind::LParen)) {
          advance();
          for (;;) {
            skipNl();
            node.indices.push_back(labelText());
            skipNl();
            if (atP(TokenKind::Comma)) {
              advance();
              continue;
            }
            expect(TokenKind::RParen, "')' after the index list");
            break;
          }
        }
        return node;
      }

      Expr
      parseSum(const Token& name)
      {
        Expr node;
        node.kind = Expr::Kind::Sum;
        node.name = name.text;
        node.key = name.key;
        expectSkip(TokenKind::LParen, "'(' after sum");
        skipNl();
        if (atP(TokenKind::LParen)) {
          advance();
          for (;;) {
            skipNl();
            node.sumIndices.push_back(
                expect(TokenKind::Identifier, "a sum index").text);
            skipNl();
            if (atP(TokenKind::Comma)) {
              advance();
              continue;
            }
            expect(TokenKind::RParen, "')' after the sum's index tuple");
            break;
          }
        }
        else {
          node.sumIndices.push_back(
              expect(TokenKind::Identifier, "a sum index").text);
        }
        expectSkip(TokenKind::Comma, "',' between the sum index and its body");
        node.args.push_back(parseExpr());
        expectSkip(TokenKind::RParen, "')' closing the sum");
        return node;
      }
    };

  } // namespace

  Program
  parseGmsTokens(const vector<Token>& tokens, const vector<string>& macroNames)
  {
    if (tokens.empty() || TokenKind::EndOfInput != tokens.back().kind) {
      throw std::invalid_argument(
          "parseGmsTokens: token stream must end with EndOfInput");
    }
    Parser parser(tokens, macroNames);
    return parser.run();
  }

  Program
  parseGmsFile(const string& path)
  {
    const LexResult lexed = lexGmsFile(path);
    return parseGmsTokens(lexed.tokens, lexed.macroNames);
  }

  Program
  parseGmsString(const string& text, const string& pseudoFile)
  {
    const LexResult lexed = lexGmsString(text, pseudoFile);
    return parseGmsTokens(lexed.tokens, lexed.macroNames);
  }

} // namespace VINCP::Gms
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
