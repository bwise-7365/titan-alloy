// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef SBC_KDF_H
#define SBC_KDF_H

// -----------------------------------------------------------------------------
// SHA-256, HMAC-SHA-256 and PBKDF2-HMAC-SHA-256, self-contained and Qt-free.
//
// Why this exists rather than calling Qt: PBKDF2 spends all of its time in one
// tight loop, and the only way to make that loop fast is to compute the two
// key-dependent hash states once and copy them per iteration. Qt's
// QCryptographicHash is not copyable and cannot export or restore its state, so
// a Qt-based PBKDF2 is forced to re-absorb the key on every one of its hundreds
// of thousands of iterations, and to heap-allocate a QByteArray for every
// intermediate digest. Owning the state struct here turns both of those costs
// into a memcpy of a few dozen bytes. See doc/kdf-performance-and-security.md.
//
// The output is ordinary, standards-conformant PBKDF2: this is a speed change,
// not a format change, and existing FPMQ1 files still decrypt bit-for-bit.
//
// Living in the sbc library rather than next to PasswordStore keeps the crypto
// primitives toolkit-agnostic and in one place, alongside the cipher itself.
// -----------------------------------------------------------------------------

#include <cstddef>
#include <cstdint>

namespace SBC {

// --- SHA-256 ----------------------------------------------------------------
// FIPS 180-4. Deliberately a plain value type: the implicit copy constructor is
// load-bearing, because cloning a mid-hash state is the whole basis of the fast
// HMAC below. Members are trivial, so the default copy is a cheap memberwise one.
class Sha256 {
public:
    static constexpr size_t kDigestLen = 32;
    static constexpr size_t kBlockLen = 64;

    Sha256();

    void reset();
    void update(const uint8_t* data, size_t len);

    // Appends the padding and writes the digest. The object is spent afterwards;
    // call reset() to hash again.
    void finish(uint8_t out[kDigestLen]);

private:
    void compress(const uint8_t block[kBlockLen]);

    uint32_t h_[8];        // the running 256-bit state
    uint64_t bits_;        // message length so far, in bits
    uint8_t buf_[kBlockLen];
    size_t buflen_;
};

// --- HMAC-SHA-256 -----------------------------------------------------------
// RFC 2104. setKey() absorbs (key ^ ipad) and (key ^ opad) once and keeps the
// two resulting states; begin() then costs a copy rather than a re-hash. For a
// fixed key this cuts the work per message from four SHA-256 compressions to
// two, which is exactly the PBKDF2 inner loop's shape.
class HmacSha256 {
public:
    static constexpr size_t kDigestLen = 32;

    HmacSha256() = default;

    // Keys longer than the 64-byte block are hashed down first, per RFC 2104.
    void setKey(const uint8_t* key, size_t len);

    void begin();                             // start a message: clones the inner state
    void update(const uint8_t* data, size_t len);
    void finish(uint8_t out[kDigestLen]);     // clones the outer state to close

private:
    Sha256 inner_;  // state after absorbing key ^ ipad
    Sha256 outer_;  // state after absorbing key ^ opad
    Sha256 cur_;    // working copy of inner_ for the message in flight
};

// One-shot convenience wrapper.
void hmacSha256(const uint8_t* key, size_t keyLen,
                const uint8_t* msg, size_t msgLen,
                uint8_t out[32]);

// --- PBKDF2-HMAC-SHA-256 ----------------------------------------------------
// RFC 8018. Allocation-free: the whole derivation runs in fixed-size buffers.
//
// Cost note worth knowing at the call site: PBKDF2 runs the full iteration count
// once per hLen (32-byte) block of output, so asking for 96 bytes costs three
// times what asking for 32 does. Prefer deriving one block and expanding it.
//
// `iterations` of 0 or 1 both mean a single pass, matching RFC 8018.
void pbkdf2HmacSha256(const uint8_t* password, size_t passwordLen,
                      const uint8_t* salt, size_t saltLen,
                      uint32_t iterations,
                      uint8_t* out, size_t dkLen);

} // namespace SBC

#endif // SBC_KDF_H
// Copyright Ben Paul Wise. All Rights Reserved.
