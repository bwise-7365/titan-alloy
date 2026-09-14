// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
// [PROPOSED contract, M10] Named symbols as primitives in a unit frame, the C++ counterpart of
// hexsheet2svg.py's SYMBOLS (map glyphs: hex circumradius 1, y down, side glyphs with the hexside on
// the x axis) and counters2svg.py's icons, echelon marks and emblems (face units: a 100-unit square).
// One library serves every front end, so a port anchor or an infantry box is drawn identically.
// ----------------------------------------------
#pragma once
#include "hexview/Scene.h"

#include <map>
#include <string>
#include <vector>

namespace HexView {

  enum class SymbolFrame : std::uint8_t { Hex, Face };

  struct Symbol {
    SymbolFrame frame;
    std::vector<Primitive> body;  // currentColor parts carry the placeholder colour kCurrentColor
  };

  inline constexpr Color kCurrentColor{1, 2, 3, 0};  // replaced by SymbolUse::color when drawn

  class SymbolLibrary {
  public:
    // Every symbol the two reference renderers define, transcribed.
    static SymbolLibrary reference();

    bool containsP(const std::string& name) const;
    // Throws std::invalid_argument naming the symbol when the library has none by that name.
    const Symbol& symbol(const std::string& name) const;
    std::vector<std::string> names() const;  // sorted

  private:
    std::map<std::string, Symbol> symbols_;
  };

}  // namespace HexView
// ----------------------------------------------
// Copyright Ben Paul Wise. All Rights Reserved.
// ----------------------------------------------
