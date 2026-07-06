// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef SBC_LEGACY_H
#define SBC_LEGACY_H

// -----------------------------------------------------------------------------
// Read-only decoder for the *legacy* .sbc container written by the old FLTK
// FPwdMan / sbctool. This reproduces EGApp::innerDecryptContents from the
// original app (endecrypt.cpp): parse the radix-64 ASCII armor, CRC-check it,
// SBC-CBC decrypt, verify the bpwDigest, and zlib-inflate to the plaintext XML.
//
// New files are written in the modern container (see crypto/PasswordStore); this
// path exists only so existing password stores keep opening.
// -----------------------------------------------------------------------------

#include "sbc_core.h"

#include <cstdint>
#include <string>
#include <vector>

namespace SBC {

// Thrown when the armor CRC fails or the framing is structurally impossible.
class CorruptFile : public SbcError {};

// Thrown when decryption succeeds structurally but the bpwDigest does not match
// (the overwhelmingly likely cause is a wrong passphrase).
class WrongPassphrase : public SbcError {};

// Returns true if `fileText` looks like a legacy container: a line of two
// decimal integers followed by radix-64 text. Cheap heuristic used by the store
// to pick the decoder when there is no modern magic marker.
bool looksLikeLegacy(const std::string& fileText);

// Decrypt a legacy .sbc file's full text; returns the plaintext (XML) bytes.
// Throws CorruptFile or WrongPassphrase on failure.
std::vector<uint8_t> legacyDecrypt(const std::string& fileText,
                                   const std::string& passphrase);

} // namespace SBC

#endif // SBC_LEGACY_H
// Copyright Ben Paul Wise. All Rights Reserved.
