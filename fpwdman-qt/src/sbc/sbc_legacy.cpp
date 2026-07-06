// Copyright Ben Paul Wise. All Rights Reserved.
#include "sbc_legacy.h"

#include <cctype>
#include <sstream>

#include <zlib.h> // legacy payload is raw zlib (compress/uncompress), not gzip

// Faithful port of the legacy decrypt pipeline. Byte layouts and index
// arithmetic mirror endecrypt.cpp + sbcutils.cpp exactly.

namespace SBC {

namespace {

// framing / container constants (from egapp.h and sbc.h)
constexpr uint32_t kBlockWords = 32;      // K_Block: 32 words = 128 bytes / block
constexpr uint32_t kKeyWords = 16;        // N_Key
constexpr uint32_t kRandomHeaderLen = 16; // random IV prefix inside the plaintext
constexpr uint32_t kDigestLen = 4;        // bpwDigest length
constexpr uint32_t kLengthCrc = 3;        // radix-64 armor CRC-24
constexpr uint32_t kLengthNpt = 4;        // stored octet-count field

constexpr uint32_t kCrc24Init = 0xb704ceu;
constexpr uint32_t kCrc24Poly = 0x1864cfbu;

// CRC-24 over octets (RFC 2440), used by the radix-64 armor.
uint32_t crc24(const uint8_t* octets, size_t len) {
    uint32_t crc = kCrc24Init;
    while (len--) {
        crc ^= static_cast<uint32_t>(*octets++) << 16;
        for (int i = 0; i < 8; ++i) {
            crc <<= 1;
            if (crc & 0x1000000u)
                crc ^= kCrc24Poly;
        }
    }
    return crc & 0xffffffu;
}

// one radix-64 character -> its 6-bit value
int intFromRad64(uint8_t c) {
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= 'a' && c <= 'z') return c - 'a' + 26;
    if (c >= '0' && c <= '9') return c - '0' + 52;
    if (c == '+') return 62;
    if (c == '/') return 63;
    return 0; // '=' padding and stray chars decode to 0, as in the original
}

// 4 radix-64 chars -> 3 octets
void octetFromRad64(const uint8_t* rad, uint8_t* octets) {
    const uint8_t r0 = static_cast<uint8_t>(intFromRad64(rad[0]));
    const uint8_t r1 = static_cast<uint8_t>(intFromRad64(rad[1]));
    const uint8_t r2 = static_cast<uint8_t>(intFromRad64(rad[2]));
    const uint8_t r3 = static_cast<uint8_t>(intFromRad64(rad[3]));
    octets[0] = static_cast<uint8_t>((r0 << 2) ^ (r1 >> 4));
    octets[1] = static_cast<uint8_t>(((r1 & 15) << 4) ^ (r2 >> 2));
    octets[2] = static_cast<uint8_t>(((r2 & 3) << 6) ^ r3);
}

// radix-64 string -> octets (n must be a multiple of 4)
std::vector<uint8_t> r64StringToOctet(const std::string& r64) {
    const size_t n = r64.size();
    if (n % 4 != 0)
        throw CorruptFile();
    const size_t nBlocks = n / 4;
    std::vector<uint8_t> out(nBlocks * 3);
    for (size_t i = 0; i < nBlocks; ++i) {
        uint8_t rad[4];
        for (int j = 0; j < 4; ++j)
            rad[j] = static_cast<uint8_t>(r64[4 * i + j]);
        octetFromRad64(rad, &out[3 * i]);
    }
    return out;
}

// Undo fullOctetStringToR64: strip [ CRC(3) | npt(4) | octets | padding ],
// verify the armor CRC, and return the `npt` real cipher octets.
std::vector<uint8_t> fullR64StringToOctet(const std::string& r64) {
    const std::vector<uint8_t> octets = r64StringToOctet(r64);
    const size_t numOctets = octets.size();
    if (numOctets <= (kLengthCrc + kLengthNpt))
        throw CorruptFile();

    uint32_t crcRecovered = 0;
    for (uint32_t i = 0; i < kLengthCrc; ++i)
        crcRecovered = (crcRecovered << 8) ^ octets[kLengthCrc - (i + 1)];

    const size_t crcTextLength = numOctets - kLengthCrc;
    const uint32_t crcComputed = crc24(&octets[kLengthCrc], crcTextLength);
    if (crcComputed != crcRecovered)
        throw CorruptFile();

    uint32_t npt = 0;
    for (uint32_t i = 0; i < kLengthNpt; ++i)
        npt = (npt << 8) ^ octets[kLengthCrc + kLengthNpt - (i + 1)];

    if (npt == 0 || npt >= numOctets)
        throw CorruptFile();

    std::vector<uint8_t> out(npt);
    for (uint32_t i = 0; i < npt; ++i)
        out[i] = octets[i + kLengthCrc + kLengthNpt];
    return out;
}

// Undo sbcCompress: input is [ LE rawLen(4) | LE crc32(4) | zlib stream ].
std::vector<uint8_t> sbcUncompress(const std::vector<uint8_t>& in) {
    if (in.size() < 8)
        throw CorruptFile();

    uLongf rawLen = 0;
    rawLen = (rawLen << 8) ^ in[3];
    rawLen = (rawLen << 8) ^ in[2];
    rawLen = (rawLen << 8) ^ in[1];
    rawLen = (rawLen << 8) ^ in[0];

    uLong storedCrc = 0;
    storedCrc = (storedCrc << 8) ^ in[7];
    storedCrc = (storedCrc << 8) ^ in[6];
    storedCrc = (storedCrc << 8) ^ in[5];
    storedCrc = (storedCrc << 8) ^ in[4];

    std::vector<uint8_t> out(rawLen);
    uLongf outLen = rawLen;
    const int rc = uncompress(out.data(), &outLen,
                              in.data() + 8,
                              static_cast<uLong>(in.size() - 8));
    if (rc != Z_OK || outLen != rawLen)
        throw CorruptFile();

    uLong crc = crc32(0L, Z_NULL, 0);
    crc = crc32(crc, out.data(), static_cast<uInt>(outLen));
    if (crc != storedCrc)
        throw CorruptFile();

    return out;
}

// Parse the readEncFile ASCII container: "numPrintChar numFullChar" then the
// radix-64 body (whitespace between rows is ignored). Returns the r64 string.
std::string parseEncFile(const std::string& fileText) {
    std::istringstream in(fileText);
    unsigned long numPrintChar = 0;
    unsigned long numFullChar = 0;
    if (!(in >> numPrintChar >> numFullChar))
        throw CorruptFile();
    if (!(numPrintChar > numFullChar) || !(numFullChar > 0))
        throw CorruptFile();

    std::string r64;
    r64.reserve(numPrintChar);
    std::string row;
    while (in >> row)
        r64 += row;

    if (r64.size() < numPrintChar)
        throw CorruptFile();
    r64.resize(numPrintChar);
    return r64;
}

} // namespace

