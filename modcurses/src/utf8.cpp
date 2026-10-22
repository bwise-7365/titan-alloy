#include "modcurses/utf8.hpp"

#include <cstdint>

namespace modcurses {
namespace {

constexpr bool is_continuation(unsigned char b) { return (b & 0xC0u) == 0x80u; }

// Rejects the encodings that are structurally valid but not legal UTF-8:
// overlong forms, surrogates, and anything past U+10FFFF.
constexpr bool is_valid_scalar(char32_t cp, int len) {
    if (cp > 0x10FFFFu) return false;
    if (cp >= 0xD800u && cp <= 0xDFFFu) return false;  // lone surrogate
    if (len == 2 && cp < 0x80u) return false;
    if (len == 3 && cp < 0x800u) return false;
    if (len == 4 && cp < 0x10000u) return false;
    return true;
}

}  // namespace

char32_t utf8_next(std::string_view in, std::size_t& pos) {
    if (pos >= in.size()) return 0;

    const auto b0 = static_cast<unsigned char>(in[pos]);
    int len = 0;
    char32_t cp = 0;

    if (b0 < 0x80u) {
        ++pos;
        return b0;
    } else if ((b0 & 0xE0u) == 0xC0u) {
        len = 2;
        cp = b0 & 0x1Fu;
    } else if ((b0 & 0xF0u) == 0xE0u) {
        len = 3;
        cp = b0 & 0x0Fu;
    } else if ((b0 & 0xF8u) == 0xF0u) {
        len = 4;
        cp = b0 & 0x07u;
    } else {
        ++pos;  // stray continuation byte or 0xFE/0xFF
        return kReplacementChar;
    }

    // Truncated sequence at end of input, or a missing continuation byte:
    // consume only what we validated so the next call resynchronises on the
    // following lead byte rather than swallowing it.
    for (int i = 1; i < len; ++i) {
        if (pos + static_cast<std::size_t>(i) >= in.size() ||
            !is_continuation(static_cast<unsigned char>(in[pos + static_cast<std::size_t>(i)]))) {
            pos += static_cast<std::size_t>(i);
            return kReplacementChar;
        }
        cp = (cp << 6) | (static_cast<unsigned char>(in[pos + static_cast<std::size_t>(i)]) & 0x3Fu);
    }

    pos += static_cast<std::size_t>(len);
    return is_valid_scalar(cp, len) ? cp : kReplacementChar;
}

std::u32string utf8_decode(std::string_view in) {
    std::u32string out;
    out.reserve(in.size());
    std::size_t pos = 0;
    while (pos < in.size()) out.push_back(utf8_next(in, pos));
    return out;
}

std::string utf8_encode(char32_t cp) {
    std::string out;
    if (cp > 0x10FFFFu || (cp >= 0xD800u && cp <= 0xDFFFu)) cp = kReplacementChar;

    if (cp < 0x80u) {
        out.push_back(static_cast<char>(cp));
    } else if (cp < 0x800u) {
        out.push_back(static_cast<char>(0xC0u | (cp >> 6)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    } else if (cp < 0x10000u) {
        out.push_back(static_cast<char>(0xE0u | (cp >> 12)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    } else {
        out.push_back(static_cast<char>(0xF0u | (cp >> 18)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 12) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | ((cp >> 6) & 0x3Fu)));
        out.push_back(static_cast<char>(0x80u | (cp & 0x3Fu)));
    }
    return out;
}

std::string utf8_encode(std::u32string_view in) {
    std::string out;
    out.reserve(in.size());
    for (char32_t cp : in) out += utf8_encode(cp);
    return out;
}

}  // namespace modcurses
