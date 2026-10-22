// Copyright Ben Paul Wise. All Rights Reserved.
#include <gtest/gtest.h>

#include "sbc_core.h"

#include <cstdint>
#include <vector>

using namespace SBC;

// qTransform(x) = (x+1)*(2x+1) mod 2^32 -- known values.
TEST(SbcCore, QTransformKnownAnswers) {
    EXPECT_EQ(qTransform(0u), 1u);          // 1 * 1
    EXPECT_EQ(qTransform(1u), 6u);          // 2 * 3
    EXPECT_EQ(qTransform(2u), 15u);         // 3 * 5
    EXPECT_EQ(qTransform(0xFFFFFFFFu), 0u); // (0) * (0xFFFFFFFF)
}

TEST(SbcCore, Rotations) {
    EXPECT_EQ(rotl32(1u, 1), 2u);
    EXPECT_EQ(rotl32(0x80000000u, 1), 1u);
    EXPECT_EQ(rotr32(1u, 1), 0x80000000u);
    EXPECT_EQ(rotl32(0x12345678u, 0), 0x12345678u);
    EXPECT_EQ(rotr32(rotl32(0xABCDEF01u, 13), 13), 0xABCDEF01u);
}

TEST(SbcCore, FitOctetIsDeterministic) {
    const std::string pass = "qwerty";
    std::vector<uint8_t> a(64), b(64);
    fitOctet(reinterpret_cast<const uint8_t*>(pass.data()), pass.size(), a.data(), a.size());
    fitOctet(reinterpret_cast<const uint8_t*>(pass.data()), pass.size(), b.data(), b.size());
    EXPECT_EQ(a, b);
    // The expansion should not be trivially all-zero.
    bool anyNonZero = false;
    for (uint8_t v : a)
        anyNonZero = anyNonZero || (v != 0);
    EXPECT_TRUE(anyNonZero);
}

TEST(SbcCore, OctifyDeoctifyRoundTrip) {
    std::vector<Word> words = {0x00000000u, 0xFFFFFFFFu, 0x12345678u, 0xABCDEF01u};
    auto bytes = octify(words);
    ASSERT_EQ(bytes.size(), words.size() * 4);
    // little-endian: word 0x12345678 -> bytes 78 56 34 12
    EXPECT_EQ(bytes[0], 0x00u);
    EXPECT_EQ(bytes[4], 0xFFu);
    EXPECT_EQ(bytes[8], 0x78u);
    EXPECT_EQ(bytes[11], 0x12u);
    EXPECT_EQ(bytes[15], 0xABu); // MSB of 0xABCDEF01
    EXPECT_EQ(deoctify(bytes), words);
}

// CBC encrypt/decrypt must be an exact inverse for the version-0 cipher.
TEST(SbcCore, CipherCbcRoundTrip) {
    SBCipher cipher(32, 16);
    std::vector<uint8_t> keyBytes(64);
    for (size_t i = 0; i < keyBytes.size(); ++i)
        keyBytes[i] = static_cast<uint8_t>(7 * i + 3);
    cipher.keySetupRaw(keyBytes.data(), keyBytes.size());

    // Three blocks of pseudo-random plaintext (deterministic LCG).
    std::vector<Word> plain(32 * 3);
    uint32_t s = 0xC0FFEEu;
    for (auto& w : plain) {
        s = s * 1664525u + 1013904223u;
        w = s;
    }

    const auto cipherText = cipher.encipherCBC(plain);
    const auto recovered = cipher.decipherCBC(cipherText);
    EXPECT_EQ(recovered, plain);
    EXPECT_NE(cipherText, plain); // it actually enciphered something
}

TEST(SbcCore, KeySetupRawRejectsWrongLength) {
    SBCipher cipher(32, 16);
    std::vector<uint8_t> tooShort(10);
    EXPECT_THROW(cipher.keySetupRaw(tooShort.data(), tooShort.size()), BadKeySize);
}

// --- known-answer vectors -----------------------------------------------------
// Key and two blocks of plaintext from a fixed LCG, so the inputs need no table.

