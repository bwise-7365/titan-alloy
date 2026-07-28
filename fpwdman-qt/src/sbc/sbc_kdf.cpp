// Copyright Ben Paul Wise. All Rights Reserved.
#include "sbc_kdf.h"

#include <cstring>

namespace SBC {

namespace {

inline uint32_t ror(uint32_t x, unsigned n) {
    return (x >> n) | (x << (32 - n));
}

// FIPS 180-4 round constants: the first 32 bits of the fractional parts of the
// cube roots of the first 64 primes.
const uint32_t kK[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

// Overwrite a scratch buffer that held key material. The volatile pointer is
// what stops the compiler from deleting a store it can prove nobody reads.
void wipe(void* p, size_t n) {
    volatile uint8_t* q = static_cast<volatile uint8_t*>(p);
    while (n--)
        *q++ = 0;
}

} // namespace

// --- SHA-256 ----------------------------------------------------------------

Sha256::Sha256() {
    reset();
}

void Sha256::reset() {
    h_[0] = 0x6a09e667; h_[1] = 0xbb67ae85; h_[2] = 0x3c6ef372; h_[3] = 0xa54ff53a;
    h_[4] = 0x510e527f; h_[5] = 0x9b05688c; h_[6] = 0x1f83d9ab; h_[7] = 0x5be0cd19;
    bits_ = 0;
    buflen_ = 0;
    std::memset(buf_, 0, sizeof buf_);
}

void Sha256::compress(const uint8_t block[kBlockLen]) {
    uint32_t w[64];
    for (int i = 0; i < 16; ++i)
        w[i] = (static_cast<uint32_t>(block[4 * i]) << 24) |
               (static_cast<uint32_t>(block[4 * i + 1]) << 16) |
               (static_cast<uint32_t>(block[4 * i + 2]) << 8) |
               static_cast<uint32_t>(block[4 * i + 3]);
    for (int i = 16; i < 64; ++i) {
        const uint32_t s0 = ror(w[i - 15], 7) ^ ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
        const uint32_t s1 = ror(w[i - 2], 17) ^ ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    uint32_t a = h_[0], b = h_[1], c = h_[2], d = h_[3];
    uint32_t e = h_[4], f = h_[5], g = h_[6], hh = h_[7];
    for (int i = 0; i < 64; ++i) {
        const uint32_t S1 = ror(e, 6) ^ ror(e, 11) ^ ror(e, 25);
        const uint32_t ch = (e & f) ^ (~e & g);
        const uint32_t t1 = hh + S1 + ch + kK[i] + w[i];
        const uint32_t S0 = ror(a, 2) ^ ror(a, 13) ^ ror(a, 22);
        const uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        const uint32_t t2 = S0 + maj;
        hh = g; g = f; f = e; e = d + t1;
        d = c; c = b; b = a; a = t1 + t2;
    }
    h_[0] += a; h_[1] += b; h_[2] += c; h_[3] += d;
    h_[4] += e; h_[5] += f; h_[6] += g; h_[7] += hh;
}

void Sha256::update(const uint8_t* data, size_t len) {
    bits_ += static_cast<uint64_t>(len) * 8;

    if (buflen_ != 0) { // top up a partial block first
        const size_t need = kBlockLen - buflen_;
        const size_t take = len < need ? len : need;
        std::memcpy(buf_ + buflen_, data, take);
        buflen_ += take;
        data += take;
        len -= take;
        if (buflen_ == kBlockLen) {
            compress(buf_);
            buflen_ = 0;
        }
    }
    while (len >= kBlockLen) { // then whole blocks straight from the caller
        compress(data);
        data += kBlockLen;
        len -= kBlockLen;
    }
    if (len != 0) {
        std::memcpy(buf_, data, len);
        buflen_ = len;
    }
}

void Sha256::finish(uint8_t out[kDigestLen]) {
    const uint64_t bits = bits_; // the length to encode is the one before padding

    buf_[buflen_++] = 0x80;
    if (buflen_ > kBlockLen - 8) { // no room for the length: flush a block first
        std::memset(buf_ + buflen_, 0, kBlockLen - buflen_);
        compress(buf_);
        buflen_ = 0;
    }
    std::memset(buf_ + buflen_, 0, (kBlockLen - 8) - buflen_);
    for (int i = 0; i < 8; ++i)
        buf_[kBlockLen - 8 + i] = static_cast<uint8_t>(bits >> (56 - 8 * i));
    compress(buf_);

    for (int i = 0; i < 8; ++i) {
        out[4 * i]     = static_cast<uint8_t>(h_[i] >> 24);
        out[4 * i + 1] = static_cast<uint8_t>(h_[i] >> 16);
        out[4 * i + 2] = static_cast<uint8_t>(h_[i] >> 8);
        out[4 * i + 3] = static_cast<uint8_t>(h_[i]);
    }
}

// --- HMAC-SHA-256 -----------------------------------------------------------

void HmacSha256::setKey(const uint8_t* key, size_t len) {
    uint8_t k[Sha256::kBlockLen];
    std::memset(k, 0, sizeof k); // keys shorter than a block are zero-padded

    if (len > Sha256::kBlockLen) { // RFC 2104: over-long keys are hashed down
        Sha256 h;
        h.update(key, len);
        h.finish(k);
    } else if (len != 0) {
        std::memcpy(k, key, len);
    }

    uint8_t pad[Sha256::kBlockLen];
    for (size_t i = 0; i < sizeof pad; ++i)
        pad[i] = static_cast<uint8_t>(k[i] ^ 0x36); // ipad
    inner_.reset();
    inner_.update(pad, sizeof pad);

    for (size_t i = 0; i < sizeof pad; ++i)
        pad[i] = static_cast<uint8_t>(k[i] ^ 0x5c); // opad
    outer_.reset();
    outer_.update(pad, sizeof pad);

    wipe(k, sizeof k);
    wipe(pad, sizeof pad);
}

void HmacSha256::begin() {
    cur_ = inner_; // the point of the whole class: a copy, not a re-hash
}

void HmacSha256::update(const uint8_t* data, size_t len) {
    cur_.update(data, len);
}

void HmacSha256::finish(uint8_t out[kDigestLen]) {
    uint8_t h1[kDigestLen];
    cur_.finish(h1);

    Sha256 o = outer_;
    o.update(h1, sizeof h1);
    o.finish(out);

    wipe(h1, sizeof h1);
}

void hmacSha256(const uint8_t* key, size_t keyLen,
                const uint8_t* msg, size_t msgLen,
                uint8_t out[32]) {
    HmacSha256 mac;
    mac.setKey(key, keyLen);
    mac.begin();
    mac.update(msg, msgLen);
    mac.finish(out);
}

// --- PBKDF2-HMAC-SHA-256 ----------------------------------------------------

void pbkdf2HmacSha256(const uint8_t* password, size_t passwordLen,
                      const uint8_t* salt, size_t saltLen,
                      uint32_t iterations,
                      uint8_t* out, size_t dkLen) {
    if (dkLen == 0)
        return;

    HmacSha256 mac;
    mac.setKey(password, passwordLen); // keyed once; every U below reuses the states

    const size_t hLen = HmacSha256::kDigestLen;
    const size_t blocks = (dkLen + hLen - 1) / hLen;

    uint8_t u[HmacSha256::kDigestLen];
    uint8_t t[HmacSha256::kDigestLen];

    for (size_t i = 1; i <= blocks; ++i) {
        // U1 = PRF(password, salt || INT32BE(i))
        const uint8_t idx[4] = {
            static_cast<uint8_t>(i >> 24), static_cast<uint8_t>(i >> 16),
            static_cast<uint8_t>(i >> 8),  static_cast<uint8_t>(i)
        };
        mac.begin();
        mac.update(salt, saltLen);
        mac.update(idx, sizeof idx);
        mac.finish(u);
        std::memcpy(t, u, sizeof t);

        // T = U1 ^ U2 ^ ... ^ Uc, where Un = PRF(password, Un-1).
        for (uint32_t j = 1; j < iterations; ++j) {
            mac.begin();
            mac.update(u, sizeof u);
            mac.finish(u);
            for (size_t k = 0; k < sizeof t; ++k)
                t[k] = static_cast<uint8_t>(t[k] ^ u[k]);
        }

        const size_t off = (i - 1) * hLen;
        const size_t take = (dkLen - off) < hLen ? (dkLen - off) : hLen;
        std::memcpy(out + off, t, take);
    }

    wipe(u, sizeof u);
    wipe(t, sizeof t);
}

} // namespace SBC
// Copyright Ben Paul Wise. All Rights Reserved.
