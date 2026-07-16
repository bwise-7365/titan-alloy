// Copyright Ben Paul Wise. All Rights Reserved.
#include "PasswordStore.h"

#include <cstdint>
#include <cstring>

#include <QFile>
#include <QList>
#include <QRandomGenerator>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "sbc_core.h"
#include "sbc_kdf.h"
#include "sbc_legacy.h"

namespace pwstore {

namespace {

const QByteArray kMagic = QByteArrayLiteral("FPMQ1");
constexpr int kIterations = 600000; // OWASP-recommended floor for PBKDF2-HMAC-SHA256
constexpr int kSaltLen = 16;
constexpr int kNonceLen = 16;
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

QByteArray hmacSha256(const QByteArray& key, const QByteArray& data) {
    QByteArray out(32, Qt::Uninitialized);
    SBC::hmacSha256(reinterpret_cast<const uint8_t*>(key.constData()),
                    static_cast<size_t>(key.size()),
                    reinterpret_cast<const uint8_t*>(data.constData()),
                    static_cast<size_t>(data.size()),
                    reinterpret_cast<uint8_t*>(out.data()));
    return out;
}

// SBC-CBC encrypt a byte-aligned (multiple of 128) framing buffer.
QByteArray sbcEncryptFraming(const QByteArray& sbcKey, const QByteArray& framing) {
    SBC::SBCipher cipher(32, 16);
    cipher.keySetupRaw(reinterpret_cast<const uint8_t*>(sbcKey.constData()), sbcKey.size());
    const std::vector<SBC::Word> words = SBC::deoctify(toBytes(framing));
    return fromBytes(SBC::octify(cipher.encipherCBC(words)));
}

QByteArray sbcDecryptFraming(const QByteArray& sbcKey, const QByteArray& cipherBytes) {
    SBC::SBCipher cipher(32, 16);
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

// The two subkeys the container needs, split from one PBKDF2 output. Zeroizes
// itself on destruction so the derived key material does not linger in memory.
struct DerivedKeys {
    QByteArray sbcKey;
    QByteArray macKey;
    ~DerivedKeys() {
        secureZero(sbcKey);
        secureZero(macKey);
    }
};

DerivedKeys deriveKeys(const QString& passphrase, const QByteArray& salt, int iterations) {
    QByteArray pw = passphrase.toUtf8();
    QByteArray dk = pbkdf2HmacSha256(pw, salt, iterations, kSbcKeyLen + kMacKeyLen);
    DerivedKeys keys;
    keys.sbcKey = dk.left(kSbcKeyLen);
    keys.macKey = dk.mid(kSbcKeyLen, kMacKeyLen);
    secureZero(dk);
    secureZero(pw);
    return keys;
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

// --- modern container -------------------------------------------------------

QByteArray encodeModern(const QString& passphrase, const std::vector<SiteEntry>& entries) {
    QByteArray xml = serializeXml(entries);
    const QByteArray salt = randomBytes(kSaltLen);
    const DerivedKeys keys = deriveKeys(passphrase, salt, kIterations);

    // framing: [ 16 nonce | 4 LE len | xml | 0xFF pad to 128 ]
    QByteArray framing = randomBytes(kNonceLen);
    framing += le32(static_cast<quint32>(xml.size()));
    framing += xml;
    const int rem = framing.size() % kBlockBytes;
    if (rem != 0)
        framing += QByteArray(kBlockBytes - rem, static_cast<char>(0xFF));

    const QByteArray cipherBytes = sbcEncryptFraming(keys.sbcKey, framing);
    const QByteArray tag = hmacSha256(keys.macKey, cipherBytes); // encrypt-then-MAC

    // The cleartext copies have served their purpose; scrub them before they free.
    secureZero(xml);
    secureZero(framing);

    QByteArray out = kMagic + "\n";
    out += "kdf=pbkdf2-sha256; iters=" + QByteArray::number(kIterations) +
           "; salt=" + salt.toBase64() + "\n";
    out += "mac=hmac-sha256; tag=" + tag.toBase64() + "\n";
    out += wrapBase64(cipherBytes.toBase64(), 64);
    return out;
}

std::vector<SiteEntry> decodeModern(const QByteArray& fileText, const QString& passphrase) {
    const QList<QByteArray> lines = fileText.split('\n');
    if (lines.isEmpty() || lines.first().trimmed() != kMagic)
        throw CorruptFile();

    QByteArray salt;
    QByteArray tag;
    int iters = 0;
    QByteArray body;
    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray ln = lines[i];
        if (ln.contains("salt=")) {
            salt = QByteArray::fromBase64(headerField(ln, "salt="));
            iters = headerField(ln, "iters=").toInt();
        } else if (ln.contains("tag=")) {
            tag = QByteArray::fromBase64(headerField(ln, "tag="));
        } else {
            body += ln.trimmed();
        }
    }

    if (salt.isEmpty() || tag.isEmpty() || iters <= 0)
        throw CorruptFile();

    const QByteArray cipherBytes = QByteArray::fromBase64(body);
    if (cipherBytes.isEmpty() || (cipherBytes.size() % kBlockBytes) != 0)
        throw CorruptFile();

    const DerivedKeys keys = deriveKeys(passphrase, salt, iters);

    // encrypt-then-MAC: a mismatch means wrong passphrase (or tampering). The
    // compare is constant-time so verification leaks no timing about the tag.
    if (!constantTimeEquals(hmacSha256(keys.macKey, cipherBytes), tag))
        throw WrongPassphrase();

    QByteArray framing;
    try {
        framing = sbcDecryptFraming(keys.sbcKey, cipherBytes);
    } catch (const SBC::SbcError&) {
        throw CorruptFile();
    }

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

// --- public entry points ----------------------------------------------------

Format detectFormat(const QByteArray& fileText) {
    QByteArray head = fileText;
    if (head.size() > 64)
        head = head.left(64);
    if (head.trimmed().startsWith(kMagic))
        return Format::Modern;
    return Format::Legacy;
}

std::vector<SiteEntry> openFile(const QString& path, const QString& passphrase) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        throw IoError(f.errorString());
    const QByteArray text = f.readAll();
    f.close();

    if (detectFormat(text) == Format::Modern)
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
    const QByteArray out = encodeModern(passphrase, entries);
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        throw IoError(f.errorString());
    if (f.write(out) != out.size()) {
        f.close();
        throw IoError(f.errorString());
    }
    f.close();
}

} // namespace pwstore
// Copyright Ben Paul Wise. All Rights Reserved.
