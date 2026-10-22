// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PASSWORDSTORE_H
#define PASSWORDSTORE_H

// -----------------------------------------------------------------------------
// PasswordStore: reads and writes the encrypted password database.
//
//   * Reading auto-detects the container: the current "FPMQ2" format, the
//     earlier "FPMQ1" format, or the legacy .sbc format written by the old FLTK
//     app (decoded via the Qt-free SBC library).
//   * Writing always produces FPMQ2: a random salt, one PBKDF2-HMAC-SHA256
//     block expanded with HKDF into separate cipher and MAC keys, SBC-CBC
//     encryption with key schedule version 1, and an encrypt-then-MAC
//     HMAC-SHA256 tag over the header and ciphertext (which gives clean
//     wrong-passphrase detection). Opening an older file and saving it converts it.
//   * FPMQ1 differed in three ways: 96 bytes straight from PBKDF2 (three times
//     the work, for no extra security), SBC key schedule version 0, and a tag
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

enum class Format { Fpmq2, Fpmq1, Legacy };

// Detect which container `fileText` holds (does not decrypt).
Format detectFormat(const QByteArray& fileText);

// Decrypt+parse a file into entries. Throws Error subclasses on failure.
std::vector<SiteEntry> openFile(const QString& path, const QString& passphrase);

// Serialize+encrypt entries to FPMQ2 and write to `path`.
void saveFile(const QString& path, const QString& passphrase,
              const std::vector<SiteEntry>& entries);

// --- exposed for unit tests -------------------------------------------------
// Container round-trip on in-memory text (no file I/O). encodeModern writes
// FPMQ2; decodeModern reads FPMQ1 or FPMQ2.
QByteArray encodeModern(const QString& passphrase, const std::vector<SiteEntry>& entries);
std::vector<SiteEntry> decodeModern(const QByteArray& fileText, const QString& passphrase);

// Writes the superseded FPMQ1 format, so its reader stays tested. The app never
// calls this.
QByteArray encodeFpmq1(const QString& passphrase, const std::vector<SiteEntry>& entries);

QByteArray serializeXml(const std::vector<SiteEntry>& entries);
std::vector<SiteEntry> parseXml(const QByteArray& xml);

// PBKDF2-HMAC-SHA256 (exposed for a known-answer test).
QByteArray pbkdf2HmacSha256(const QByteArray& password, const QByteArray& salt,
                            int iterations, int dkLen);

} // namespace pwstore

#endif // PASSWORDSTORE_H
// Copyright Ben Paul Wise. All Rights Reserved.
