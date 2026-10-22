// Copyright Ben Paul Wise. All Rights Reserved.
#include "PasswordStore.h"

#include <cstdint>
#include <cstring>

#include <QFile>
#include <QList>
#include <QRandomGenerator>
#include <QSaveFile>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "sbc_core.h"
#include "sbc_kdf.h"
#include "sbc_legacy.h"

namespace pwstore {

namespace {

const QByteArray kMagic1 = QByteArrayLiteral("FPMQ1"); // read only
const QByteArray kMagic2 = QByteArrayLiteral("FPMQ2"); // read and written
constexpr int kIterations = 600000; // OWASP-recommended floor for PBKDF2-HMAC-SHA256
constexpr int kSaltLen = 16;
constexpr int kNonceLen = 16;
constexpr int kMasterLen = 32; // FPMQ2: one PBKDF2 block, expanded with HKDF
constexpr int kSbcKeyLen = 64; // 4 * 16-word key
constexpr int kMacKeyLen = 32;
constexpr int kBlockBytes = 128; // 32 words

std::vector<uint8_t> toBytes(const QByteArray& b) {
    const auto* p = reinterpret_cast<const uint8_t*>(b.constData());
    return std::vector<uint8_t>(p, p + b.size());
}

QByteArray fromBytes(const std::vector<uint8_t>& v) {
    return QByteArray(reinterpret_cast<const char*>(v.data()), static_cast<int>(v.size()));
}

QByteArray randomBytes(int n) {
    QByteArray b(n, Qt::Uninitialized);
    QRandomGenerator* rng = QRandomGenerator::system();
    int i = 0;
    while (i < n) {
        quint32 r = rng->generate();
        for (int k = 0; k < 4 && i < n; ++k) {
            b[i++] = static_cast<char>(r & 0xFF);
            r >>= 8;
        }
    }
    return b;
}

QByteArray le32(quint32 v) {
    QByteArray b(4, 0);
    b[0] = static_cast<char>(v & 0xFF);
    b[1] = static_cast<char>((v >> 8) & 0xFF);
    b[2] = static_cast<char>((v >> 16) & 0xFF);
    b[3] = static_cast<char>((v >> 24) & 0xFF);
    return b;
}

quint32 readLe32(const QByteArray& b, int off) {
    return static_cast<quint32>(static_cast<uint8_t>(b[off])) |
           (static_cast<quint32>(static_cast<uint8_t>(b[off + 1])) << 8) |
           (static_cast<quint32>(static_cast<uint8_t>(b[off + 2])) << 16) |
           (static_cast<quint32>(static_cast<uint8_t>(b[off + 3])) << 24);
}

QByteArray wrapBase64(const QByteArray& b64, int width) {
    QByteArray out;
    for (int i = 0; i < b64.size(); i += width) {
        out += b64.mid(i, width);
        out += '\n';
    }
    return out;
}

// Extract the value after `key` in a header line, up to ';', whitespace or EOL.
QByteArray headerField(const QByteArray& line, const char* key) {
    int idx = line.indexOf(key);
    if (idx < 0)
        return {};
    idx += static_cast<int>(std::strlen(key));
    int end = idx;
    while (end < line.size()) {
        const char c = line[end];
        if (c == ';' || c == ' ' || c == '\r' || c == '\n' || c == '\t')
            break;
        ++end;
    }
    return line.mid(idx, end - idx);
}

// The key-derivation header line, identical in FPMQ1 and FPMQ2.
QByteArray kdfLine(int iterations, const QByteArray& salt) {
    return "kdf=pbkdf2-sha256; iters=" + QByteArray::number(iterations) +
           "; salt=" + salt.toBase64();
}

QByteArray hmacSha256(const QByteArray& key, const QByteArray& data) {
    QByteArray out(32, Qt::Uninitialized);
    SBC::hmacSha256(reinterpret_cast<const uint8_t*>(key.constData()),
                    static_cast<size_t>(key.size()),
                    reinterpret_cast<const uint8_t*>(data.constData()),
                    static_cast<size_t>(data.size()),
                    reinterpret_cast<uint8_t*>(out.data()));
    return out;
}

// HKDF-Expand of a 32-byte pseudorandom key into `len` bytes bound to `info`.
QByteArray hkdfExpand(const QByteArray& prk, const char* info, int len) {
    QByteArray out(len, Qt::Uninitialized);
    SBC::hkdfExpand(reinterpret_cast<const uint8_t*>(prk.constData()),
                    static_cast<size_t>(prk.size()),
                    reinterpret_cast<const uint8_t*>(info), std::strlen(info),
                    reinterpret_cast<uint8_t*>(out.data()), static_cast<size_t>(len));
    return out;
}

// SBC-CBC encrypt a byte-aligned (multiple of 128) framing buffer. FPMQ1 used key
// schedule version 0; FPMQ2 uses version 1.
QByteArray sbcEncryptFraming(const QByteArray& sbcKey, const QByteArray& framing,
                             SBC::KeySchedule schedule) {
    SBC::SBCipher cipher(32, 16, 24, schedule);
    cipher.keySetupRaw(reinterpret_cast<const uint8_t*>(sbcKey.constData()), sbcKey.size());
    const std::vector<SBC::Word> words = SBC::deoctify(toBytes(framing));
    return fromBytes(SBC::octify(cipher.encipherCBC(words)));
}

QByteArray sbcDecryptFraming(const QByteArray& sbcKey, const QByteArray& cipherBytes,
                             SBC::KeySchedule schedule) {
    SBC::SBCipher cipher(32, 16, 24, schedule);
    cipher.keySetupRaw(reinterpret_cast<const uint8_t*>(sbcKey.constData()), sbcKey.size());
    const std::vector<SBC::Word> words = SBC::deoctify(toBytes(cipherBytes));
    return fromBytes(SBC::octify(cipher.decipherCBC(words)));
}

// Overwrite a buffer's bytes in place before it is freed. Best-effort: QByteArray
// is copy-on-write, so this scrubs only the copy we hold, not any earlier detach.
// The volatile pointer stops the compiler from optimizing the writes away.
void secureZero(QByteArray& b) {
    if (b.isEmpty())
        return;
    volatile char* p = b.data(); // data() detaches, giving us a unique buffer to wipe
    for (int i = 0; i < b.size(); ++i)
        p[i] = 0;
}

// Length-independent-branch byte comparison for MAC tags: never short-circuits on
// the first differing byte, so it leaks no timing signal about how much matched.
bool constantTimeEquals(const QByteArray& a, const QByteArray& b) {
    if (a.size() != b.size())
        return false;
    unsigned char diff = 0;
    for (int i = 0; i < a.size(); ++i)
        diff |= static_cast<unsigned char>(a[i]) ^ static_cast<unsigned char>(b[i]);
    return diff == 0;
}

// The two subkeys the container needs. Zeroizes itself on destruction so the
// derived key material does not linger in memory.
struct DerivedKeys {
    QByteArray sbcKey;
    QByteArray macKey;
    ~DerivedKeys() {
        secureZero(sbcKey);
        secureZero(macKey);
    }
};

// FPMQ1: 96 bytes straight from PBKDF2. That runs the iteration count three
// times, yet an attacker testing a guess needs only the block holding the MAC
// key, so two of the three runs cost the user time and add no security.
DerivedKeys deriveKeysFpmq1(const QString& passphrase, const QByteArray& salt, int iterations) {
    QByteArray pw = passphrase.toUtf8();
    QByteArray dk = pbkdf2HmacSha256(pw, salt, iterations, kSbcKeyLen + kMacKeyLen);
    DerivedKeys keys;
    keys.sbcKey = dk.left(kSbcKeyLen);
    keys.macKey = dk.mid(kSbcKeyLen, kMacKeyLen);
    secureZero(dk);
    secureZero(pw);
    return keys;
}

// FPMQ2: one 32-byte PBKDF2 block (the slow, guess-resisting step), expanded
// with HKDF into separate cipher and MAC keys (fast, and needs no slowing: its
// input is already a strong key).
DerivedKeys deriveKeysFpmq2(const QString& passphrase, const QByteArray& salt, int iterations) {
    QByteArray pw = passphrase.toUtf8();
    QByteArray master = pbkdf2HmacSha256(pw, salt, iterations, kMasterLen);
    DerivedKeys keys;
    keys.sbcKey = hkdfExpand(master, "FPMQ2 sbc", kSbcKeyLen);
    keys.macKey = hkdfExpand(master, "FPMQ2 mac", kMacKeyLen);
    secureZero(master);
    secureZero(pw);
    return keys;
}

// framing: [ 16 nonce | 4 LE len | xml | 0xFF pad to 128 ]
QByteArray frame(const QByteArray& xml) {
    QByteArray framing = randomBytes(kNonceLen);
    framing += le32(static_cast<quint32>(xml.size()));
    framing += xml;
    const int rem = framing.size() % kBlockBytes;
    if (rem != 0)
        framing += QByteArray(kBlockBytes - rem, static_cast<char>(0xFF));
    return framing;
}

// Undo frame() and parse the XML, scrubbing the cleartext either way.
std::vector<SiteEntry> unframe(QByteArray& framing) {
    if (framing.size() < kNonceLen + 4) {
        secureZero(framing);
        throw CorruptFile();
    }
    const quint32 xlen = readLe32(framing, kNonceLen);
    if (static_cast<qint64>(kNonceLen) + 4 + xlen > framing.size()) {
        secureZero(framing);
        throw CorruptFile();
    }

    QByteArray xml = framing.mid(kNonceLen + 4, static_cast<int>(xlen));
    try {
        std::vector<SiteEntry> entries = parseXml(xml);
        secureZero(xml);
        secureZero(framing);
        return entries;
    } catch (...) {
        secureZero(xml); // scrub the cleartext even when the XML is malformed
        secureZero(framing);
        throw;
    }
}

// A parsed FPMQ1 or FPMQ2 file, not yet authenticated.
struct ModernFile {
    QByteArray magic;   // kMagic1 or kMagic2
    QByteArray kdfLine; // the key-derivation header line, trimmed
    QByteArray salt;
    QByteArray tag;
    int iters = 0;
    QByteArray cipherBytes;
};

// Parse the header and body, and check everything that can be checked before
// deriving keys.
ModernFile parseModern(const QByteArray& fileText) {
    const QList<QByteArray> lines = fileText.split('\n');
    if (lines.isEmpty())
        throw CorruptFile();

    ModernFile f;
    f.magic = lines.first().trimmed();
    if (f.magic != kMagic1 && f.magic != kMagic2)
        throw CorruptFile();

    QByteArray body;
    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray ln = lines[i];
        if (ln.contains("salt=")) {
            f.kdfLine = ln.trimmed();
            f.salt = QByteArray::fromBase64(headerField(ln, "salt="));
            f.iters = headerField(ln, "iters=").toInt();
        } else if (ln.contains("tag=")) {
            f.tag = QByteArray::fromBase64(headerField(ln, "tag="));
        } else {
            body += ln.trimmed();
        }
    }