namespace {

struct Lcg {
    uint32_t s = 12345u;
    uint32_t next() { return s = s * 1664525u + 1013904223u; }
};

void katInputs(std::vector<uint8_t>& key, std::vector<Word>& plain) {
    Lcg g;
    key.assign(64, 0);
    for (auto& b : key)
        b = static_cast<uint8_t>(g.next() >> 24);
    plain.assign(64, 0);
    for (auto& w : plain)
        w = g.next();
}

std::vector<Word> katCipher(KeySchedule schedule) {
    std::vector<uint8_t> key;
    std::vector<Word> plain;
    katInputs(key, plain);
    SBCipher cipher(32, 16, 24, schedule);
    cipher.keySetupRaw(key.data(), key.size());
    const auto c = cipher.encipherCBC(plain);
    EXPECT_EQ(cipher.decipherCBC(c), plain);
    return c;
}

} // namespace

// Pins version 0 exactly as it was before version 1 existed (captured from that
// build). Legacy and FPMQ1 files are readable only while this holds.
TEST(SbcCore, Version0KnownAnswer) {
    const std::vector<Word> expected = {
        0x9ba4a51cu, 0x3a3f5e94u, 0x9b528a11u, 0x380164fau, 0x3c66c609u, 0x3748f65fu, 0x1e725ea4u, 0xe90a18b5u,
        0x6ee104dfu, 0xde1bc5e7u, 0xbbb760c6u, 0xa3e7424bu, 0x64121277u, 0xc9ee578fu, 0x5ff86156u, 0xbab5b6ddu,
        0xadce15e1u, 0x5cdfdeb1u, 0x27196dd4u, 0x79821e06u, 0xc9c13335u, 0x97e24d04u, 0x60e01955u, 0x4411c1f7u,
        0x5d2686adu, 0x075f7bfcu, 0xe0a1b94eu, 0xa6bda835u, 0x6063932cu, 0x1e047a6eu, 0x6abbce68u, 0x9cafb8fbu,
        0x5052a9a8u, 0x921ca048u, 0x6c19242bu, 0x2ea7de2du, 0xcbaf3d71u, 0xafec956du, 0x8687d8eeu, 0x3d5d2024u,
        0x9b516328u, 0xce3a850cu, 0xfd458861u, 0x65e62250u, 0xeda39403u, 0xffcd10a1u, 0xf8601c39u, 0xf1a7de85u,
        0x022c2da3u, 0xc50a8eefu, 0x6c42ed36u, 0x1dddab4au, 0x5e20f449u, 0x70d11b7du, 0x8412d8ebu, 0x21bd6580u,
        0x449bdf75u, 0xdda5e6edu, 0x54e99180u, 0x8bf4615bu, 0x1b9548d1u, 0xd7de3103u, 0x7e3877d7u, 0x20cac468u,
    };
    EXPECT_EQ(katCipher(KeySchedule::V0), expected);
}

// Version 1 on the same inputs. The expected words come from an independent
// Python reference (HKDF from Python's hmac/hashlib, and a cipher port that
// reproduces Version0KnownAnswer), not from this code's own output.
TEST(SbcCore, Version1KnownAnswer) {
    const std::vector<Word> expected = {
        0x89012e53u, 0x73bd03cdu, 0x92db957cu, 0xb81c33a1u, 0x0a69e1bau, 0x67f5a844u, 0xa01ee160u, 0x106ff85bu,
        0x7373ff91u, 0xd1efa634u, 0x4f2dd38cu, 0x4d566e1cu, 0x9df8a278u, 0x1935915du, 0x2f69fc76u, 0xfdac31fbu,
        0xb44c9455u, 0x4d845018u, 0x61ad7db2u, 0x9efce18bu, 0xc409be6du, 0xfef7f156u, 0xe7b58c78u, 0x2946ab9fu,
        0x60666c56u, 0x198ffe4cu, 0xa4a1543du, 0x4e8f4d8fu, 0xfcbc237fu, 0x9e51e877u, 0xe6479b4cu, 0xf0cec448u,
        0xa40e6678u, 0x1aad809cu, 0x5c82b6b2u, 0xad750553u, 0x6cc8bbebu, 0xd552b474u, 0x379eafe5u, 0x8d191ca4u,
        0xe9ebb3f8u, 0x75d78d51u, 0xab07a663u, 0x745c4fa6u, 0x07c0db00u, 0x5a7e00a6u, 0xe25a101bu, 0xd73a44e4u,
        0x63d92355u, 0x4311e4dcu, 0xcfbbdef4u, 0xf49b6314u, 0x48103a53u, 0x1d39abd7u, 0x9fd1a509u, 0x4469fe6au,
        0x24526367u, 0xf8a5f74du, 0xce159ebdu, 0xe0bd5c2bu, 0x5d1695c3u, 0xc50327e4u, 0x5041a2afu, 0xc1d38aaeu,
    };
    EXPECT_EQ(katCipher(KeySchedule::V1), expected);
}

