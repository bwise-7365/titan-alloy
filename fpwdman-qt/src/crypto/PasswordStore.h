// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PASSWORDSTORE_H
#define PASSWORDSTORE_H

// -----------------------------------------------------------------------------
// PasswordStore: reads and writes the encrypted password database.
//
//   * Reading auto-detects the container: the current "FPMQ3" format, the
//     earlier "FPMQ2" and "FPMQ1" formats, or the legacy .sbc format written by
//     the old FLTK app (decoded via the Qt-free SBC library).
//   * Writing always produces FPMQ3: a random salt, one PBKDF2-HMAC-SHA256
//     block expanded with HKDF into separate cipher and MAC keys, SBC-CBC
//     encryption with SBC version 2, and an encrypt-then-MAC HMAC-SHA256 tag
//     over the header and ciphertext (which gives clean wrong-passphrase
//     detection). Opening an older file and saving it converts it.
//   * FPMQ2 differs from FPMQ3 only in its HKDF labels and in using SBC
//     version 1 (no data-dependent rotation, unkeyed permutation).
//   * FPMQ1 differed from FPMQ2 in three ways: 96 bytes straight from PBKDF2
//     (three times the work, for no extra security), SBC version 0, and a tag
//     over the ciphertext only. See doc/sbc-round-constants.md.
//
// The on-disk payload is XML (the same SiteEntry schema the old app used); both
// the legacy <SiteTable> root and the current <FpwdMan><SiteTable> root parse.
// -----------------------------------------------------------------------------

#include <vector>

#include <QByteArray>
#include <QString>

#include "SiteEntry.h"

namespace pwstore {

// --- errors -----------------------------------------------------------------
class Error {
public:
    virtual ~Error() = default;
};
class WrongPassphrase : public Error {};            // bad password (or tampered)
class CorruptFile : public Error {};                // structurally invalid
class IoError : public Error {
public:
    explicit IoError(QString m) : message(std::move(m)) {}
    QString message;
};

enum class Format { Fpmq3, Fpmq2, Fpmq1, Legacy };

// Detect which container `fileText` holds (does not decrypt).
Format detectFormat(const QByteArray& fileText);

// Decrypt+parse a file into entries. Throws Error subclasses on failure.
std::vector<SiteEntry> openFile(const QString& path, const QString& passphrase);

// Serialize+encrypt entries to FPMQ3 and write to `path`.
void saveFile(const QString& path, const QString& passphrase,
              const std::vector<SiteEntry>& entries);

// --- exposed for unit tests -------------------------------------------------
// Container round-trip on in-memory text (no file I/O). encodeModern writes
// FPMQ3; decodeModern reads FPMQ1, FPMQ2 or FPMQ3.
QByteArray encodeModern(const QString& passphrase, const std::vector<SiteEntry>& entries);
std::vector<SiteEntry> decodeModern(const QByteArray& fileText, const QString& passphrase);

// Write the superseded FPMQ1 and FPMQ2 formats, so their readers stay tested.
// The app never calls these.
QByteArray encodeFpmq1(const QString& passphrase, const std::vector<SiteEntry>& entries);
QByteArray encodeFpmq2(const QString& passphrase, const std::vector<SiteEntry>& entries);

QByteArray serializeXml(const std::vector<SiteEntry>& entries);
std::vector<SiteEntry> parseXml(const QByteArray& xml);

// PBKDF2-HMAC-SHA256 (exposed for a known-answer test).
QByteArray pbkdf2HmacSha256(const QByteArray& password, const QByteArray& salt,
                            int iterations, int dkLen);

} // namespace pwstore

#endif // PASSWORDSTORE_H
// Copyright Ben Paul Wise. All Rights Reserved.
