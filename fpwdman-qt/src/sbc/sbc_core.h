// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef SBC_CORE_H
#define SBC_CORE_H

// -----------------------------------------------------------------------------
// SBC: Ben Wise's "Sliding Block Cipher", re-implemented self-contained for
// fpwdman-qt. This is a faithful port of the deterministic "version 0" cipher
// from the original reference library (perm1 / subkey0 / j0 / fe4-fd4,
// blockSize = 32 words, keySize = 16 words, 24 rounds, permInterval = 64).
//
// The port keeps the exact arithmetic of the original so that files written by
// the old FLTK app decrypt bit-for-bit; only the scaffolding changes (fixed
// width uint32_t words, std::vector, thrown exceptions instead of assert/abort,
// no chahar/mblock/FLTK dependency).
//
// This header is toolkit-agnostic: no Qt. It is the shared cipher engine used
// by both the legacy reader (sbc_legacy) and the modern container (PasswordStore).
// -----------------------------------------------------------------------------

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace SBC {

using Word = uint32_t; // the cipher works on 32-bit words throughout

// --- errors -----------------------------------------------------------------
class SbcError {}; // base for all SBC failures
class BadBlockSize : public SbcError {};
class BadKeySize : public SbcError {};

// --- 32-bit primitives (exact ports of the reference implementation) --------
Word rotl32(Word x, unsigned int y);
Word rotr32(Word x, unsigned int y);
uint8_t rotl8(uint8_t x, unsigned int y);
uint8_t rotr8(uint8_t x, unsigned int y);

// qTransform(x) = (x + 1) * (2x + 1)  (mod 2^32).  1-to-1, no fixed points.
Word qTransform(Word x);

// mix(x, y) = qTransform(x ^ P1) ^ qTransform(y ^ P2)
Word mix(Word x, Word y);

// stretch-and-fold index map used by perm1
Word fold(Word i, Word n);

// --- key-derivation helpers (ports of digest.cpp) ---------------------------
// fitOctet: 1-to-1 fit of `in` (m octets) into `out` (n octets). Used by the
// legacy key schedule to expand a typed passphrase to a fixed key length.
void fitOctet(const uint8_t* in, size_t m, uint8_t* out, size_t n);

// bpwDigest: the legacy integrity digest. Requires m > n (throws SbcError
// otherwise, which the caller treats as corrupt / wrong passphrase).
void bpwDigest(const uint8_t* text, size_t m, uint8_t* digest, size_t n);

// --- octet <-> word marshalling (little-endian per word, as octifySBCB) -----
std::vector<uint8_t> octify(const std::vector<Word>& words);
std::vector<Word> deoctify(const std::vector<uint8_t>& bytes); // size must be a multiple of 4

// --- the cipher -------------------------------------------------------------

// Which subkey each step uses. Everything else (fe4, j0, perm1 and its
// schedule) is the same in both versions.
enum class KeySchedule {
    // Version 0: step i uses qTransform(key[i mod keySize]). Every round uses the
    // same subkeys, which leaves the cipher open to slide attacks. Kept so legacy
    // and FPMQ1 files still decrypt; do not write new data with it.
    V0,
    // Version 1: the key is expanded with HKDF-SHA-256 into one subkey per step,
    //   S = HKDF(IKM = key, salt = empty,
    //            info = "SBC v1 subkeys" || INT32BE(blockSize) || INT32BE(rounds),
    //            L = 4 * rounds * blockSize),
    // each 4 bytes packed big-endian. See doc/sbc-round-constants.md.
    V1
};

class SBCipher {
public:
    // Version-0 parameters by default: 32-word block, 16-word key, 24 rounds.
    // Version 1 supports rounds * blockSize up to 2040 (HKDF's output limit);
    // beyond that the constructor throws BadBlockSize.
    explicit SBCipher(uint32_t blockSizeWords = 32,
                      uint32_t keySizeWords = 16,
                      uint32_t rounds = 24,
                      KeySchedule schedule = KeySchedule::V0);
    ~SBCipher(); // wipes the key and subkeys

    SBCipher(const SBCipher&) = default;
    SBCipher& operator=(const SBCipher&) = default;

    // Legacy key schedule: fitOctet-expand the passphrase, then pack big-endian
    // into key words (matches SBCipher::keySetup + fitOctet in the original).
    // A typed passphrase is weak key material: use this only to read old files.
    void keySetupFromString(const std::string& passphrase);

    // Raw key bytes (e.g. KDF output). Version 0 needs exactly 4*keySize bytes,
    // packed big-endian into key words, bypassing fitOctet. Version 1 accepts any
    // non-empty length. Neither slows down guessing: derive keys from passphrases
    // with PBKDF2 or similar first.
    void keySetupRaw(const uint8_t* keyBytes, size_t n);

    // The subkey for every step, rounds * blockSize of them (exposed for tests).
    const std::vector<Word>& subkeys() const { return sub_; }

    // CBC with a zero IV (matches encipherCBC/decipherCBC). Input length must be
    // a whole number of blocks (a multiple of blockSize words).
    std::vector<Word> encipherCBC(const std::vector<Word>& plain) const;
    std::vector<Word> decipherCBC(const std::vector<Word>& cipher) const;

    uint32_t blockSizeWords() const { return K_; }
    uint32_t keySizeWords() const { return N_; }

private:
    void encipherBlock(Word* x) const; // in place, K_ words
    void decipherBlock(Word* y) const; // in place, K_ words

    uint32_t K_;             // block size, in words
    uint32_t N_;             // key size, in words
    uint32_t rounds_;        // number of rounds
    uint32_t permInterval_;  // apply perm1 every this many steps (2*K_)
    KeySchedule schedule_;
    std::vector<Word> key_;  // N_ words (version 0 only)
    std::vector<Word> sub_;  // rounds_ * K_ step subkeys
};

} // namespace SBC

#endif // SBC_CORE_H
// Copyright Ben Paul Wise. All Rights Reserved.