// --- the version-1 key schedule ---------------------------------------------

namespace {

std::vector<uint8_t> sampleKey(size_t n) {
    std::vector<uint8_t> k(n);
    for (size_t i = 0; i < n; ++i)
        k[i] = static_cast<uint8_t>(31 * i + 5);
    return k;
}

// The subkeys of round r (32 steps) as a vector, for comparing rounds.
std::vector<Word> roundSubkeys(const SBCipher& c, uint32_t r) {
    const auto& s = c.subkeys();
    return std::vector<Word>(s.begin() + 32 * r, s.begin() + 32 * (r + 1));
}

} // namespace

// The weakness version 1 removes: under version 0 every round has the same
// subkeys; under version 1 no two rounds do.
TEST(SbcCore, Version1RoundsAreAllDifferent) {
    const auto key = sampleKey(64);
    SBCipher v0(32, 16, 24, KeySchedule::V0);
    SBCipher v1(32, 16, 24, KeySchedule::V1);
    v0.keySetupRaw(key.data(), key.size());
    v1.keySetupRaw(key.data(), key.size());
    ASSERT_EQ(v1.subkeys().size(), 24u * 32u);

    EXPECT_EQ(roundSubkeys(v0, 0), roundSubkeys(v0, 23));
    for (uint32_t a = 0; a < 24; ++a)
        for (uint32_t b = a + 1; b < 24; ++b)
            EXPECT_NE(roundSubkeys(v1, a), roundSubkeys(v1, b)) << a << " vs " << b;
}

// The block size and round count are bound into the schedule, so one shape's
// subkeys are not simply the start of another's.
TEST(SbcCore, Version1ScheduleDependsOnShape) {
    const auto key = sampleKey(64);
    SBCipher r24(32, 16, 24, KeySchedule::V1);
    SBCipher r25(32, 16, 25, KeySchedule::V1);
    r24.keySetupRaw(key.data(), key.size());
    r25.keySetupRaw(key.data(), key.size());
    EXPECT_NE(roundSubkeys(r24, 0), roundSubkeys(r25, 0));
}

TEST(SbcCore, Version1AcceptsAnyKeyLength) {
    for (size_t n : {1u, 16u, 64u, 200u}) {
        SBCipher c(32, 16, 24, KeySchedule::V1);
        const auto key = sampleKey(n);
        c.keySetupRaw(key.data(), key.size());
        std::vector<Word> plain(32, 0x01234567u);
        EXPECT_EQ(c.decipherCBC(c.encipherCBC(plain)), plain) << n;
    }
    SBCipher c(32, 16, 24, KeySchedule::V1);
    EXPECT_THROW(c.keySetupRaw(nullptr, 0), BadKeySize);
}

// HKDF can produce at most 2040 subkeys; a larger shape is refused up front.
TEST(SbcCore, Version1RefusesTooManySteps) {
    EXPECT_NO_THROW(SBCipher(85, 16, 24, KeySchedule::V1));  // 2040 steps
    EXPECT_THROW(SBCipher(86, 16, 24, KeySchedule::V1), BadBlockSize); // 2064
    EXPECT_NO_THROW(SBCipher(86, 16, 24, KeySchedule::V0));  // version 0 has no limit
}
// Copyright Ben Paul Wise. All Rights Reserved.