    // Both formats always write kIterations, so any other count is not one of
    // them. The count must be checked here, before deriving keys: it cannot be
    // authenticated first (the MAC key comes out of PBKDF2), so an unchecked value
    // would let a crafted file run PBKDF2 for as long as it likes on the GUI
    // thread. Malformed text needs no separate test: toInt() returns 0 for it.
    if (f.salt.isEmpty() || f.tag.isEmpty() || f.iters != kIterations)
        throw CorruptFile();

    // FPMQ2 authenticates its header, so it must be exactly what the writer
    // produces; stray fields would otherwise sit outside the MAC.
    if (f.magic == kMagic2 && f.kdfLine != kdfLine(f.iters, f.salt))
        throw CorruptFile();

    f.cipherBytes = QByteArray::fromBase64(body);
    if (f.cipherBytes.isEmpty() || (f.cipherBytes.size() % kBlockBytes) != 0)
        throw CorruptFile();
    return f;
}

} // namespace

// --- PBKDF2-HMAC-SHA256 -----------------------------------------------------

// Thin marshalling shell over SBC::pbkdf2HmacSha256. The derivation itself lives
// in the Qt-free sbc library because Qt's hash classes cannot be copied, and
// copying a keyed mid-hash state is the only way to keep this loop cheap --
// see sbc_kdf.h and doc/kdf-performance-and-security.md. Output is unchanged:
// standard PBKDF2, so existing FPMQ1 files still decrypt.
QByteArray pbkdf2HmacSha256(const QByteArray& password, const QByteArray& salt,
                            int iterations, int dkLen) {
    if (dkLen <= 0)
        return QByteArray();

    QByteArray dk(dkLen, Qt::Uninitialized);
    SBC::pbkdf2HmacSha256(reinterpret_cast<const uint8_t*>(password.constData()),
                          static_cast<size_t>(password.size()),
                          reinterpret_cast<const uint8_t*>(salt.constData()),
                          static_cast<size_t>(salt.size()),
                          static_cast<uint32_t>(iterations < 0 ? 0 : iterations),
                          reinterpret_cast<uint8_t*>(dk.data()),
                          static_cast<size_t>(dkLen));
    return dk;
}