bool looksLikeLegacy(const std::string& fileText) {
    std::istringstream in(fileText);
    unsigned long a = 0;
    unsigned long b = 0;
    if (!(in >> a >> b))
        return false;
    return a > b && b > 0;
}

std::vector<uint8_t> legacyDecrypt(const std::string& fileText,
                                   const std::string& passphrase) {
    // 1. ASCII armor -> cipher octets (armor CRC verified here).
    const std::string r64 = parseEncFile(fileText);
    const std::vector<uint8_t> cipherOctets = fullR64StringToOctet(r64);

    const size_t numOct = cipherOctets.size();
    if (numOct == 0 || numOct % (4 * kBlockWords) != 0)
        throw CorruptFile();

    // 2. Decrypt (CBC, zero IV, key = fitOctet-expanded passphrase).
    SBCipher cipher(kBlockWords, kKeyWords);
    cipher.keySetupFromString(passphrase);
    const std::vector<Word> cipherWords = deoctify(cipherOctets);
    const std::vector<Word> plainWords = cipher.decipherCBC(cipherWords);
    const std::vector<uint8_t> qtext = octify(plainWords); // little-endian per word
    const size_t nqt = qtext.size();

    // 3. Framing: [16 IV][4 digest][4 LE length][compressed][0xFF pad].
    //    length lives at bytes 20..23, assembled big-endian.
    const uint32_t off = kRandomHeaderLen + kDigestLen; // 20
    if (nqt < off + 4)
        throw CorruptFile();
    uint32_t textLen = 0;
    for (uint32_t i = 0; i < 4; ++i)
        textLen = (textLen << 8) ^ qtext[off + i];

    const uint32_t textOff = kRandomHeaderLen + kDigestLen + 4; // 24
    // Wrong passphrase almost always yields an absurd length; reject cleanly.
    if (static_cast<uint64_t>(textOff) + textLen > nqt)
        throw WrongPassphrase();
    if (static_cast<uint64_t>(textOff) + textLen + (4 * kBlockWords) < nqt)
        throw WrongPassphrase();
    if (textLen <= 8)
        throw WrongPassphrase();

    // Pull the compressed text, undoing the within-word byte reversal.
    std::vector<uint8_t> text0(textLen);
    for (uint32_t i = 0; i < textLen; ++i) {
        const uint32_t k = i + textOff;
        const uint32_t j = 4 * (k / 4) + (3 - (k % 4));
        text0[i] = qtext[j];
    }

    // Pull the recovered digest (also reversed within each 4-byte group).
    uint8_t rDigest[kDigestLen];
    for (uint32_t i = 0; i < kDigestLen; ++i) {
        const uint32_t j = (4 * (i / 4) + 3 - (i % 4)) + kRandomHeaderLen;
        rDigest[i] = qtext[j];
    }

    // 4. Integrity: recompute the digest of the recovered compressed text.
    uint8_t digest0[kDigestLen];
    try {
        bpwDigest(text0.data(), textLen, digest0, kDigestLen);
    } catch (const SbcError&) {
        throw WrongPassphrase();
    }
    for (uint32_t i = 0; i < kDigestLen; ++i)
        if (digest0[i] != rDigest[i])
            throw WrongPassphrase();

    // 5. Inflate to the plaintext XML.
    return sbcUncompress(text0);
}

} // namespace SBC
// Copyright Ben Paul Wise. All Rights Reserved.
