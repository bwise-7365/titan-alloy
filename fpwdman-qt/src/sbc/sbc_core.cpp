// Copyright Ben Paul Wise. All Rights Reserved.
#include "sbc_core.h"

// Faithful port of the SBC "version 0" cipher (perm1 / subkey0 / j0 / fe4-fd4).
// Every arithmetic detail mirrors the original reference library so that files
// encrypted by the old FLTK app decrypt bit-for-bit.

namespace SBC {

namespace {

// constants from the reference sbc.h / sbcutils.cpp
constexpr Word kPhi = 0x9e3779b9u;        // (sqrt(5)-1)/2 * 2^32
constexpr Word kMagicPrime0 = 0x9ebd87a7u; // magicPrimes[0], used by fitOctet
constexpr Word kMixP1 = 0x9bf7f721u;
constexpr Word kMixP2 = 0x8da7e0e7u;
constexpr unsigned int kFoldNum = 8;      // perm1 fold count
constexpr unsigned int kFoldRot = 7;      // perm1 fold rotation

// j0: J(i) = i-1, wrapping to blockSize-1 at 0
inline Word j0(Word i, Word blockSize) {
    return (i > 0) ? (i - 1) : (blockSize - 1);
}

// subkey0(i) = qTransform(key[i mod keySize])
inline Word subkey0(const std::vector<Word>& key, unsigned int i) {
    return qTransform(key[i % key.size()]);
}

// fe4: C = (qTransform(B ^ S) <<< 7) + A
inline Word fe4(Word A, Word B, Word S) {
    return rotl32(qTransform(B ^ S), 7) + A; // wraps mod 2^32 naturally
}

// fd4: A = C - (qTransform(B ^ S) <<< 7)
inline Word fd4(Word C, Word B, Word S) {
    return C - rotl32(qTransform(B ^ S), 7); // wraps mod 2^32 naturally
}

// perm1: the rotate-stretch-fold permutation (data-dependent, key-independent).
// permP == true permutes (encipher); false inverts (decipher).
void perm1(Word* words, uint32_t blockSize, bool permP) {
    std::vector<Word> Y(blockSize);
    Word sumBlock = 0;
    for (uint32_t j = 0; j < blockSize; ++j)
        sumBlock ^= qTransform(words[j]);

    std::vector<Word> sum(kFoldNum);
    for (unsigned int i = 0; i < kFoldNum; ++i) {
        sum[i] = sumBlock;
        sumBlock = rotl32(sumBlock, kFoldRot);
    }

    for (unsigned int i = 0; i < kFoldNum; ++i) {
        for (uint32_t j = 0; j < blockSize; ++j) {
            const Word s = permP ? sum[i] : sum[kFoldNum - (1 + i)];
            const uint32_t j2 =
                static_cast<uint32_t>((static_cast<uint64_t>(j) + s) % blockSize);
            const uint32_t k = static_cast<uint32_t>(fold(j2, blockSize));
            if (permP)
                Y[k] = words[j];
            else
                Y[j] = words[k];
        }
        for (uint32_t j = 0; j < blockSize; ++j)
            words[j] = Y[j];
    }
}

} // namespace

// --- primitives -------------------------------------------------------------

Word rotl32(Word x, unsigned int y) {
    y &= 31u;
    return y == 0 ? x : static_cast<Word>((x << y) | (x >> (32 - y)));
}

Word rotr32(Word x, unsigned int y) {
    y &= 31u;
    return y == 0 ? x : static_cast<Word>((x >> y) | (x << (32 - y)));
}

uint8_t rotl8(uint8_t x, unsigned int y) {
    y &= 7u;
    return y == 0 ? x : static_cast<uint8_t>((x << y) | (x >> (8 - y)));
}

uint8_t rotr8(uint8_t x, unsigned int y) {
    y &= 7u;
    return y == 0 ? x : static_cast<uint8_t>((x >> y) | (x << (8 - y)));
}

Word qTransform(Word x) {
    // (x + 1) * (2x + 1), all mod 2^32
    const Word t1 = x + 1u;
    const Word t3 = (2u * x) + 1u;
    return t1 * t3;
}

Word mix(Word x, Word y) {
    return qTransform(x ^ kMixP1) ^ qTransform(y ^ kMixP2);
}

Word fold(Word i, Word n) {
    if ((2u * i) <= (n - 1u))
        return 2u * i;
    return 2u * (n - i) - 1u;
}

// --- key-derivation helpers -------------------------------------------------

void fitOctet(const uint8_t* in, size_t m, uint8_t* out, size_t n) {
    if (m == 0 || n == 0)
        throw SbcError();

    const size_t k = 2u * (n + m); // scan `in` >= twice, wrap `out` >= twice
    const unsigned int b = 3;
    const Word P = kMagicPrime0;

    for (size_t i = 0; i < n; ++i)
        out[i] = 0;

    // x accumulates without a 32-bit mask in the original; only bits 24..31 are
    // ever read (via x >> 24), and those bits are identical whether or not the
    // sum wraps at 2^32, so a 64-bit accumulator reproduces both old builds.
    uint64_t x = 0;
    for (size_t i = 0; i < k; ++i) {
        x = x + P;
        const uint8_t y = static_cast<uint8_t>(x >> 24);
        const uint8_t z = rotl8(out[i % n], b);
        out[(i + 1) % n] = static_cast<uint8_t>((in[i % m] ^ z) + y);
    }
}

void bpwDigest(const uint8_t* text, size_t m, uint8_t* digest, size_t n) {
    if (!(n > 0) || !(m > n))
        throw SbcError(); // matches the original assert(n>0), assert(m>n)

    const size_t a = 1 + ((n + 1) / m);
    size_t k = a * m;
    const unsigned int b = 3;

    for (size_t i = 0; i < n; ++i)
        digest[i] = text[(k - 1 - i) % m];

    k = m + (2 * n); // loop the digest around >= twice, read the text >= once
    Word sum = 0;
    for (size_t i = 0; i < k; ++i) {
        sum = sum + kPhi;
        const uint8_t z = static_cast<uint8_t>(text[i % m] ^ rotl8(digest[i % n], b));
        digest[(i + 1) % n] = static_cast<uint8_t>(z ^ sum);
    }
}

// --- octet <-> word marshalling (little-endian per word) --------------------

std::vector<uint8_t> octify(const std::vector<Word>& words) {
    std::vector<uint8_t> out(words.size() * 4);
    size_t k = 0;
    for (Word w : words) {
        out[k++] = static_cast<uint8_t>(w & 0xFF);
        out[k++] = static_cast<uint8_t>((w >> 8) & 0xFF);
        out[k++] = static_cast<uint8_t>((w >> 16) & 0xFF);
        out[k++] = static_cast<uint8_t>((w >> 24) & 0xFF);
    }
    return out;
}

std::vector<Word> deoctify(const std::vector<uint8_t>& bytes) {
    if (bytes.size() % 4 != 0)
        throw SbcError();
    std::vector<Word> words(bytes.size() / 4);
    for (size_t j = 0; j < words.size(); ++j) {
        const size_t base = 4 * j;
        words[j] = static_cast<Word>(bytes[base]) |
                   (static_cast<Word>(bytes[base + 1]) << 8) |
                   (static_cast<Word>(bytes[base + 2]) << 16) |
                   (static_cast<Word>(bytes[base + 3]) << 24);
    }
    return words;
}

// --- SBCipher ---------------------------------------------------------------

SBCipher::SBCipher(uint32_t blockSizeWords, uint32_t keySizeWords, uint32_t rounds)
    : K_(blockSizeWords), N_(keySizeWords), rounds_(rounds),
      permInterval_(2 * blockSizeWords), key_(keySizeWords, 0) {
    if (K_ == 0)
        throw BadBlockSize();
    if (N_ == 0)
        throw BadKeySize();
}

void SBCipher::keySetupFromString(const std::string& passphrase) {
    std::vector<uint8_t> expanded(4 * N_);
    const auto* p = reinterpret_cast<const uint8_t*>(passphrase.data());
    fitOctet(p, passphrase.size(), expanded.data(), expanded.size());
    keySetupRaw(expanded.data(), expanded.size());
    for (auto& e : expanded)
        e = 0xFF; // scrub the local expansion
}

void SBCipher::keySetupRaw(const uint8_t* keyBytes, size_t n) {
    if (n != 4u * N_)
        throw BadKeySize();
    for (uint32_t i = 0; i < N_; ++i) {
        key_[i] = (static_cast<Word>(keyBytes[4 * i]) << 24) |
                  (static_cast<Word>(keyBytes[4 * i + 1]) << 16) |
                  (static_cast<Word>(keyBytes[4 * i + 2]) << 8) |
                  static_cast<Word>(keyBytes[4 * i + 3]);
    }
}

void SBCipher::encipherBlock(Word* x) const {
    const uint32_t k = K_;
    const uint32_t total = rounds_ * k;
    for (uint32_t i = 0; i < total; ++i) {
        const Word j = j0(i % k, k);
        x[i % k] = fe4(x[i % k], x[j], subkey0(key_, i));
        if (permInterval_ > 0 && i > 0 && (i % permInterval_) == 0)
            perm1(x, k, /*permP=*/true);
    }
}

void SBCipher::decipherBlock(Word* y) const {
    const uint32_t k = K_;
    const uint32_t total = rounds_ * k;
    for (uint32_t ii = total; ii-- > 0;) {
        const uint32_t i = ii;
        if (permInterval_ > 0 && i > 0 && (i % permInterval_) == 0)
            perm1(y, k, /*permP=*/false);
        const Word j = j0(i % k, k);
        y[i % k] = fd4(y[i % k], y[j], subkey0(key_, i));
    }
}

std::vector<Word> SBCipher::encipherCBC(const std::vector<Word>& plain) const {
    if (plain.size() % K_ != 0)
        throw BadBlockSize();
    const size_t numBlocks = plain.size() / K_;
    std::vector<Word> cipher(plain.size());
    std::vector<Word> curr(K_);

    for (size_t b = 0; b < numBlocks; ++b) {
        const size_t off = b * K_;
        for (uint32_t w = 0; w < K_; ++w)
            curr[w] = plain[off + w] ^ (b == 0 ? 0u : cipher[off - K_ + w]);
        encipherBlock(curr.data());
        for (uint32_t w = 0; w < K_; ++w)
            cipher[off + w] = curr[w];
    }
    return cipher;
}

std::vector<Word> SBCipher::decipherCBC(const std::vector<Word>& cipher) const {
    if (cipher.size() % K_ != 0)
        throw BadBlockSize();
    const size_t numBlocks = cipher.size() / K_;
    std::vector<Word> plain(cipher.size());
    std::vector<Word> curr(K_);

    for (size_t b = 0; b < numBlocks; ++b) {
        const size_t off = b * K_;
        for (uint32_t w = 0; w < K_; ++w)
            curr[w] = cipher[off + w];
        decipherBlock(curr.data());
        for (uint32_t w = 0; w < K_; ++w)
            plain[off + w] = curr[w] ^ (b == 0 ? 0u : cipher[off - K_ + w]);
    }
    return plain;
}

} // namespace SBC
// Copyright Ben Paul Wise. All Rights Reserved.