// --- XML mapping ------------------------------------------------------------

QByteArray serializeXml(const std::vector<SiteEntry>& entries) {
    QByteArray data;
    QXmlStreamWriter xml(&data);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("FpwdMan");
    xml.writeStartElement("SiteTable");
    for (const auto& e : entries) {
        xml.writeStartElement("SiteEntry");
        xml.writeTextElement("title", e.Title);
        xml.writeTextElement("site", e.Site);
        xml.writeTextElement("userid", e.UserID);
        xml.writeTextElement("password", e.Password);
        xml.writeTextElement("comments", e.Comment);
        xml.writeEndElement();
    }
    xml.writeEndElement();
    xml.writeEndElement();
    xml.writeEndDocument();
    return data;
}

std::vector<SiteEntry> parseXml(const QByteArray& xml) {
    std::vector<SiteEntry> entries;
    QXmlStreamReader r(xml);
    while (!r.atEnd()) {
        const QXmlStreamReader::TokenType tt = r.readNext();
        if (tt == QXmlStreamReader::StartElement &&
            r.name().toString() == QStringLiteral("SiteEntry")) {
            SiteEntry e;
            while (!r.atEnd()) {
                const QXmlStreamReader::TokenType t2 = r.readNext();
                if (t2 == QXmlStreamReader::EndElement &&
                    r.name().toString() == QStringLiteral("SiteEntry"))
                    break;
                if (t2 == QXmlStreamReader::StartElement) {
                    const QString name = r.name().toString();
                    const QString text = r.readElementText();
                    if (name == QStringLiteral("title")) e.Title = text;
                    else if (name == QStringLiteral("site")) e.Site = text;
                    else if (name == QStringLiteral("userid")) e.UserID = text;
                    else if (name == QStringLiteral("password")) e.Password = text;
                    else if (name == QStringLiteral("comments")) e.Comment = text;
                }
            }
            entries.push_back(e);
        }
    }
    if (r.hasError())
        throw CorruptFile();
    return entries;
}

