// Copyright Ben Paul Wise. All Rights Reserved.
#ifndef PASSWORDSTORE_H
#define PASSWORDSTORE_H

// -----------------------------------------------------------------------------
// PasswordStore: reads and writes the encrypted password database.
//
//   * Reading auto-detects the container: the modern self-describing "FPMQ1"
//     format, or the legacy .sbc format written by the old FLTK app (decoded
//     via the Qt-free SBC library).
//   * Writing always produces the modern format: a random salt + PBKDF2-
//     HMAC-SHA256 key derivation, SBC-CBC encryption, and an encrypt-then-MAC
//     HMAC-SHA256 tag (which gives clean wrong-passphrase detection).
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

enum class Format { Modern, Legacy };

// Detect which container `fileText` holds (does not decrypt).
Format detectFormat(const QByteArray& fileText);

// Decrypt+parse a file into entries. Throws Error subclasses on failure.
std::vector<SiteEntry> openFile(const QString& path, const QString& passphrase);

// Serialize+encrypt entries to the modern format and write to `path`.
void saveFile(const QString& path, const QString& passphrase,
              const std::vector<SiteEntry>& entries);

// --- exposed for unit tests -------------------------------------------------
// Modern container round-trip on in-memory text (no file I/O).
QByteArray encodeModern(const QString& passphrase, const std::vector<SiteEntry>& entries);
std::vector<SiteEntry> decodeModern(const QByteArray& fileText, const QString& passphrase);

QByteArray serializeXml(const std::vector<SiteEntry>& entries);
std::vector<SiteEntry> parseXml(const QByteArray& xml);

// PBKDF2-HMAC-SHA256 (exposed for a known-answer test).
QByteArray pbkdf2HmacSha256(const QByteArray& password, const QByteArray& salt,
                            int iterations, int dkLen);

} // namespace pwstore

#endif // PASSWORDSTORE_H
// Copyright Ben Paul Wise. All Rights Reserved.
