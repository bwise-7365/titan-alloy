// Copyright Ben Paul Wise. All Rights Reserved.
//
// Known-answer tests for the hand-rolled SHA-256 / HMAC-SHA-256 / PBKDF2 in the
// sbc library. Hand-written crypto earns trust only against published vectors,
// so every case here is quoted from a standard rather than captured from our own
// output -- a self-generated "expected" value would only prove the code agrees
// with itself. Sources: FIPS 180-4 (SHA-256), RFC 4231 (HMAC-SHA-256),
// RFC 7914 s11 and RFC 6070's structure (PBKDF2-HMAC-SHA-256), RFC 5869
// Appendix A (HKDF-SHA-256).
//
// These also pin the format: PasswordStore's PBKDF2 must keep producing exactly
// these bytes, or every existing FPMQ1 file becomes unreadable.

#include <gtest/gtest.h>

#include <cstring>
#include <string>
#include <vector>

#include "sbc_kdf.h"

namespace {

std::string toHex(const uint8_t* p, size_t n) {
    static const char* d = "0123456789abcdef";
    std::string s;
    s.reserve(n * 2);
    for (size_t i = 0; i < n; ++i) {
        s.push_back(d[p[i] >> 4]);
        s.push_back(d[p[i] & 0x0F]);
    }
    return s;
}

const uint8_t* bytes(const char* s) {
    return reinterpret_cast<const uint8_t*>(s);
}

std::string sha256Hex(const std::string& msg) {
    uint8_t out[32];
    SBC::Sha256 h;
    h.update(bytes(msg.c_str()), msg.size());
    h.finish(out);
    return toHex(out, sizeof out);
}

} // namespace

// --- SHA-256 (FIPS 180-4) ---------------------------------------------------

TEST(SbcSha256, EmptyString) {
    EXPECT_EQ(sha256Hex(""),
              "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855");
}

TEST(SbcSha256, Abc) {
    EXPECT_EQ(sha256Hex("abc"),
              "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad");
}

// The two-block case: exercises the buffering path across a 64-byte boundary.
TEST(SbcSha256, TwoBlockMessage) {
    EXPECT_EQ(sha256Hex("abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"),
              "248d6a61d20638b8e5c026930c3e6039a33ce45964ff2167f6ecedd419db06c1");
}

// Exactly one block of input, so padding must spill into a second block.
TEST(SbcSha256, SixtyFourBytes) {
    EXPECT_EQ(sha256Hex(std::string(64, 'a')),
              "ffe054fe7ae0cb6dc65c3af9b61d5209f439851db43d0ba5997337df154668eb");
}

// A million 'a' -- the classic FIPS long-message vector. Fed in uneven chunks to
// prove update() re-assembles blocks correctly regardless of caller framing.
TEST(SbcSha256, MillionA) {
    SBC::Sha256 h;
    const std::string chunk(1000, 'a');
    for (int i = 0; i < 1000; ++i)
        h.update(bytes(chunk.c_str()), chunk.size());
    uint8_t out[32];
    h.finish(out);
    EXPECT_EQ(toHex(out, sizeof out),
              "cdc76e5c9914fb9281a1c7e284d73e67f1809a48a497200e046d39ccc7112cd0");
}

TEST(SbcSha256, UpdateChunkingIsIrrelevant) {
    const std::string msg = "the quick brown fox jumps over the lazy dog, twice over, at length";
    SBC::Sha256 a;
    a.update(bytes(msg.c_str()), msg.size());
    uint8_t oneShot[32];
    a.finish(oneShot);

    SBC::Sha256 b;
    for (size_t i = 0; i < msg.size(); ++i) // one byte at a time
        b.update(bytes(msg.c_str()) + i, 1);
    uint8_t drip[32];
    b.finish(drip);

    EXPECT_EQ(toHex(oneShot, 32), toHex(drip, 32));
}

// --- HMAC-SHA-256 (RFC 4231) ------------------------------------------------