// --- modern containers ------------------------------------------------------

// FPMQ2: PBKDF2 + HKDF keys, SBC version 1, and an HMAC over the header lines
// and the ciphertext (encrypt-then-MAC).
QByteArray encodeModern(const QString& passphrase, const std::vector<SiteEntry>& entries) {
    QByteArray xml = serializeXml(entries);
    const QByteArray salt = randomBytes(kSaltLen);
    const DerivedKeys keys = deriveKeysFpmq2(passphrase, salt, kIterations);

    QByteArray framing = frame(xml);
    const QByteArray cipherBytes =
        sbcEncryptFraming(keys.sbcKey, framing, SBC::KeySchedule::V1);
    const QByteArray header = kMagic2 + "\n" + kdfLine(kIterations, salt) + "\n";
    const QByteArray tag = hmacSha256(keys.macKey, header + cipherBytes);

    // The cleartext copies have served their purpose; scrub them before they free.
    secureZero(xml);
    secureZero(framing);

    QByteArray out = header;
    out += "mac=hmac-sha256; tag=" + tag.toBase64() + "\n";
    out += wrapBase64(cipherBytes.toBase64(), 64);
    return out;
}

// FPMQ1, exactly as the app wrote it before FPMQ2: three-block PBKDF2, SBC
// version 0, HMAC over the ciphertext only. Kept so the FPMQ1 reader stays tested.
QByteArray encodeFpmq1(const QString& passphrase, const std::vector<SiteEntry>& entries) {
    QByteArray xml = serializeXml(entries);
    const QByteArray salt = randomBytes(kSaltLen);
    const DerivedKeys keys = deriveKeysFpmq1(passphrase, salt, kIterations);

    QByteArray framing = frame(xml);
    const QByteArray cipherBytes =
        sbcEncryptFraming(keys.sbcKey, framing, SBC::KeySchedule::V0);
    const QByteArray tag = hmacSha256(keys.macKey, cipherBytes); // encrypt-then-MAC

    secureZero(xml);
    secureZero(framing);

    QByteArray out = kMagic1 + "\n";
    out += kdfLine(kIterations, salt) + "\n";
    out += "mac=hmac-sha256; tag=" + tag.toBase64() + "\n";
    out += wrapBase64(cipherBytes.toBase64(), 64);
    return out;
}

