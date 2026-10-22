#include "doctest.h"
#include "modcurses/utf8.hpp"

using namespace modcurses;

TEST_CASE("round-trips across all four encoded lengths") {
    const std::u32string src = U"aé中\U0001F600";  // 1, 2, 3 and 4 bytes
    const std::string encoded = utf8_encode(src);
    CHECK(encoded.size() == 1 + 2 + 3 + 4);
    CHECK(utf8_decode(encoded) == src);
}

TEST_CASE("box-drawing literals survive the compiler") {
    // This is the /utf-8 canary: without it MSVC parses these in the system
    // codepage and silently corrupts them (C4066).
    CHECK(utf8_decode("\xe2\x94\x8c") == std::u32string{U'┌'});
    const std::u32string box = U"┌─┐";
    CHECK(utf8_decode(utf8_encode(box)) == box);
}

TEST_CASE("malformed input yields U+FFFD and resynchronises") {
    SUBCASE("stray continuation byte") {
        const auto out = utf8_decode("a\x80z");
        REQUIRE(out.size() == 3);
        CHECK(out[0] == U'a');
        CHECK(out[1] == kReplacementChar);
        CHECK(out[2] == U'z');
    }
    SUBCASE("truncated sequence does not swallow the next lead byte") {
        const auto out = utf8_decode("\xe4\xb8" "A");
        REQUIRE(out.size() == 2);
        CHECK(out[0] == kReplacementChar);
        CHECK(out[1] == U'A');
    }
    SUBCASE("overlong encoding of '/' is rejected") {
        CHECK(utf8_decode("\xc0\xaf") == std::u32string{kReplacementChar});
    }
    SUBCASE("surrogate half is rejected") {
        CHECK(utf8_decode("\xed\xa0\x80") == std::u32string{kReplacementChar});
    }
    SUBCASE("beyond U+10FFFF is rejected") {
        CHECK(utf8_decode("\xf7\xbf\xbf\xbf") == std::u32string{kReplacementChar});
    }
}

TEST_CASE("encoding an invalid scalar substitutes rather than emitting garbage") {
    CHECK(utf8_encode(static_cast<char32_t>(0x110000)) == utf8_encode(kReplacementChar));
    CHECK(utf8_encode(static_cast<char32_t>(0xD800)) == utf8_encode(kReplacementChar));
}

TEST_CASE("col_width is 1 in v1 - CJK is deferred to M6") {
    CHECK(col_width(U'a') == 1);
    CHECK(col_width(U'中') == 1);
}