TEST(SbcHmacSha256, Rfc4231Case1) {
    const std::vector<uint8_t> key(20, 0x0b);
    uint8_t out[32];
    SBC::hmacSha256(key.data(), key.size(), bytes("Hi There"), 8, out);
    EXPECT_EQ(toHex(out, sizeof out),
              "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
}

TEST(SbcHmacSha256, Rfc4231Case2) {
    uint8_t out[32];
    SBC::hmacSha256(bytes("Jefe"), 4, bytes("what do ya want for nothing?"), 28, out);
    EXPECT_EQ(toHex(out, sizeof out),
              "5bdcc146bf60754e6a042426089575c75a003f089d2739839dec58b964ec3843");
}

// Key longer than the 64-byte block, so setKey() must hash it down first.
TEST(SbcHmacSha256, Rfc4231Case6_OverlongKey) {
    const std::vector<uint8_t> key(131, 0xaa);
    uint8_t out[32];
    SBC::hmacSha256(key.data(), key.size(),
                    bytes("Test Using Larger Than Block-Size Key - Hash Key First"), 54, out);
    EXPECT_EQ(toHex(out, sizeof out),
              "60e431591ee0b67f0d8a26aacbf5b77f8e0bc6213728c5140546040f0ee37f54");
}

// The streaming API and the one-shot wrapper must agree, since PBKDF2 uses the
// former and the MAC tag path uses the latter.
TEST(SbcHmacSha256, StreamingMatchesOneShot) {
    const std::vector<uint8_t> key(20, 0x0b);
    uint8_t oneShot[32];
    SBC::hmacSha256(key.data(), key.size(), bytes("Hi There"), 8, oneShot);

    SBC::HmacSha256 mac;
    mac.setKey(key.data(), key.size());
    mac.begin();
    mac.update(bytes("Hi "), 3);
    mac.update(bytes("There"), 5);
    uint8_t streamed[32];
    mac.finish(streamed);

    EXPECT_EQ(toHex(oneShot, 32), toHex(streamed, 32));
}

// A keyed object must be reusable across messages: begin() has to fully reset
// the working state from the stored inner midstate, not carry residue over.
TEST(SbcHmacSha256, KeyIsReusableAcrossMessages) {
    const std::vector<uint8_t> key(20, 0x0b);
    SBC::HmacSha256 mac;
    mac.setKey(key.data(), key.size());

    uint8_t first[32];
    mac.begin();
    mac.update(bytes("Hi There"), 8);
    mac.finish(first);

    uint8_t second[32]; // same message again, from the same object
    mac.begin();
    mac.update(bytes("Hi There"), 8);
    mac.finish(second);

    EXPECT_EQ(toHex(first, 32), toHex(second, 32));
    EXPECT_EQ(toHex(first, 32),
              "b0344c61d8db38535ca8afceaf0bf12b881dc200c9833da726e9376c2e32cff7");
}

// --- PBKDF2-HMAC-SHA-256 ----------------------------------------------------

namespace {

std::string pbkdf2Hex(const std::string& pw, const std::string& salt,
                      uint32_t iters, size_t dkLen) {
    std::vector<uint8_t> out(dkLen);
    SBC::pbkdf2HmacSha256(bytes(pw.c_str()), pw.size(),
                          bytes(salt.c_str()), salt.size(),
                          iters, out.data(), dkLen);
    return toHex(out.data(), out.size());
}

} // namespace

// This is the vector StoreTest pins too: the format depends on it.
TEST(SbcPbkdf2, PasswordSaltOneIteration) {
    EXPECT_EQ(pbkdf2Hex("password", "salt", 1, 32),
              "120fb6cffcf8b32c43e7225256c4f837a86548c92ccc35480805987cb70be17b");
}

TEST(SbcPbkdf2, PasswordSaltTwoIterations) {
    EXPECT_EQ(pbkdf2Hex("password", "salt", 2, 32),
              "ae4d0c95af6b46d32d0adff928f06dd02a303f8ef3c251dfd6e2d85a95474c43");
}

TEST(SbcPbkdf2, PasswordSalt4096Iterations) {
    EXPECT_EQ(pbkdf2Hex("password", "salt", 4096, 32),
              "c5e478d59288c841aa530db6845c4c8d962893a001ce4e11a4963873aa98134a");
}

// RFC 7914 s11. dkLen of 64 spans two output blocks, which is the multi-block
// path PasswordStore actually uses (it asks for 96).
TEST(SbcPbkdf2, Rfc7914_PasswdSalt_TwoBlocks) {
    EXPECT_EQ(pbkdf2Hex("passwd", "salt", 1, 64),
              "55ac046e56e3089fec1691c22544b605f94185216dde0465e68b9d57c20dacbc"
              "49ca9cccf179b645991664b39d77ef317c71b845b1e30bd509112041d3a19783");
}

// RFC 7914 s11, second vector: 80000 iterations over two blocks.
TEST(SbcPbkdf2, Rfc7914_PasswordNaCl_ManyIterations) {
    EXPECT_EQ(pbkdf2Hex("Password", "NaCl", 80000, 64),
              "4ddcd8f60b98be21830cee5ef22701f9641a4418d04c0414aeff08876b34ab56"
              "a1d425a1225833549adb841b51c9b3176a272bdebba1d078478f62b397f33c8d");
}

// Truncation must be a prefix: a short request equals the head of a long one.
TEST(SbcPbkdf2, ShortOutputIsAPrefixOfLong) {
    const std::string long64 = pbkdf2Hex("passwd", "salt", 1, 64);
    const std::string short20 = pbkdf2Hex("passwd", "salt", 1, 20);
    EXPECT_EQ(short20, long64.substr(0, 40));
}

// The property that motivated the rewrite: cost scales with the number of
// output blocks, so the 96 bytes PasswordStore asks for runs the iteration
// count three times. This does not assert timing -- it pins the block boundary
// so the arithmetic behind that claim stays visible and correct.
TEST(SbcPbkdf2, OutputBlocksAreIndependentOfDkLen) {
    const std::string ninetySix = pbkdf2Hex("password", "salt", 2, 96);
    const std::string thirtyTwo = pbkdf2Hex("password", "salt", 2, 32);
    EXPECT_EQ(thirtyTwo, ninetySix.substr(0, 64));
    EXPECT_EQ(ninetySix.size(), size_t(192));
}

// --- HKDF-SHA-256 (RFC 5869, Appendix A.1-A.3) -----------------------------
// FPMQ2 and SBC version 1 both depend on these exact bytes.

namespace {

std::vector<uint8_t> range(int from, int to) { // [from, to)
    std::vector<uint8_t> v;
    for (int b = from; b < to; ++b)
        v.push_back(static_cast<uint8_t>(b));
    return v;
}

struct HkdfOut {
    std::string prk;
    std::string okm;
};

HkdfOut hkdfHex(const std::vector<uint8_t>& ikm, const std::vector<uint8_t>& salt,
                const std::vector<uint8_t>& info, size_t len) {
    uint8_t prk[32];
    SBC::hkdfExtract(salt.data(), salt.size(), ikm.data(), ikm.size(), prk);
    std::vector<uint8_t> okm(len);
    EXPECT_TRUE(SBC::hkdfExpand(prk, sizeof prk, info.data(), info.size(), okm.data(), len));

    std::vector<uint8_t> oneShot(len);
    EXPECT_TRUE(SBC::hkdfSha256(salt.data(), salt.size(), ikm.data(), ikm.size(),
                                info.data(), info.size(), oneShot.data(), len));
    EXPECT_EQ(oneShot, okm); // the convenience wrapper is extract-then-expand
    return {toHex(prk, sizeof prk), toHex(okm.data(), okm.size())};
}

} // namespace

TEST(SbcHkdf, Rfc5869Case1) {
    const HkdfOut r = hkdfHex(std::vector<uint8_t>(22, 0x0b), range(0x00, 0x0d),
                              range(0xf0, 0xfa), 42);
    EXPECT_EQ(r.prk, "077709362c2e32df0ddc3f0dc47bba6390b6c73bb50f9c3122ec844ad7c2b3e5");
    EXPECT_EQ(r.okm, "3cb25f25faacd57a90434f64d0362f2a2d2d0a90cf1a5a4c5db02d56ecc4c5bf"
                     "34007208d5b887185865");
}

// Long inputs: 80-byte IKM, salt and info, and an output spanning three blocks.
TEST(SbcHkdf, Rfc5869Case2_LongInputs) {
    const HkdfOut r = hkdfHex(range(0x00, 0x50), range(0x60, 0xb0), range(0xb0, 0x100), 82);
    EXPECT_EQ(r.prk, "06a6b88c5853361a06104c9ceb35b45cef760014904671014a193f40c15fc244");
    EXPECT_EQ(r.okm, "b11e398dc80327a1c8e7f78c596a49344f012eda2d4efad8a050cc4c19afa97c"
                     "59045a99cac7827271cb41c65e590e09da3275600c2f09b8367793a9aca3db71"
                     "cc30c58179ec3e87c14c01d5c1f3434f1d87");
}

// Empty salt and info: the case SBC version 1 uses (empty salt).
TEST(SbcHkdf, Rfc5869Case3_EmptySaltAndInfo) {
    const HkdfOut r = hkdfHex(std::vector<uint8_t>(22, 0x0b), {}, {}, 42);
    EXPECT_EQ(r.prk, "19ef24a32c717b167f33a91d6f648bdf96596776afdb6377ac434c1c293ccb04");
    EXPECT_EQ(r.okm, "8da4e775a563c18f715f802a063c5a31b8a11f5c5ee1879ec3454e5f3c738d2d"
                     "9d201395faa4b61a96c8");
}

TEST(SbcHkdf, RefusesOverlongOutput) {
    const uint8_t prk[32] = {};
    std::vector<uint8_t> out(SBC::kHkdfMaxLen + 1, 0xAA);
    EXPECT_TRUE(SBC::hkdfExpand(prk, sizeof prk, nullptr, 0, out.data(), SBC::kHkdfMaxLen));
    EXPECT_FALSE(SBC::hkdfExpand(prk, sizeof prk, nullptr, 0, out.data(), out.size()));
}
// Copyright Ben Paul Wise. All Rights Reserved.
