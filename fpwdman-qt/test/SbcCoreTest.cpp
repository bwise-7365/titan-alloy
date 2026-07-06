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
// Copyright Ben Paul Wise. All Rights Reserved.