std::vector<SiteEntry> decodeModern(const QByteArray& fileText, const QString& passphrase) {
    const ModernFile f = parseModern(fileText);
    const bool fpmq2 = (f.magic == kMagic2);

    const DerivedKeys keys = fpmq2 ? deriveKeysFpmq2(passphrase, f.salt, f.iters)
                                   : deriveKeysFpmq1(passphrase, f.salt, f.iters);

    // encrypt-then-MAC: a mismatch means wrong passphrase (or tampering). FPMQ2's
    // tag also covers the header, so the version and parameters are bound to it.
    // The compare is constant-time so verification leaks no timing about the tag.
    const QByteArray macInput =
        fpmq2 ? (f.magic + "\n" + f.kdfLine + "\n" + f.cipherBytes) : f.cipherBytes;
    if (!constantTimeEquals(hmacSha256(keys.macKey, macInput), f.tag))
        throw WrongPassphrase();

    QByteArray framing;
    try {
        framing = sbcDecryptFraming(keys.sbcKey, f.cipherBytes,
                                    fpmq2 ? SBC::KeySchedule::V1 : SBC::KeySchedule::V0);
    } catch (const SBC::SbcError&) {
        throw CorruptFile();
    }
    return unframe(framing);
}

// --- public entry points ----------------------------------------------------

Format detectFormat(const QByteArray& fileText) {
    const QByteArray head = fileText.left(64).trimmed();
    if (head.startsWith(kMagic2))
        return Format::Fpmq2;
    if (head.startsWith(kMagic1))
        return Format::Fpmq1;
    return Format::Legacy;
}

std::vector<SiteEntry> openFile(const QString& path, const QString& passphrase) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw IoError(f.errorString());
    const QByteArray text = f.readAll();
    f.close();

    if (detectFormat(text) != Format::Legacy)
        return decodeModern(text, passphrase);

    // Legacy path: decode via the Qt-free SBC library, then parse the XML.
    try {
        const std::vector<uint8_t> xmlBytes =
            SBC::legacyDecrypt(text.toStdString(), passphrase.toStdString());
        return parseXml(fromBytes(xmlBytes));
    } catch (const SBC::WrongPassphrase&) {
        throw WrongPassphrase();
    } catch (const SBC::SbcError&) {
        throw CorruptFile();
    }
}

void saveFile(const QString& path, const QString& passphrase,
              const std::vector<SiteEntry>& entries) {
    // No .bak copy is kept. A backup would preserve the previous file under its
    // old format and old passphrase -- after converting a legacy file, a copy
    // that allows fast passphrase guessing. QSaveFile already protects against a
    // failed save: it writes to a temporary file and replaces `path` only on a
    // successful commit, so the old file survives any error.
    const QByteArray out = encodeModern(passphrase, entries);
    QSaveFile saveFile(path);
    if (!saveFile.open(QIODevice::WriteOnly))
        throw IoError(saveFile.errorString());
    if (saveFile.write(out) != out.size()) {
        saveFile.cancelWriting();
        throw IoError(saveFile.errorString());
    }
    if (!saveFile.commit())
        throw IoError(saveFile.errorString());
}

} // namespace pwstore
// Copyright Ben Paul Wise. All Rights Reserved.
