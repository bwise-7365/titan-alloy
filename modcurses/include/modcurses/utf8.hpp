#pragma once
//
// modcurses/utf8.hpp - UTF-8 <-> char32_t conversion.
//
// Glyph data is char32_t everywhere in the library; UTF-8 exists only at the
// edges (source literals, file I/O, the narrow overload of Canvas::print).
// Decoding is permissive: malformed input yields U+FFFD and resynchronises,
// so a bad byte in a loaded file cannot desynchronise an entire buffer.
//
#include <cstddef>
#include <string>
#include <string_view>

namespace modcurses {

inline constexpr char32_t kReplacementChar = U'�';

[[nodiscard]] std::u32string utf8_decode(std::string_view in);
[[nodiscard]] std::string utf8_encode(std::u32string_view in);
[[nodiscard]] std::string utf8_encode(char32_t cp);

// Decode one codepoint starting at `pos`; advances `pos` past it. Returns
// kReplacementChar on malformed input (advancing at least one byte).
[[nodiscard]] char32_t utf8_next(std::string_view in, std::size_t& pos);

// Column width of a codepoint. v1 is BMP-narrow-only and always answers 1 for
// printable characters; double-width (CJK) is deferred to M6. Column
// arithmetic in widgets must route through here so that change stays local.
[[nodiscard]] constexpr int col_width(char32_t) { return 1; }

}  // namespace modcurses
